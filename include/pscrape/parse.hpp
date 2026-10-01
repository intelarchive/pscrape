#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pscrape {

struct proxy {
  std::string host;
  uint16_t port = 0;
  std::string user;
  std::string pass;
};

std::vector<proxy> parse_body(const std::string& body);
std::vector<proxy> dedupe(std::vector<proxy> in);
std::optional<proxy> parse_line(const std::string& ln);

} // namespace pscrape