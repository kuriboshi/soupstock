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

#include <asio.hpp>
#include <deque>
#include <fmt/format.h>

using namespace std::literals;

namespace fixme
{
class base_session
{
public:
  base_session(asio::ip::tcp::socket socket)
    : _socket(std::move(socket)),
      _timer(_socket.get_executor())
  {}
  base_session(asio::ip::tcp::socket socket, std::string session)
    : _socket(std::move(socket)),
      _timer(_socket.get_executor()),
      _session(std::move(session))
  {}
  virtual ~base_session() = default;

protected:
  asio::awaitable<void> reader()
  {
    try
    {
      for(;;)
      {
        std::int16_t length;
        co_await asio::async_read(_socket, asio::buffer(&length, sizeof(length)), asio::use_awaitable);
        length = ntohs(length);
        std::string msg;
        co_await asio::async_read(_socket, asio::dynamic_buffer(msg, length), asio::use_awaitable);
        process_message(msg);
      }
    }
    catch(const std::exception& ex)
    {
      fmt::println("session '{}' closed: {}", _session, ex.what());
      stop();
    }
  }

  asio::awaitable<void> writer()
  {
    try
    {
      while(_socket.is_open())
      {
        if(_messages.empty())
        {
          _timer.expires_after(1s);
          asio::error_code ec;
          co_await _timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
          timer_handler(ec);
        }
        else
        {
          auto data = _messages.front();
          std::int16_t length = data.length();
          length = htons(length);
          co_await asio::async_write(_socket, asio::buffer(&length, sizeof(length)), asio::use_awaitable);
          co_await asio::async_write(_socket, asio::buffer(data), asio::use_awaitable);
          _messages.pop_front();
          _timer.expires_after(1s);
        }
      }
    }
    catch(const std::exception& ex)
    {
      fmt::println("writer: {}", ex.what());
      stop();
    }
  }

  void stop()
  {
    std::error_code ec;
    _socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    _socket.close();
    _timer.cancel();
  }

  void dispatch(const std::string& data)
  {
    _messages.push_back(data);
    _timer.cancel_one();
  }

  asio::ip::tcp::socket _socket;
  asio::steady_timer _timer;
  std::string _session;

  std::deque<std::string> _messages;
  int _sequence;

private:
  virtual void process_message(const std::string& msg) = 0;
  virtual void timer_handler(asio::error_code ec) = 0;
};
} // namespace fixme
