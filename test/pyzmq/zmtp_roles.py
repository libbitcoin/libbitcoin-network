"""libzmq (pyzmq) interoperability test for the zmtp socket roles.

Each role server is a test suite in the network test binary
(test/zmtp/harness.cpp) that this script runs by name with ZMTP_HARNESS set
(the cases pass trivially without it), connecting a real libzmq peer of the
compatible socket type to it. The exchange is fixed by
both sides: the server asserts what it reads, this script asserts what it
receives, and the server process exit code reports the server's assertions.

  puller  PUSH   -> [push][one][two] x3
  replier REQ    -> [echo][text] answered [text] x2, [fail] answered
                    [code][message]
  router  DEALER -> (routing id peer1) [][echo][text] answered [][text],
                    then notified [][topic][note], then [][done]

Usage: zmtp_roles.py [--test <path to libbitcoin-network-test.exe>]
"""
import argparse
import os
import struct
import subprocess
import sys
import time

import zmq

PULLER_PORT = 65031
REPLIER_PORT = 65032
ROUTER_PORT = 65033

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_TEST = os.path.normpath(os.path.join(
    HERE, "..", "..", "bin", "x64", "Debug", "v145", "static",
    "libbitcoin-network-test.exe"))


def check(condition, message):
    if not condition:
        raise AssertionError(message)
    print("  ok:", message)


class Server:
    """One role server case of the network test binary."""

    def __init__(self, test, case, port):
        self.name = case
        self.port = port
        self.process = subprocess.Popen(
            [test, f"--run_test=zmtp_harness_{case}_tests",
             "--log_level=test_suite"],
            env={**os.environ, "ZMTP_HARNESS": "1"},
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

        # The case binds on entry; wait for its entry line, then a moment.
        while True:
            line = self.process.stdout.readline()
            if not line:
                raise AssertionError(f"{case}: server exited before entry")
            if "Entering test case" in line:
                break
        time.sleep(0.3)

    def close(self, timeout=10.0):
        try:
            output, _ = self.process.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            self.process.kill()
            output, _ = self.process.communicate()
            print(output)
            raise AssertionError(f"{self.name}: server did not exit")
        code = self.process.returncode
        if code != 0:
            print(output)
        check(code == 0, f"{self.name}: server assertions passed (exit {code})")

    def abort(self):
        """Report the server output after a client side failure."""
        try:
            output, _ = self.process.communicate(timeout=2.0)
        except subprocess.TimeoutExpired:
            self.process.kill()
            output, _ = self.process.communicate()
        print(output)


def connect(context, kind, port):
    socket = context.socket(kind)
    socket.setsockopt(zmq.LINGER, 0)
    socket.setsockopt(zmq.RCVTIMEO, 5000)
    socket.setsockopt(zmq.SNDTIMEO, 5000)
    socket.connect(f"tcp://127.0.0.1:{port}")
    return socket


def run_puller(context, test):
    server = Server(test, "puller", PULLER_PORT)
    push = connect(context, zmq.PUSH, PULLER_PORT)
    for _ in range(3):
        push.send_multipart([b"push", b"one", b"two"])
    server.close()
    push.close()


def run_replier(context, test):
    server = Server(test, "replier", REPLIER_PORT)
    req = connect(context, zmq.REQ, REPLIER_PORT)
    try:
        for text in (b"abc", b"xyz"):
            req.send_multipart([b"echo", text])
            reply = req.recv_multipart()
            check(reply == [text], f"replier: [echo][{text.decode()}] answered {reply}")
        req.send_multipart([b"fail"])
        reply = req.recv_multipart()
        check(len(reply) == 2, f"replier: error reply has code and message {reply}")
        (code,) = struct.unpack("<q", reply[0])
        check(code == -1 and reply[1] == b"nope",
              f"replier: error code {code} message {reply[1]}")
    except Exception:
        server.abort()
        raise
    server.close()
    req.close()


def run_router(context, test):
    server = Server(test, "router", ROUTER_PORT)
    dealer = context.socket(zmq.DEALER)
    dealer.setsockopt(zmq.ROUTING_ID, b"peer1")
    dealer.setsockopt(zmq.LINGER, 0)
    dealer.setsockopt(zmq.RCVTIMEO, 5000)
    dealer.setsockopt(zmq.SNDTIMEO, 5000)
    dealer.connect(f"tcp://127.0.0.1:{ROUTER_PORT}")
    try:
        dealer.send_multipart([b"", b"echo", b"abc"])
        reply = dealer.recv_multipart()
        check(reply == [b"", b"abc"], f"router: [][echo][abc] answered {reply}")
        note = dealer.recv_multipart()
        check(note == [b"", b"topic", b"note"], f"router: notified {note}")
        dealer.send_multipart([b"", b"done"])
    except Exception:
        server.abort()
        raise
    server.close()
    dealer.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--test", default=DEFAULT_TEST)
    args = parser.parse_args()
    print("pyzmq", zmq.pyzmq_version(), "libzmq", zmq.zmq_version())
    context = zmq.Context()
    try:
        run_puller(context, args.test)
        run_replier(context, args.test)
        run_router(context, args.test)
        print("PASS")
        return 0
    except Exception as error:
        print("FAIL:", error)
        return 1
    finally:
        context.destroy(linger=0)


if __name__ == "__main__":
    sys.exit(main())
