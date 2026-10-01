cmake_minimum_required(VERSION 3.24...4.0)
project(pscrape LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

add_library(pscrape_lib
  src/cfg.cpp src/fetch.cpp src/parse.cpp src/val.cpp src/out.cpp)

# both include/ and include/pscrape so "parse.hpp" and "pscrape/parse.hpp"
# both resolve, but internal includes use the short form
target_include_directories(pscrape_lib PUBLIC include)
target_include_directories(pscrape_lib PRIVATE include/pscrape)
target_link_libraries(pscrape_lib PUBLIC CURL::libcurl Threads::Threads)

if(MSVC)
  target_compile_options(pscrape_lib PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(pscrape_lib PRIVATE -Wall -Wextra -Werror)
endif()

add_executable(pscrape src/main.cpp)
target_link_libraries(pscrape PRIVATE pscrape_lib)

include(FetchContent)
FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.12)
FetchContent_MakeAvailable(doctest)

add_executable(test_pscrape
  tests/test_parse.cpp tests/test_val.cpp tests/test_out.cpp)
target_link_libraries(test_pscrape PRIVATE pscrape_lib doctest::doctest)

enable_testing()
add_test(NAME test_pscrape COMMAND test_pscrape)