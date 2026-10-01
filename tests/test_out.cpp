#include "doctest.h"
#include "pscrape/out.hpp"
#include <filesystem>
#include <fstream>

using namespace pscrape;

TEST_CASE("render plain") {
  std::vector<val_res> vs = {
    {{"1.1.1.1", 80, "", ""}, true, 12, ""},
    {{"2.2.2.2", 80, "", ""}, false, -1, "x"},
  };
  auto s = render(vs, "plain");
  CHECK(s == "1.1.1.1:80\n");
}

TEST_CASE("render auth") {
  std::vector<val_res> vs = {
    {{"1.1.1.1", 80, "u", "p"}, true, 12, ""},
  };
  CHECK(render(vs, "auth") == "1.1.1.1:80:u:p\n");
}

TEST_CASE("render json") {
  std::vector<val_res> vs = {
    {{"1.1.1.1", 80, "u", "p"}, true, 12, ""},
  };
  auto s = render(vs, "json");
  CHECK(s.find("\"host\":\"1.1.1.1\"") != std::string::npos);
  CHECK(s.find("\"latency_ms\":12") != std::string::npos);
}

TEST_CASE("write atomic") {
  namespace fs = std::filesystem;
  auto d = fs::temp_directory_path() / "pscrape_test";
  fs::create_directories(d);
  auto f = d / "out.txt";
  std::string err;
  CHECK(write_file(f.string(), "hi\n", err));
  std::ifstream in(f);
  std::string s;
  std::getline(in, s);
  CHECK(s == "hi");
  fs::remove_all(d);
}