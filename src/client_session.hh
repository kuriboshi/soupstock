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
#include "util.hh"

#include <asio.hpp>
#include <chrono>
#include <deque>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <regex>
#include <spdlog/spdlog.h>

using namespace std::literals;

namespace fixme::soupstock
{
struct session_config
{
  std::string host;
  std::string port;
  std::string username;
  std::string password;
  std::string session;
};

template<typename Handler>
class client_session: public base_session
{
public:
  client_session(asio::io_context& context, const session_config& config)
    : base_session(asio::ip::tcp::socket{context}, config.session),
      _handler(std::make_unique<Handler>()),
      _host(config.host),
      _port(config.port),
      _username(config.username),
      _password(config.password),
      _resolver(context)
  {}

  void run()
  {
    auto self = std::static_pointer_cast<client_session>(shared_from_this());
    asio::co_spawn(_socket.get_executor(), [self] { return self->reader(); }, asio::detached);
    asio::co_spawn(_socket.get_executor(), [self] { return self->timer(); }, asio::detached);
    asio::co_spawn(_socket.get_executor(), [self] { return self->timeout(); }, asio::detached);
  }

  void close() { stop(); }

  void send_login()
  {
    _database.open(fmt::format("client-{}-{}.db", _username, _session));
    load_messages();
    ++_sequence;
    auto msg = fmt::format("{:<6s}{:<10s}{:<10s}{:<20d}", _username, _password, _session, _sequence);
    dispatch('L', msg);
    asio::connect(_socket, _resolver.resolve(_host, _port));
  }

  void send_logout() { dispatch('O'); }

  void send_debug(std::string_view data) { dispatch('+', data); }

  void send_unsequenced(std::string_view data) { dispatch('U', data); }

private:
  void load_messages()
  {
    auto rows = _database.load_input();
    for(const auto& r: rows)
    {
      _sequence = r.sequence;
      spdlog::info("load: ({}) {}", _sequence, r.message);
    }
  }

  void process_sequenced(std::string_view msg)
  {
    _database.store_input(msg);
    _handler->process_sequenced(*this, msg);
    ++_sequence;
  }

  void process_message(std::string_view msg) override
  {
    switch(msg[0])
    {
      case '+':
        spdlog::info("debug {}", msg.substr(1));
        break;
      case 'J':
        spdlog::info("login rejected {}", msg.substr(1, 1));
        stop();
        break;
      case 'A':
        spdlog::info("login accept {}", std::tuple(trim(msg.substr(1, 10)), trim(msg.substr(11, 20))));
        break;
      case 'U':
        break;
      case 'S':
        process_sequenced(msg.substr(1));
        break;
      case 'H':
        _timeout.expires_after(15s);
        break;
      default:
        spdlog::info("unknown packet type: {}", msg[0]);
        break;
    }
  }

  void timer_handler() override { dispatch('R'); }

  std::unique_ptr<Handler> _handler;
  std::string _host;
  std::string _port;
  std::string _username;
  std::string _password;
  asio::ip::tcp::resolver _resolver;
  database _database;
};
} // namespace fixme::soupstock
