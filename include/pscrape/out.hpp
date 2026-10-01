#pragma once
#include "parse.hpp"
#include "val.hpp"
#include <string>
#include <vector>

namespace pscrape {

std::string render(const std::vector<val_res>& vs, const std::string& fmt);

bool write_file(const std::string& path, const std::string& data,
                std::string& err);

} // namespace pscrape