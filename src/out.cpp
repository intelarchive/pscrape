#include "pscrape/out.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace pscrape {

static std::string esc_json(const std::string& s) {
  std::string o;
  o.reserve(s.size() + 2);
  for (char c : s) {
    switch (c) {
      case '"': o += "\\\""; break;
      case '\\': o += "\\\\"; break;
      case '\n': o += "\\n"; break;
      case '\r': o += "\\r"; break;
      case '\t': o += "\\t"; break;
      default:
        if ((unsigned char)c < 0x20) {
          char b[8];
          std::snprintf(b, sizeof(b), "\\u%04x", c);
          o += b;
        } else o += c;
    }
  }
  return o;
}

std::string render(const std::vector<val_res>& vs, const std::string& fmt) {
  std::ostringstream o;
  if (fmt == "json") {
    o << "[";
    bool first = true;
    for (const auto& v : vs) {
      if (!v.ok) continue;
      if (!first) o << ",";
      first = false;
      o << "{\"host\":\"" << esc_json(v.p.host) << "\","
        << "\"port\":" << v.p.port << ","
        << "\"latency_ms\":" << v.latency_ms;
      if (!v.p.user.empty())
        o << ",\"user\":\"" << esc_json(v.p.user) << "\","
          << "\"pass\":\"" << esc_json(v.p.pass) << "\"";
      o << "}";
    }
    o << "]\n";
    return o.str();
  }
  for (const auto& v : vs) {
    if (!v.ok) continue;
    if (fmt == "auth" && !v.p.user.empty()) {
      o << v.p.host << ":" << v.p.port << ":"
        << v.p.user << ":" << v.p.pass << "\n";
    } else {
      o << v.p.host << ":" << v.p.port << "\n";
    }
  }
  return o.str();
}

bool write_file(const std::string& path, const std::string& data,
                std::string& err) {
  namespace fs = std::filesystem;
  fs::path p(path);
  fs::path tmp = p;
  tmp += ".tmp";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f) { err = "open tmp"; return false; }
    f.write(data.data(), (std::streamsize)data.size());
    if (!f) { err = "write tmp"; return false; }
  }
  std::error_code ec;
  fs::rename(tmp, p, ec);
  if (ec) {
    fs::remove(tmp, ec);
    err = "rename: " + ec.message();
    return false;
  }
  return true;
}

} // namespace pscrape