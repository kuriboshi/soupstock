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
#include <spdlog/spdlog.h>

using namespace std::literals;

namespace fixme
{
class base_session: public std::enable_shared_from_this<base_session>
{
public:
  base_session(asio::ip::tcp::socket socket)
    : _socket(std::move(socket)),
      _strand(asio::make_strand(_socket.get_executor())),
      _timer(_strand),
      _timeout(_strand)
  {}
  base_session(asio::ip::tcp::socket socket, std::string session)
    : _socket(std::move(socket)),
      _strand(asio::make_strand(_socket.get_executor())),
      _timer(_strand),
      _timeout(_strand),
      _session(std::move(session))
  {}
  virtual ~base_session() = default;

  void run()
  {
    auto self = shared_from_this();
    asio::co_spawn(_strand, [self] { return self->reader(); }, asio::detached);
    asio::co_spawn(_strand, [self] { return self->writer(); }, asio::detached);
    asio::co_spawn(_strand, [self] { return self->timer(); }, asio::detached);
    asio::co_spawn(_strand, [self] { return self->timeout(); }, asio::detached);
  }

protected:
  asio::awaitable<void> reader()
  {
    try
    {
      while(_socket.is_open())
      {
        std::int16_t length;
        co_await asio::async_read(_socket, asio::buffer(&length, sizeof(length)), asio::use_awaitable);
        length = ntohs(length);
        std::string msg;
        co_await asio::async_read(_socket, asio::dynamic_buffer(msg, length), asio::use_awaitable);
        _timeout.expires_after(15s);
        process_message(msg);
      }
    }
    catch(const std::exception& ex)
    {
      spdlog::info("session '{}' closed: {}", _session, ex.what());
      stop();
    }
  }

  asio::awaitable<void> writer()
  {
    try
    {
      while(_socket.is_open() && !_messages.empty())
      {
        auto data = _messages.front();
        std::int16_t length = data.length();
        length = htons(length);
        co_await asio::async_write(_socket, asio::buffer(&length, sizeof(length)), asio::use_awaitable);
        co_await asio::async_write(_socket, asio::buffer(data), asio::use_awaitable);
        _messages.pop_front();
      }
      _timer.expires_after(1s);
    }
    catch(const std::exception& ex)
    {
      spdlog::info("writer: {}", ex.what());
      stop();
    }
  }

  asio::awaitable<void> timer()
  {
    try
    {
      _timer.expires_after(1s);
      while(_socket.is_open())
      {
        asio::error_code ec;
        co_await _timer.async_wait(asio::redirect_error(asio::use_awaitable, ec));
        if(ec != asio::error::operation_aborted)
        {
          _timer.expires_after(1s);
          timer_handler();
        }
      }
    }
    catch(const std::exception& ex)
    {
      spdlog::info("{}", ex.what());
      stop();
    }
  }

  asio::awaitable<void> timeout()
  {
    try
    {
      _timeout.expires_after(15s);
      while(_socket.is_open())
      {
        asio::error_code ec;
        co_await _timeout.async_wait(asio::redirect_error(asio::use_awaitable, ec));
        if(ec != asio::error::operation_aborted)
        {
          spdlog::info("Timeout");
          stop();
        }
      }
    }
    catch(const std::exception& ex)
    {
      spdlog::info("{}", ex.what());
      stop();
    }
  }

  void stop()
  {
    std::error_code ec;
    _socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    _socket.close();
    _timer.cancel();
    _timeout.cancel();
  }

  void dispatch(char message_type, std::string_view data = {})
  {
    asio::post(_strand, [this, message_type, msg = std::string(data)]() {
      _messages.push_back(fmt::format("{}{}", message_type, msg));
      if (_messages.size() == 1) {
        auto self = shared_from_this();
        asio::co_spawn(_strand, [self] { return self->writer(); }, asio::detached);
      }
    });
  }

  asio::ip::tcp::socket _socket;
  asio::strand<asio::any_io_executor> _strand;
  asio::steady_timer _timer;
  asio::steady_timer _timeout;
  std::string _session;

  std::deque<std::string> _messages;
  int _sequence;

private:
  virtual void process_message(const std::string& msg) = 0;
  virtual void timer_handler() = 0;
};
} // namespace fixme
