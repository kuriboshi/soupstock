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
#include <chrono>
#include <deque>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <regex>

using namespace std::literals;

namespace fixme::soup
{
struct session_config
{
  std::string host;
  std::string port;
  std::string username;
  std::string password;
  std::string session;
};

class client: public std::enable_shared_from_this<client>, public base_session
{
public:
  client(asio::io_context& context, const session_config& config)
    : base_session(asio::ip::tcp::socket{context}, config.session),
      _host(config.host),
      _port(config.port),
      _username(config.username),
      _password(config.password),
      _stdin(context, STDIN_FILENO),
      _resolver(context)
  {}

  asio::awaitable<void> run()
  {
    _database.open(fmt::format("client-{}-{}.db", _username, _session));
    load_messages();
    ++_sequence;
    send_login();
    co_await stdin();
  }

private:
  asio::awaitable<void> stdin()
  {
    static std::regex re_quit{"q(uit)?"};
    static std::regex re_logout{"lo(gout)?"};
    static std::regex re_debug{"debug (.*)"};
    static std::regex re_date{"date"};
    while(true)
    {
      std::string data;
      co_await asio::async_read_until(_stdin, asio::dynamic_buffer(data), '\n', asio::use_awaitable);
      if(data.length() >= 1)
      {
        data.pop_back();
        std::smatch m;
        if(std::regex_match(data, m, re_quit))
          break;
        if(std::regex_match(data, m, re_logout))
        {
          send_logout();
          break;
        }
        if(std::regex_match(data, m, re_debug))
        {
          send_debug(m[1].str());
          continue;
        }
        if(std::regex_match(data, m, re_date))
        {
          send_unsequenced(data);
          continue;
        }
        fmt::println("unknown command");
        // send_unsequenced(data);
      }
    }

    stop();
  }

  void load_messages()
  {
    auto rows = _database.load_input();
    for(const auto& r: rows)
    {
      _sequence = r.sequence;
      fmt::println("{}", r.message);
    }
  }

  void send_login()
  {
    auto msg = fmt::format("L{:<6s}{:<10s}{:<10s}{:<20d}", _username, _password, _session, _sequence);
    dispatch(msg);
    asio::connect(_socket, _resolver.resolve(_host, _port));
    asio::co_spawn(_socket.get_executor(), [self = shared_from_this()] { return self->writer(); }, asio::detached);
    asio::co_spawn(_socket.get_executor(), [self = shared_from_this()] { return self->reader(); }, asio::detached);
  }

  void send_logout()
  {
    auto msg = fmt::format("O");
    dispatch(msg);
  }

  void send_debug(const std::string& data)
  {
    auto msg = fmt::format("+{}", data);
    dispatch(msg);
  }

  void send_unsequenced(const std::string& data)
  {
    auto msg = fmt::format("U{}", data);
    dispatch(msg);
  }

  void process_sequenced(const std::string& msg)
  {
    fmt::println("{}", msg);
    _database.store_input(msg);
  }

  void process_message(const std::string& msg) override
  {
    switch(msg[0])
    {
      case '+':
        fmt::println("debug {}", msg.substr(1));
        break;
      case 'J':
        fmt::println("login rejected {}", msg.substr(2, 1));
        break;
      case 'A':
        fmt::println("login accept '{}' '{}' '{}' '{}'", msg.substr(1, 6), msg.substr(7, 10), msg.substr(17, 10),
          msg.substr(27, 20));
        break;
      case 'S':
        process_sequenced(msg.substr(1));
        break;
      case 'H':
        break;
      default:
        fmt::println("unknown packet type: {}", msg[0]);
        break;
    }
  }

  void timer_handler(asio::error_code ec) override
  {
    if(ec != asio::error::operation_aborted)
      _messages.push_back("R");
  }

  std::string _host;
  std::string _port;
  std::string _username;
  std::string _password;
  asio::posix::stream_descriptor _stdin;
  asio::ip::tcp::resolver _resolver;
  database _database;
};
} // namespace fixme::soup
