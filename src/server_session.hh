// soupstock - a soupbintcp library
//
// Copyright 2024-2025 Krister Joas
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
#include <chrono>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

using namespace std::literals;

namespace fixme::soupstock
{
template<typename Handler>
class server_session: public base_session
{
public:
  server_session(asio::ip::tcp::socket socket)
    : base_session(std::move(socket)),
      _handler(std::make_unique<Handler>())
  {}

  void send_sequenced(std::string_view msg)
  {
    ++_sequence;
    spdlog::info("{}: sequenced ({}) {}", _session, _sequence, msg);
    _database.store_output(msg);
    dispatch('S', msg);
  }

  void reject_login(std::string_view reason) { dispatch('J', reason); }

  void accept_login(std::string_view session, std::string_view msg)
  {
    _session = session;
    dispatch('A', msg);
    _database.open(fmt::format("server-{}.db", session));
  }

  void replay_sequenced(int sequence)
  {
    _sequence = sequence;
    replay_sequenced();
  }

private:
  void process_message(std::string_view msg) override
  {
    if(msg.empty())
      throw std::runtime_error("empty message");
    switch(msg[0])
    {
      case '+':
        spdlog::info("{}: debug {}", _session, msg.substr(1));
        break;
      case 'L':
        _handler->process_login(*this, msg.substr(1));
        break;
      case 'S':
        break;
      case 'U':
        _handler->process_unsequenced(*this, msg.substr(1));
        break;
      case 'R':
        _timeout.expires_after(15s);
        break;
      case 'O':
        spdlog::info("{}: logout", _session);
        stop();
        break;
      default:
        spdlog::info("unknown packet type: {}", msg[0]);
        break;
    }
  }

  void timer_handler() override { dispatch('H'); }

  void replay_sequenced() { load_messages(); }

  void load_messages()
  {
    auto rows = _database.load_output(_sequence);
    for(const auto& r: rows)
    {
      _sequence = r.sequence;
      spdlog::info("{}: replay ({}) '{}'", _session, _sequence, r.message);
      dispatch('S', r.message);
    }
  }

  std::unique_ptr<Handler> _handler;
  database _database;
};
} // namespace fixme::soupstock
