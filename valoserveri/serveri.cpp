#include "valoserveri/Config.h"
#include "valoserveri/DMXController.h"
#include "valoserveri/LightPacket.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <libwebsockets.h>


using namespace valoserveri;


#ifdef USE_LIBWEBSOCKETS


/* one of these created for each message */

struct msg {
	void *payload; /* is malloc'd */
	size_t len;
};


/* one of these is created for each client connecting to us */

struct per_session_data__minimal {
	struct per_session_data__minimal *pss_list;
	struct lws *wsi;
	int last; /* the last message number we sent */
};


/* one of these is created for each vhost our protocol is used with */

struct per_vhost_data__minimal {
	struct lws_context *context;
	struct lws_vhost *vhost;
	const struct lws_protocols *protocol;

	struct per_session_data__minimal *pss_list; /* linked-list of live pss*/

	struct msg amsg; /* the one pending message... */
	int current; /* the current message number we are caching */
};


static int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);


class Serveri {
	int                 UDPfd;

	// TODO: quit signaled, quit pipe

	DMXController       dmx;

	size_t              buflen;

#ifdef USE_LIBWEBSOCKETS

	struct lws_context  *ws_context;

	// TODO: does this need to live indefinetely?
	std::vector<lws_protocols>  protocols;

#endif  // USE_LIBWEBSOCKETS

	std::vector<pollfd>  pollfds;


public:

	Serveri() = delete;

	Serveri(const Config & /* config */);

	~Serveri();

	Serveri(const Serveri &) noexcept            = delete;
	Serveri(Serveri &&) noexcept                 = delete;

	Serveri &operator=(const Serveri &) noexcept = delete;
	Serveri &operator=(Serveri &&) noexcept      = delete;

	// TODO: signalQuit

	void addFD(int fd, int events);

	void deleteFD(int fd);

	void setFDEvents(int fd, int events);

	void lightPacket(const std::vector<char> &packet, unsigned int len);

	void run();
};


// TODO: hax, remove
static Serveri *globalServeri = nullptr;


/* destroys the message when everyone has had a copy of it */

static void
__minimal_destroy_message(void *_msg)
{
	struct msg *msg = reinterpret_cast<struct msg *>(_msg);

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
}


static int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {

	printf("callback reason: %d\n", reason);

	struct per_session_data__minimal *pss =
			(struct per_session_data__minimal *)user;
	struct per_vhost_data__minimal *vhd =
			(struct per_vhost_data__minimal *)
			lws_protocol_vh_priv_get(lws_get_vhost(wsi),
					lws_get_protocol(wsi));
	int m;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		vhd = (struct per_vhost_data__minimal *) lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct per_vhost_data__minimal));
		vhd->context = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->vhost = lws_get_vhost(wsi);
		break;

	case LWS_CALLBACK_ESTABLISHED:
		/* add ourselves to the list of live pss held in the vhd */
		lws_ll_fwd_insert(pss, pss_list, vhd->pss_list);
		pss->wsi = wsi;
		pss->last = vhd->current;
		break;

	case LWS_CALLBACK_CLOSED:
		/* remove our closing pss from the list of live pss */
		lws_ll_fwd_remove(struct per_session_data__minimal, pss_list,
				  pss, vhd->pss_list);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (!vhd->amsg.payload)
			break;

		if (pss->last == vhd->current)
			break;

		/* notice we allowed for LWS_PRE in the payload already */
		m = lws_write(wsi, ((unsigned char *)vhd->amsg.payload) +
			      LWS_PRE, vhd->amsg.len, LWS_WRITE_TEXT);
		if (m < (int)vhd->amsg.len) {
			lwsl_err("ERROR %d writing to ws\n", m);
			return -1;
		}

		pss->last = vhd->current;
		break;

	case LWS_CALLBACK_RECEIVE:
		if (vhd->amsg.payload)
			__minimal_destroy_message(&vhd->amsg);

		vhd->amsg.len = len;
		/* notice we over-allocate by LWS_PRE */
		vhd->amsg.payload = malloc(LWS_PRE + len);
		if (!vhd->amsg.payload) {
			lwsl_user("OOM: dropping\n");
			break;
		}

		memcpy((char *)vhd->amsg.payload + LWS_PRE, in, len);
		vhd->current++;

		/*
		 * let everybody know we want to write something on them
		 * as soon as they are ready
		 */
		lws_start_foreach_llp(struct per_session_data__minimal **,
				      ppss, vhd->pss_list) {
			lws_callback_on_writable((*ppss)->wsi);
		} lws_end_foreach_llp(ppss, pss_list);
		break;

	case LWS_CALLBACK_ADD_POLL_FD: {
		auto pa = reinterpret_cast<lws_pollargs *>(in);
		globalServeri->addFD(pa->fd, pa->events);
	} break;

	case LWS_CALLBACK_DEL_POLL_FD: {
		auto pa = reinterpret_cast<lws_pollargs *>(in);
		globalServeri->deleteFD(pa->fd);
	} break;

	case LWS_CALLBACK_CHANGE_MODE_POLL_FD: {
		auto pa = reinterpret_cast<lws_pollargs *>(in);
		globalServeri->setFDEvents(pa->fd, pa->events);
	} break;

	default:
		break;
	}

	return 0;
}


#endif  // USE_LIBWEBSOCKETS


Serveri::Serveri(const Config &config)
: UDPfd(0)
, dmx(config)
, buflen(512)
#ifdef USE_LIBWEBSOCKETS
, ws_context(nullptr)
#endif  // USE_LIBWEBSOCKETS
{
	globalServeri = this;

	int port = config.get("global", "udpPort", 9909);

	// socket
	// TODO: IPv6
	UDPfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	printf("fd: %d\n", UDPfd);

	// bind
	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	// TODO: get bind address from config
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	{
		int retval = bind(UDPfd, reinterpret_cast<struct sockaddr *>(&bindAddr), sizeof(bindAddr));
		if (retval != 0) {
			printf("bind error: %d \"%s\"\n", errno, strerror(errno));
			close(UDPfd);
			UDPfd = 0;
			// TODO: better exception
			throw std::runtime_error("bind error");
		}
	}

	{
		pollfd p;
		memset(&p, 0, sizeof(p));
		p.fd      = UDPfd;
		p.events  = POLLIN | POLLERR;

		pollfds.push_back(p);
	}

	// TODO: make buffer length configurable

#ifdef USE_LIBWEBSOCKETS

	{
		protocols.reserve(3);

		lws_protocols light = {
			  .name                  = "default"
			, .callback              = callback
			, .per_session_data_size = sizeof(struct per_session_data__minimal)
			, .rx_buffer_size        = 128
			, .id                    = 1
			, .user                  = nullptr
			, .tx_packet_size        = 0

		};
		protocols.push_back(light);

		// TODO: monitor protocol

		// TODO: do we need this?
		lws_protocols http = { "http", lws_callback_http_dummy, 0, 0, 0, nullptr, 0 };
		protocols.push_back(http);

		/* terminator */
		lws_protocols terminator = { nullptr, nullptr, 0, 0, 0, nullptr, 0 };
		protocols.push_back(terminator);
	}

	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));

	info.port                  = config.get("global", "wsPort", 9910);
	// info.mounts                = &mount;
	info.protocols             = protocols.data();

	// TODO: configure these
	info.vhost_name            = "127.0.0.1";
	info.ws_ping_pong_interval = 10;
	// info.options               = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	ws_context = lws_create_context(&info);

	// call service once so protocols get registered
	// TODO: why is this needed? shouldn't be
	int retval = lws_service(ws_context, 0);
	printf("lws_service: %d\n", retval);

#endif  // USE_LIBWEBSOCKETS
}


Serveri::~Serveri() {
#ifdef USE_LIBWEBSOCKETS

	lws_context_destroy(ws_context);
	ws_context = nullptr;

#endif  // USE_LIBWEBSOCKETS

	close(UDPfd);
	UDPfd = 0;

	globalServeri = nullptr;
}


void Serveri::addFD(int fd, int events) {
	printf("addFD %d %d\n", fd, events);

	pollfd p;
	memset(&p, 0, sizeof(p));
	p.fd      = fd;
	p.events  = events;

	pollfds.push_back(p);
}


void Serveri::deleteFD(int fd) {
	printf("deleteFD %d\n", fd);

	auto it = pollfds.begin();
	while (it != pollfds.end()) {
		if (it->fd == fd) {
			it = pollfds.erase(it);
		} else {
			it++;
		}
	}
}


void Serveri::setFDEvents(int fd, int events) {
	printf("setModeFD %d %d\n", fd, events);

	auto it = pollfds.begin();
	while (it != pollfds.end()) {
		if (it->fd == fd) {
			it->events = events;
		}

		it++;
	}
}


void Serveri::lightPacket(const std::vector<char> &buffer, unsigned int len) {
	// parse packet
	auto lights = parseLightPacket(buffer, len);
	printf("lights: %u\n", (unsigned int) lights.size());

	// TODO: update lights
	for (const auto &l : lights) {
		printf("%u: %u %u %u\n", l.index, l.color.red, l.color.green, l.color.blue);
		dmx.setLightColor(l.index, l.color);
	}
}


void Serveri::run() {
	std::vector<char> buffer(buflen, 0);

	while (true) {
		int pollresult = poll(pollfds.data(), pollfds.size(), -1);
		printf("pollresult: %d\n", pollresult);

		int count = 0;
		for (pollfd &fd : pollfds) {
			if (count >= pollresult) {
				// everything has been processed, early out
				break;
			}

			if (fd.revents != 0) {
				if (fd.fd == UDPfd) {
					// it's our udp fd

					struct sockaddr_in from;
					memset(&from, 0, sizeof(from));
					socklen_t fromLength = sizeof(from);

					ssize_t len = recvfrom(UDPfd, buffer.data(), buffer.size(), 0, reinterpret_cast<struct sockaddr *>(&from), &fromLength);
					printf("received %d bytes from \"%s\":%d\n", int(len), inet_ntoa(from.sin_addr), ntohs(from.sin_port));

					lightPacket(buffer, len);

					fd.revents = 0;

#ifdef USE_LIBWEBSOCKETS

				} else {

					int retval = lws_service_fd(ws_context, &fd);
					printf("lws_service_fd: %d\n", retval);

#endif  // USE_LIBWEBSOCKETS

				}
			}

			count++;
		}

#ifdef USE_LIBWEBSOCKETS

		// TODO: hax, remove after poll works
		int retval = lws_service(ws_context, 0);
		printf("lws_service: %d\n", retval);

#endif  // USE_LIBWEBSOCKETS

		dmx.update();
	}
}


int main(int /* argc */, char * /* argv */ []) {
	// TODO: catch all exceptions
	// TODO: parse command line arguments

	// read config file
	valoserveri::Config config("valoserveri.conf");

	Serveri serveri(config);

	// TODO: gracefully handle SIGINT
	// TODO: re-exec on SIGHUP
	serveri.run();
}
