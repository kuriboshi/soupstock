# soupstock - a soupbintcp library
#
# Copyright 2024 Krister Joas
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

add_library(soupstock INTERFACE)
target_sources(soupstock INTERFACE
  base_session.hh
  client_session.hh
  database.hh
  server_session.hh
)
target_link_libraries(soupstock INTERFACE asio::asio)
target_link_libraries(soupstock INTERFACE sqlite3::sqlite3)
target_link_libraries(soupstock INTERFACE fmt::fmt)
target_link_libraries(soupstock INTERFACE spdlog::spdlog)
target_compile_definitions(soupstock INTERFACE SPDLOG_FMT_EXTERNAL)
add_library(soupstock::soupstock ALIAS soupstock)

add_executable(server)
target_sources(server PRIVATE server.cc)
target_link_libraries(server PRIVATE soupstock::soupstock)

add_executable(client)
target_sources(client PRIVATE client.cc)
target_link_libraries(client PRIVATE soupstock::soupstock)
