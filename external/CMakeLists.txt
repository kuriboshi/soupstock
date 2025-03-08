# soupstock - a soupbintcp library
#
# Copyright 2024 Krister Joas <krister@joas.jp>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

file(DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d
)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR}/cmake)
include(CPM)

CPMAddPackage("gh:google/googletest@1.14.0")
CPMAddPackage("gh:gabime/spdlog@1.15.1")
CPMAddPackage("gh:chriskohlhoff/asio#asio-1-30-2@1.30.2")
CPMAddPackage("gh:fmtlib/fmt#11.1.3")
CPMAddPackage(NAME sqlite3 URL "https://sqlite.org/2024/sqlite-amalgamation-3460000.zip")

find_package(Threads REQUIRED)

if(asio_ADDED)
  add_library(asio INTERFACE)
  target_include_directories(asio SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)
  target_compile_definitions(asio INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
  target_link_libraries(asio INTERFACE Threads::Threads)
  add_library(asio::asio ALIAS asio)
endif()

if(sqlite3_ADDED)
  add_library(sqlite3 STATIC)
  target_sources(sqlite3 PRIVATE ${sqlite3_SOURCE_DIR}/sqlite3.c)
  target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
  add_library(sqlite3::sqlite3 ALIAS sqlite3)
endif()
