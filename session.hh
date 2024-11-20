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

#include "base_session.hh"
#include "database.hh"
#include <asio.hpp>
#include <charconv>
#include <chrono>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/std.h>

using namespace std::literals;

namespace fixme::soup
{
class session: public std::enable_shared_from_this<session>, public base_session
{
public:
  session(asio::ip::tcp::socket socket)
    : base_session(std::move(socket))
  {}

  void run()
  {
    asio::co_spawn(_socket.get_executor(), [self = shared_from_this()] { return self->reader(); }, asio::detached);
    asio::co_spawn(_socket.get_executor(), [self = shared_from_this()] { return self->writer(); }, asio::detached);
  }

private:
  void send_sequenced(const std::string& msg)
  {
    _database.store_output(msg);
    dispatch(fmt::format("S{}", msg));
  }

  void process_login(const std::string_view msg)
  {
    _username = msg.substr(0, 6);
    std::erase(_username, ' ');
    _password = msg.substr(6, 10);
    std::erase(_password, ' ');
    _session = msg.substr(16, 10);
    std::erase(_session, ' ');
    auto sequence = msg.substr(26, 20);
    auto [ptr, ec] = std::from_chars(sequence.data(), sequence.data() + sequence.length(), _sequence);
    if(ec != std::errc{})
    {
      fmt::println("process_login: {}", std::make_error_code(ec).message());
      dispatch("JA");
      return;
    }
    fmt::println("process_login '{}' '{}' '{}' '{}'", _username, _password, _session, _sequence);
    dispatch(fmt::format("A{}", msg));
    _database.open(fmt::format("server-{}-{}.db", _username, _session));
    replay_sequenced();
  }

  void process_message(const std::string& msg) override
  {
    switch(msg[0])
    {
      case '+':
        fmt::println("debug {}", msg.substr(1));
        break;
      case 'L':
        process_login(msg.substr(1));
        break;
      case 'S':
        break;
      case 'U':
        process_unsequenced(msg.substr(1));
        break;
      case 'R':
        break;
      case 'O':
        fmt::println("logout");
        stop();
        break;
      default:
        fmt::println("unknown packet type: {}", msg[0]);
        break;
    }
  }

  void process_unsequenced(const std::string_view msg)
  {
    if(msg == "date")
      send_sequenced(fmt::format(
        "{:%Y-%m-%d %H:%M:%S}", std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now())));
  }

  void timer_handler(asio::error_code ec) override
  {
    if(ec != asio::error::operation_aborted)
      dispatch("H");
  }

  void replay_sequenced()
  {
    load_messages();
    ++_sequence;
  }

  void load_messages()
  {
    auto rows = _database.load_output(_sequence);
    for(const auto& r: rows)
    {
      _sequence = r.sequence;
      fmt::println("{}", r.message);
      dispatch(fmt::format("S{}", r.message));
    }
  }

  std::string _username;
  std::string _password;
  database _database;
};
} // namespace fixme::soup
