#include "pscrape/cfg.hpp"
#include "pscrape/fetch.hpp"
#include "pscrape/out.hpp"
#include "pscrape/parse.hpp"
#include "pscrape/val.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace pscrape;

static std::vector<std::string> read_sources(const std::string& path) {
  std::vector<std::string> urls;
  std::ifstream f(path);
  if (!f) throw std::runtime_error("cannot open sources: " + path);
  std::string ln;
  while (std::getline(f, ln)) {
    while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ' ||
                           ln.back() == '\t'))
      ln.pop_back();
    size_t a = ln.find_first_not_of(" \t");
    if (a == std::string::npos) continue;
    ln = ln.substr(a);
    if (ln.empty() || ln[0] == '#') continue;
    urls.push_back(ln);
  }
  return urls;
}

int main(int argc, char** argv) {
  try {
    auto oc = parse_args(argc, argv);
    if (!oc) { std::cerr << usage(); return 0; }
    auto c = *oc;
    auto urls = read_sources(c.src_path);
    std::cerr << "sources: " << urls.size() << "\n";
    auto res = fetch_all(urls, c.thr, c.timeout_s, c.ua, c.robots);
    size_t ok = 0;
    std::vector<proxy> all;
    for (auto& r : res) {
      if (r.err.empty() && r.code >= 200 && r.code < 300) {
        ++ok;
        auto ps = parse_body(r.body);
        all.insert(all.end(), ps.begin(), ps.end());
      } else {
        std::cerr << "skip " << r.url << " code=" << r.code
                  << " err=" << r.err << "\n";
      }
    }
    std::cerr << "fetched: " << ok << "/" << res.size() << "\n";
    auto uniq = dedupe(std::move(all));
    std::cerr << "unique proxies: " << uniq.size() << "\n";

    std::vector<val_res> vs;
    if (c.http_probe) {
      vs = validate_http(uniq, c.thr, c.timeout_s, c.probe_url);
    } else if (c.validate) {
      vs = validate_tcp(uniq, c.thr, c.timeout_s);
    } else {
      vs.reserve(uniq.size());
      for (auto& p : uniq) vs.push_back({p, true, -1, ""});
    }

    if (c.validate) {
      size_t live = 0;
      for (auto& v : vs) if (v.ok) ++live;
      std::cerr << "live: " << live << "\n";
    }

    auto data = render(vs, c.fmt);
    std::string err;
    if (!write_file(c.out_path, data, err)) {
      std::cerr << "write err: " << err << "\n";
      return 2;
    }
    std::cerr << "wrote " << c.out_path << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "fatal: " << e.what() << "\n";
    return 1;
  }
}