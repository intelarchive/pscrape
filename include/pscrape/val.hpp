#pragma once
#include "parse.hpp"
#include <cstdint>
#include <vector>

namespace pscrape {

struct val_res {
  proxy p;
  bool ok = false;
  int latency_ms = -1;
  std::string err;
};

std::vector<val_res> validate_tcp(const std::vector<proxy>& ps,
                                  uint32_t thr, uint32_t timeout_s);

std::vector<val_res> validate_http(const std::vector<proxy>& ps,
                                   uint32_t thr, uint32_t timeout_s,
                                   const std::string& probe_url);

} // namespace pscrape