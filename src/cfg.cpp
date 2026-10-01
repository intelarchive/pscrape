#include "pscrape/cfg.hpp"
#include <cstring>
#include <optional>
#include <sstream>

namespace pscrape {

static const char* u =
"pscrape - scrape + validate proxy lists\n"
"usage: pscrape [opts]\n"
"  -s <path>       sources file, one url per line  (default sources.txt)\n"
"  -o <path>       output file                     (default proxies.txt)\n"
"  -f <fmt>        plain|auth|json                 (default plain)\n"
"  --thr <n>       concurrency                     (default 64)\n"
"  --timeout <s>   per-op timeout seconds          (default 3)\n"
"  --validate      tcp connect check\n"
"  --http-probe    full http get through proxy (implies --validate)\n"
"  --probe <url>   probe url                       (default httpbin)\n"
"  --no-robots     skip robots.txt check\n"
"  -h, --help      this\n";

std::string usage() { return u; }

static bool need(int i, int argc) { return i + 1 < argc; }

std::optional<cfg> parse_args(int argc, char** argv) {
  cfg c;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "-h" || a == "--help") return std::nullopt;
    else if (a == "-s" && need(i, argc)) c.src_path = argv[++i];
    else if (a == "-o" && need(i, argc)) c.out_path = argv[++i];
    else if (a == "-f" && need(i, argc)) c.fmt = argv[++i];
    else if (a == "--thr" && need(i, argc)) c.thr = (uint32_t)std::stoul(argv[++i]);
    else if (a == "--timeout" && need(i, argc)) c.timeout_s = (uint32_t)std::stoul(argv[++i]);
    else if (a == "--validate") c.validate = true;
    else if (a == "--http-probe") { c.http_probe = true; c.validate = true; }
    else if (a == "--probe" && need(i, argc)) c.probe_url = argv[++i];
    else if (a == "--no-robots") c.robots = false;
    else {
      std::ostringstream e;
      e << "unknown arg: " << a;
      throw std::runtime_error(e.str());
    }
  }
  if (c.thr == 0) c.thr = 1;
  if (c.thr > 1024) c.thr = 1024;
  return c;
}

} // namespace pscrape