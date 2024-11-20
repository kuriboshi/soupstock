// soupstock - a soupbintcp library
//
// Copyright 2024 Krister Joas
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <fmt/format.h>
#include <string>
#include <sqlite3.h>

namespace fixme
{
class database
{
public:
  struct row
  {
    int sequence;
    std::string message;
  };

  void open(const std::string& filename)
  {
    if(_db_handle != nullptr)
      return;
    if(auto ec = sqlite3_open(filename.c_str(), &_db_handle); ec != SQLITE_OK)
      throw std::runtime_error(fmt::format("sqlite3 error {}", sqlite3_errmsg(_db_handle)));
    char* error{nullptr};
    auto ec = sqlite3_exec(_db_handle, R"(
create table if not exists input
(sequence integer primary key autoincrement, message text);
create table if not exists output
(sequence integer primary key autoincrement, message text)
)",
      nullptr, nullptr, &error);
    if(ec != SQLITE_OK)
    {
      std::string error_message{error};
      sqlite3_free(error);
      throw std::runtime_error(error_message);
    }
  }

  void store_output(const std::string& msg)
  {
    sqlite3_stmt* stmt{nullptr};
    std::string sql{R"(insert into output (message) values (?))"};
    auto ec = sqlite3_prepare(_db_handle, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if(ec != SQLITE_OK)
      throw std::runtime_error(fmt::format("{}", sqlite3_errmsg(_db_handle)));
    sqlite3_bind_text(stmt, 1, msg.c_str(), -1, SQLITE_STATIC);
    ec = sqlite3_step(stmt);
    if(ec != SQLITE_DONE)
    {
      ec = sqlite3_finalize(stmt);
      throw std::runtime_error(fmt::format("step: {}", sqlite3_errmsg(_db_handle)));
    }
    ec = sqlite3_finalize(stmt);
    if(ec != SQLITE_OK)
      throw std::runtime_error(fmt::format("finalize: {}", sqlite3_errmsg(_db_handle)));
  }

  std::vector<row> load_output(int sequence)
  {
    std::vector<row> rows;
    std::string sql{fmt::format(R"(select * from output where sequence >= {})", sequence)};
    sqlite3_stmt* stmt{nullptr};
    auto ec = sqlite3_prepare(_db_handle, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    while(true)
    {
      ec = sqlite3_step(stmt);
      if(ec == SQLITE_ROW)
        rows.push_back(row{sqlite3_column_int(stmt, 0), reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))});
      else if(ec == SQLITE_DONE)
        break;
    }
    ec = sqlite3_finalize(stmt);
    if(ec != SQLITE_OK)
      throw std::runtime_error(fmt::format("finalize: {}", sqlite3_errmsg(_db_handle)));
    return rows;
  }

  void store_input(const std::string& msg)
  {
    std::string sql{fmt::format(R"(insert into input (message) values (?))")};
    sqlite3_stmt* stmt{nullptr};
    auto ec = sqlite3_prepare(_db_handle, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if(ec != SQLITE_OK)
      throw std::runtime_error(fmt::format("{}", sqlite3_errmsg(_db_handle)));
    sqlite3_bind_text(stmt, 1, msg.c_str(), -1, SQLITE_STATIC);
    ec = sqlite3_step(stmt);
    if(ec != SQLITE_DONE)
    {
      ec = sqlite3_finalize(stmt);
      throw std::runtime_error(fmt::format("step: {}", sqlite3_errmsg(_db_handle)));
    }
    ec = sqlite3_finalize(stmt);
    if(ec != SQLITE_OK)
      throw std::runtime_error(fmt::format("finalize: {}", sqlite3_errmsg(_db_handle)));
  }

  std::vector<row> load_input()
  {
    std::vector<row> rows;
    sqlite3_stmt* stmt{nullptr};
    std::string sql{fmt::format(R"(select * from input where sequence >= {})", 1)};
    auto ec = sqlite3_prepare(_db_handle, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    while(true)
    {
      ec = sqlite3_step(stmt);
      if(ec == SQLITE_ROW)
        rows.push_back(row{sqlite3_column_int(stmt, 0), reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1))});
      else if(ec == SQLITE_DONE)
        break;
    }
    ec = sqlite3_finalize(stmt);
    if(ec != SQLITE_OK)
      throw std::runtime_error(fmt::format("finalize: {}", sqlite3_errmsg(_db_handle)));
    return rows;
  }

private:
  sqlite3* _db_handle{nullptr};
};
} // namespace fixme
