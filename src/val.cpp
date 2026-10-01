#include "pscrape/val.hpp"
#include <atomic>
#include <chrono>
#include <curl/curl.h>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#define PS_CLOSE_SOCK closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define PS_CLOSE_SOCK ::close
#endif

namespace pscrape {

using clk = std::chrono::steady_clock;

#ifdef _WIN32
struct wsa_guard {
  wsa_guard() {
    WSADATA d{};
    WSAStartup(MAKEWORD(2, 2), &d);
  }
  ~wsa_guard() { WSACleanup(); }
};
static wsa_guard g_wsa;
#endif

static int tcp_connect(const std::string& host, uint16_t port, int timeout_ms) {
#ifdef _WIN32
  SOCKET fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == INVALID_SOCKET) return -1;
#else
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;
#endif

  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  if (::inet_pton(AF_INET, host.c_str(), &a.sin_addr) != 1) {
    PS_CLOSE_SOCK(fd);
    return -1;
  }

#ifdef _WIN32
  u_long nb = 1;
  ::ioctlsocket(fd, FIONBIO, &nb);
#else
  int fl = ::fcntl(fd, F_GETFL, 0);
  ::fcntl(fd, F_SETFL, fl | O_NONBLOCK);
#endif

  int rc = ::connect(fd, (sockaddr*)&a, sizeof(a));

#ifdef _WIN32
  if (rc == SOCKET_ERROR) {
    int err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
      PS_CLOSE_SOCK(fd);
      return -1;
    }
    fd_set wf;
    FD_ZERO(&wf);
    FD_SET(fd, &wf);
    timeval sel{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    rc = ::select(0, nullptr, &wf, nullptr, &sel);
    if (rc <= 0) { PS_CLOSE_SOCK(fd); return -1; }
    int soerr = 0;
    int sl = sizeof(soerr);
    ::getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&soerr, &sl);
    if (soerr != 0) { PS_CLOSE_SOCK(fd); return -1; }
  }
#else
  if (rc < 0) {
    fd_set wf;
    FD_ZERO(&wf);
    FD_SET(fd, &wf);
    timeval sel{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    rc = ::select(fd + 1, nullptr, &wf, nullptr, &sel);
    if (rc <= 0) { PS_CLOSE_SOCK(fd); return -1; }
    int soerr = 0;
    socklen_t sl = sizeof(soerr);
    ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &soerr, &sl);
    if (soerr != 0) { PS_CLOSE_SOCK(fd); return -1; }
  }
#endif

  return (int)fd;
}

static size_t sink(char*, size_t sz, size_t n, void*) { return sz * n; }

static bool http_probe(const proxy& p, int timeout_ms,
                       const std::string& url) {
  CURL* h = curl_easy_init();
  if (!h) return false;
  curl_easy_setopt(h, CURLOPT_URL, url.c_str());
  curl_easy_setopt(h, CURLOPT_PROXY,
                   (p.host + ":" + std::to_string(p.port)).c_str());
  if (!p.user.empty()) {
    std::string cred = p.user + ":" + p.pass;
    curl_easy_setopt(h, CURLOPT_PROXYUSERPWD, cred.c_str());
  }
  curl_easy_setopt(h, CURLOPT_TIMEOUT_MS, (long)timeout_ms);
  curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT_MS, (long)timeout_ms);
  curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, sink);
  curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L);
  long code = 0;
  CURLcode rc = curl_easy_perform(h);
  if (rc == CURLE_OK) curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
  curl_easy_cleanup(h);
  return rc == CURLE_OK && code >= 200 && code < 400;
}

std::vector<val_res> validate_tcp(const std::vector<proxy>& ps,
                                  uint32_t thr, uint32_t timeout_s) {
  std::vector<val_res> out(ps.size());
  std::atomic<size_t> next{0};
  auto n = (size_t)thr;
  if (n > ps.size()) n = ps.size();
  if (n == 0) return out;
  int to_ms = (int)timeout_s * 1000;
  std::vector<std::thread> pool;
  pool.reserve(n);
  for (size_t t = 0; t < n; ++t) {
    pool.emplace_back([&] {
      while (true) {
        size_t i = next.fetch_add(1);
        if (i >= ps.size()) break;
        out[i].p = ps[i];
        auto t0 = clk::now();
        int fd = tcp_connect(ps[i].host, ps[i].port, to_ms);
        if (fd >= 0) {
          PS_CLOSE_SOCK(fd);
          out[i].ok = true;
          out[i].latency_ms =
            (int)std::chrono::duration_cast<std::chrono::milliseconds>(
              clk::now() - t0).count();
        } else {
          out[i].err = "tcp";
        }
      }
    });
  }
  for (auto& th : pool) th.join();
  return out;
}

std::vector<val_res> validate_http(const std::vector<proxy>& ps,
                                   uint32_t thr, uint32_t timeout_s,
                                   const std::string& probe_url) {
  auto out = validate_tcp(ps, thr, timeout_s);
  std::atomic<size_t> next{0};
  auto n = (size_t)thr;
  if (n > out.size()) n = out.size();
  if (n == 0) return out;
  int to_ms = (int)timeout_s * 1000;
  std::vector<std::thread> pool;
  pool.reserve(n);
  for (size_t t = 0; t < n; ++t) {
    pool.emplace_back([&] {
      while (true) {
        size_t i = next.fetch_add(1);
        if (i >= out.size()) break;
        if (!out[i].ok) continue;
        auto t0 = clk::now();
        if (!http_probe(out[i].p, to_ms, probe_url)) {
          out[i].ok = false;
          out[i].err = "http";
        } else {
          out[i].latency_ms =
            (int)std::chrono::duration_cast<std::chrono::milliseconds>(
              clk::now() - t0).count();
        }
      }
    });
  }
  for (auto& th : pool) th.join();
  return out;
}

} // namespace pscrape