#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pscrape {

struct cfg {
  std::string src_path = "sources.txt";
  std::string out_path = "proxies.txt";
  std::string fmt = "plain";
  uint32_t thr = 64;
  uint32_t timeout_s = 3;
  bool validate = false;
  bool http_probe = false;
  bool robots = true;
  std::string probe_url = "http://httpbin.org/ip";
  std::string ua = "pscrape/0.1 (+https://example.invalid)";
};

std::optional<cfg> parse_args(int argc, char** argv);
std::string usage();

} // namespace pscrape