#include "pscrape/fetch.hpp"
#include <atomic>
#include <curl/curl.h>
#include <mutex>
#include <thread>
#include <vector>

namespace pscrape {

static size_t wr_cb(char* p, size_t sz, size_t n, void* ud) {
  auto* s = static_cast<std::string*>(ud);
  s->append(p, sz * n);
  return sz * n;
}

struct curl_global_guard {
  curl_global_guard() { curl_global_init(CURL_GLOBAL_DEFAULT); }
  ~curl_global_guard() { curl_global_cleanup(); }
};

static fetch_res do_fetch(const std::string& url, uint32_t timeout_s,
                          const std::string& ua) {
  fetch_res r;
  r.url = url;
  CURL* h = curl_easy_init();
  if (!h) { r.err = "curl_easy_init"; return r; }
  curl_easy_setopt(h, CURLOPT_URL, url.c_str());
  curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, wr_cb);
  curl_easy_setopt(h, CURLOPT_WRITEDATA, &r.body);
  curl_easy_setopt(h, CURLOPT_TIMEOUT, (long)timeout_s);
  curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT, (long)timeout_s);
  curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(h, CURLOPT_MAXREDIRS, 5L);
  curl_easy_setopt(h, CURLOPT_USERAGENT, ua.c_str());
  curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(h, CURLOPT_ACCEPT_ENCODING, "");
  CURLcode rc = curl_easy_perform(h);
  if (rc != CURLE_OK) r.err = curl_easy_strerror(rc);
  else curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &r.code);
  curl_easy_cleanup(h);
  if (r.body.size() > (16u << 20)) r.body.resize(16u << 20);
  return r;
}

std::vector<fetch_res> fetch_all(const std::vector<std::string>& urls,
                                 uint32_t thr, uint32_t timeout_s,
                                 const std::string& ua, bool robots) {
  curl_global_guard g;
  std::vector<fetch_res> out(urls.size());
  std::atomic<size_t> next{0};
  std::atomic<bool> stop{false};
  auto n = (size_t)thr;
  if (n > urls.size()) n = urls.size();
  if (n == 0) return out;
  std::vector<std::thread> pool;
  pool.reserve(n);
  for (size_t t = 0; t < n; ++t) {
    pool.emplace_back([&] {
      while (!stop.load(std::memory_order_relaxed)) {
        size_t i = next.fetch_add(1);
        if (i >= urls.size()) break;
        const auto& url = urls[i];
        if (robots && !robots_ok(url, timeout_s, ua)) {
          out[i].url = url;
          out[i].err = "robots disallow";
          continue;
        }
        out[i] = do_fetch(url, timeout_s, ua);
      }
    });
  }
  for (auto& th : pool) th.join();
  return out;
}

bool robots_ok(const std::string& url, uint32_t timeout_s,
               const std::string& ua) {
  auto ps = url.find("://");
  if (ps == std::string::npos) return false;
  auto hs = ps + 3;
  auto he = url.find('/', hs);
  std::string origin = url.substr(0, he == std::string::npos ? url.size() : he);
  std::string path = he == std::string::npos ? "/" : url.substr(he);
  std::string rurl = origin + "/robots.txt";
  auto r = do_fetch(rurl, timeout_s, ua);
  if (r.code == 404 || r.body.empty()) return true;
  if (r.code < 200 || r.code >= 300) return true;
  bool star = false;
  bool disallow_all = false;
  size_t i = 0;
  while (i < r.body.size()) {
    auto e = r.body.find('\n', i);
    if (e == std::string::npos) e = r.body.size();
    std::string ln = r.body.substr(i, e - i);
    if (!ln.empty() && ln.back() == '\r') ln.pop_back();
    size_t a = ln.find_first_not_of(" \t");
    if (a != std::string::npos) {
      std::string t = ln.substr(a);
      if (t.rfind("User-agent:", 0) == 0 || t.rfind("user-agent:", 0) == 0) {
        std::string v = t.substr(11);
        size_t b = v.find_first_not_of(" \t");
        if (b != std::string::npos) v = v.substr(b);
        size_t c = v.find_last_not_of(" \t\r");
        if (c != std::string::npos) v = v.substr(0, c + 1);
        star = (v == "*");
      } else if (star && (t.rfind("Disallow:", 0) == 0 ||
                          t.rfind("disallow:", 0) == 0)) {
        std::string v = t.substr(9);
        size_t b = v.find_first_not_of(" \t");
        if (b != std::string::npos) v = v.substr(b);
        size_t c = v.find_last_not_of(" \t\r");
        if (c != std::string::npos) v = v.substr(0, c + 1);
        if (v == "/" || (!v.empty() && path.rfind(v, 0) == 0)) disallow_all = true;
      }
    }
    i = e + 1;
  }
  return !disallow_all;
}

} // namespace pscrape