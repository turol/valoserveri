module Main (main) where

-- Somewhat old example in haskell. Some changes might be in place.

-- requires: network, random, bytestring
import Data.ByteString as ByteString hiding (map, putStrLn, mapAccumL)
import Data.List (mapAccumL)
import Data.Word
import Network.Socket hiding (send, sendTo)
import Network.Socket.ByteString (send, sendTo)
import System.Random


light :: Word8 -> (Word8, Word8, Word8) -> ByteString
light n (r, g, b) =
  ByteString.pack [1, n, 0, r, g, b]


main :: IO()
main = withSocketsDo $ do
  let hints = defaultHints
  (addr:_) <- getAddrInfo (Just hints) (Just "10.0.69.214") (Just "9909")
  -- print addr
  let realAddr = addrAddress addr

  s <- socket AF_INET Datagram 0
  bind s (SockAddrInet 0 iNADDR_ANY)

  r <- getStdGen
  let lightIds = [0..23] :: [Word8]
  let (_, lightCmds) = mapAccumL (\g n -> randomLight n g) r lightIds

  let payload = ByteString.concat (ByteString.pack [1, 0, 0]:lightCmds)
  _ <- sendTo s payload realAddr
  return ()

  where
    randomLight :: Word8 -> StdGen -> (StdGen, ByteString)
    randomLight n r =
     (g3, light n (red, green, blue))
     where
       (red,   g1) = random r
       (green, g2) = random g1
       (blue,  g3) = random g2
