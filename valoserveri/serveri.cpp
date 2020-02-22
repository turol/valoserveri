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

	default:
		break;
	}

	return 0;
}



static const struct lws_protocols protocols[] = {
	{ .name                  = "default"
	, .callback              = callback
	, .per_session_data_size = sizeof(struct per_session_data__minimal)
	, .rx_buffer_size        = 128
	, .id                    = 1
	, .user                  = nullptr
	, .tx_packet_size        = 0
	}
	, { "http", lws_callback_http_dummy, 0, 0, 0, nullptr, 0 }
	, { nullptr, nullptr, 0, 0, 0, nullptr, 0 } /* terminator */
};


#endif  // USE_LIBWEBSOCKETS


class Serveri {
	// TODO:
	//  user fd
	//  libwebsockets context
	//  poll array


public:

	// TODO: private

	int UDPfd;

#ifdef USE_LIBWEBSOCKETS

	struct lws_context *ws_context;

#endif  // USE_LIBWEBSOCKETS


	Serveri() = delete;

	Serveri(const Config & /* config */);

	~Serveri();

	Serveri(const Serveri &) noexcept            = delete;
	Serveri(Serveri &&) noexcept                 = default;

	Serveri &operator=(const Serveri &) noexcept = delete;
	Serveri &operator=(Serveri &&) noexcept      = default;

	// TODO: signalQuit

	void run();
};


Serveri::Serveri(const Config & /* config */)
: UDPfd(0)
#ifdef USE_LIBWEBSOCKETS
, ws_context(nullptr)
#endif  // USE_LIBWEBSOCKETS
{
}


Serveri::~Serveri() {
}


int main(int /* argc */, char * /* argv */ []) {
	// TODO: catch all exceptions
	// TODO: parse command line arguments

	// read config file
	valoserveri::Config config("valoserveri.conf");

	Serveri serveri(config);

	DMXController dmx(config);

	int port = config.get("global", "udpPort", 9909);

	// socket
	// TODO: IPv6
	serveri.UDPfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	printf("fd: %d\n", serveri.UDPfd);

	// bind
	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(port);
	// TODO: get bind address from config
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	{
		int retval = bind(serveri.UDPfd, reinterpret_cast<struct sockaddr *>(&bindAddr), sizeof(bindAddr));
		if (retval != 0) {
			printf("bind error: %d \"%s\"\n", errno, strerror(errno));
			close(serveri.UDPfd);
			return 1;
		}
	}

	// recvfrom
	// TODO: make buffer length configurable
	size_t buflen = 512;
	std::vector<char> buffer(buflen, 0);

#ifdef USE_LIBWEBSOCKETS

	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));

	info.port                  = config.get("global", "wsPort", 9910);
	// info.mounts                = &mount;
	info.protocols             = protocols;

	// TODO: configure these
	info.vhost_name            = "127.0.0.1";
	info.ws_ping_pong_interval = 10;
	// info.options               = LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

	serveri.ws_context = lws_create_context(&info);

#endif  // USE_LIBWEBSOCKETS

	// TODO: gracefully handle SIGINT
	// TODO: re-exec on SIGHUP
	while (true) {

#ifdef USE_LIBWEBSOCKETS

		// TODO: do something with retval
		int retval = lws_service(serveri.ws_context, 1);

#endif  // USE_LIBWEBSOCKETS

		struct sockaddr_in from;
		memset(&from, 0, sizeof(from));
		socklen_t fromLength = sizeof(from);

		// TODO: need to poll both this and libwebsockets
		ssize_t len = recvfrom(serveri.UDPfd, buffer.data(), buffer.size(), 0, reinterpret_cast<struct sockaddr *>(&from), &fromLength);
		printf("received %d bytes from \"%s\":%d\n", int(len), inet_ntoa(from.sin_addr), ntohs(from.sin_port));

		// parse packet
		auto lights = parseLightPacket(buffer, len);
		printf("lights: %u\n", (unsigned int) lights.size());

		// TODO: update lights
		for (const auto &l : lights) {
			printf("%u: %u %u %u\n", l.index, l.color.red, l.color.green, l.color.blue);
			dmx.setLightColor(l.index, l.color);
		}

		dmx.update();
	}

#ifdef USE_LIBWEBSOCKETS

	lws_context_destroy(serveri.ws_context);

#endif  // USE_LIBWEBSOCKETS

	close(serveri.UDPfd);
}
