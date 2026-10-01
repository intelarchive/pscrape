#include "pscrape/parse.hpp"
#include <charconv>
#include <regex>
#include <unordered_set>

namespace pscrape {

static bool port_ok(const std::string& s, uint16_t& out) {
  if (s.empty() || s.size() > 5) return false;
  unsigned v = 0;
  auto r = std::from_chars(s.data(), s.data() + s.size(), v);
  if (r.ec != std::errc{} || r.ptr != s.data() + s.size()) return false;
  if (v == 0 || v > 65535) return false;
  out = (uint16_t)v;
  return true;
}

std::optional<proxy> parse_line(const std::string& ln) {
  std::string s = ln;
  while (!s.empty() && (s.back() == '\r' || s.back() == '\n' ||
                        s.back() == ' ' || s.back() == '\t'))
    s.pop_back();
  size_t a = s.find_first_not_of(" \t");
  if (a == std::string::npos) return std::nullopt;
  s = s.substr(a);
  std::vector<std::string> parts;
  size_t i = 0;
  while (i <= s.size()) {
    auto e = s.find(':', i);
    if (e == std::string::npos) { parts.push_back(s.substr(i)); break; }
    parts.push_back(s.substr(i, e - i));
    i = e + 1;
  }
  if (parts.size() < 2 || parts.size() > 4) return std::nullopt;
  proxy p;
  p.host = parts[0];
  if (!port_ok(parts[1], p.port)) return std::nullopt;
  if (parts.size() >= 3) p.user = parts[2];
  if (parts.size() >= 4) p.pass = parts[3];
  if (p.host.empty()) return std::nullopt;
  bool ip = true;
  int dots = 0;
  for (char c : p.host) {
    if (c == '.') { ++dots; continue; }
    if (c < '0' || c > '9') { ip = false; break; }
  }
  if (!ip || dots != 3) return std::nullopt;
  return p;
}

std::vector<proxy> parse_body(const std::string& body) {
  static const std::regex re4(
    R"((\d{1,3}(?:\.\d{1,3}){3}):(\d{1,5}):([^\s:]{1,64}):([^\s:]{1,64}))");
  static const std::regex re2(
    R"((\d{1,3}(?:\.\d{1,3}){3}):(\d{1,5}))");
  std::vector<proxy> out;
  auto begin = std::sregex_iterator(body.begin(), body.end(), re4);
  auto end = std::sregex_iterator();
  for (auto it = begin; it != end; ++it) {
    proxy p;
    p.host = (*it)[1].str();
    if (!port_ok((*it)[2].str(), p.port)) continue;
    p.user = (*it)[3].str();
    p.pass = (*it)[4].str();
    out.push_back(std::move(p));
  }
  auto b2 = std::sregex_iterator(body.begin(), body.end(), re2);
  for (auto it = b2; it != end; ++it) {
    proxy p;
    p.host = (*it)[1].str();
    if (!port_ok((*it)[2].str(), p.port)) continue;
    out.push_back(std::move(p));
  }
  return out;
}

std::vector<proxy> dedupe(std::vector<proxy> in) {
  std::unordered_set<std::string> seen;
  seen.reserve(in.size() * 2);
  std::vector<proxy> out;
  out.reserve(in.size());
  for (auto& p : in) {
    std::string k = p.host + ":" + std::to_string(p.port);
    if (seen.insert(k).second) out.push_back(std::move(p));
  }
  return out;
}

} // namespace pscrape