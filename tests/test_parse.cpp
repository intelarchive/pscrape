#include "doctest.h"
#include "pscrape/parse.hpp"

using namespace pscrape;

TEST_CASE("parse_line basic") {
  auto p = parse_line("1.2.3.4:8080");
  REQUIRE(p);
  CHECK(p->host == "1.2.3.4");
  CHECK(p->port == 8080);
  CHECK(p->user.empty());
}

TEST_CASE("parse_line auth") {
  auto p = parse_line("1.2.3.4:8080:alice:s3cret");
  REQUIRE(p);
  CHECK(p->user == "alice");
  CHECK(p->pass == "s3cret");
}

TEST_CASE("parse_line bad port") {
  CHECK(!parse_line("1.2.3.4:0"));
  CHECK(!parse_line("1.2.3.4:99999"));
  CHECK(!parse_line("1.2.3.4:abc"));
}

TEST_CASE("parse_line hostname rejected") {
  CHECK(!parse_line("example.com:80"));
}

TEST_CASE("parse_body mixed") {
  std::string b =
    "junk 1.1.1.1:80 more 2.2.2.2:443:u:p\n"
    "dup 1.1.1.1:80 3.3.3.3:1080\n";
  auto ps = parse_body(b);
  auto u = dedupe(ps);
  CHECK(u.size() == 3);
}

TEST_CASE("parse_body boundary ip") {
  auto ps = parse_body("999.999.999.999:80\n1.2.3.4:65535\n");
  bool found = false;
  for (auto& p : ps) if (p.host == "1.2.3.4" && p.port == 65535) found = true;
  CHECK(found);
}