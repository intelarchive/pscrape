#include "doctest.h"
#include "pscrape/val.hpp"

using namespace pscrape;

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

static int make_listener(uint16_t& out_port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  REQUIRE(fd >= 0);
  int one = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  a.sin_port = 0;
  REQUIRE(::bind(fd, (sockaddr*)&a, sizeof(a)) == 0);
  socklen_t sl = sizeof(a);
  REQUIRE(::getsockname(fd, (sockaddr*)&a, &sl) == 0);
  out_port = ntohs(a.sin_port);
  REQUIRE(::listen(fd, 8) == 0);
  return fd;
}

TEST_CASE("tcp ok on local listener") {
  uint16_t port = 0;
  int lfd = make_listener(port);
  std::thread acc([&] {
    int c = ::accept(lfd, nullptr, nullptr);
    if (c >= 0) ::close(c);
  });
  std::vector<proxy> ps = {{"127.0.0.1", port, "", ""}};
  auto r = validate_tcp(ps, 1, 2);
  REQUIRE(r.size() == 1);
  CHECK(r[0].ok);
  ::close(lfd);
  acc.join();
}
#endif

TEST_CASE("tcp fail on dead port") {
  std::vector<proxy> ps = {{"127.0.0.1", 1, "", ""}};
  auto r = validate_tcp(ps, 1, 1);
  REQUIRE(r.size() == 1);
  CHECK(!r[0].ok);
}