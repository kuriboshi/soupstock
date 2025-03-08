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
#include <spdlog/spdlog.h>

using namespace std::literals;

namespace fixme::soup
{
class session: public base_session
{
public:
  session(asio::ip::tcp::socket socket)
    : base_session(std::move(socket))
  {}

private:
  void send_sequenced(std::string_view msg)
  {
    _database.store_output(msg);
    spdlog::info("send sequenced: {}", msg);
    dispatch('S', msg);
  }

  void process_login(const std::string_view msg)
  {
    std::string sequence;
    std::tie(_username, _password, _session, sequence) =
      std::tuple(msg.substr(0, 6), msg.substr(6, 10), msg.substr(16, 10), msg.substr(26, 20));
    std::erase(_username, ' ');
    std::erase(_password, ' ');
    std::erase(_session, ' ');
    std::erase(sequence, ' ');
    auto [ptr, ec] = std::from_chars(sequence.data(), sequence.data() + sequence.length(), _sequence);
    if(ec != std::errc{})
    {
      spdlog::info("process_login: {}", std::make_error_code(ec).message());
      dispatch('J', "A");
      return;
    }
    spdlog::info("process_login '{}' '{}' '{}' '{}'", _username, _password, _session, _sequence);
    dispatch('A', msg);
    _database.open(fmt::format("server-{}-{}.db", _username, _session));
    replay_sequenced();
  }

  void process_message(std::string_view msg) override
  {
    if(msg.empty())
      throw std::runtime_error("empty message");
    switch(msg[0])
    {
      case '+':
        spdlog::info("debug {}", msg.substr(1));
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
        _timeout.expires_after(15s);
        break;
      case 'O':
        spdlog::info("logout");
        stop();
        break;
      default:
        spdlog::info("unknown packet type: {}", msg[0]);
        break;
    }
  }

  void process_unsequenced(const std::string_view msg)
  {
    if(msg == "date")
      send_sequenced(fmt::format(
        "{:%Y-%m-%d %H:%M:%S}", std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())));
  }

  void timer_handler() override
  {
    dispatch('H');
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
      spdlog::info("{}", r.message);
      dispatch('S', r.message);
    }
  }

  std::string _username;
  std::string _password;
  database _database;
};
} // namespace fixme::soup
