#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace pscrape {

struct fetch_res {
  std::string url;
  int code = 0;
  std::string body;
  std::string err;
};

std::vector<fetch_res> fetch_all(const std::vector<std::string>& urls,
                                 uint32_t thr, uint32_t timeout_s,
                                 const std::string& ua, bool robots);

bool robots_ok(const std::string& url, uint32_t timeout_s,
               const std::string& ua);

} // namespace pscrape