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

#include "client_handler.hh"
#include "client_session.hh"
#include "util.hh"

namespace
{
template<typename Session>
asio::awaitable<int> read_from_stream(asio::posix::stream_descriptor& stream, Session& session)
{
  static std::regex re_quit{"q(uit)?"};
  static std::regex re_logout{"lo(gout)?"};
  static std::regex re_debug{"debug (.*)"};
  static std::regex re_date{"date"};
  try
  {
    while(true)
    {
      std::string data;
      co_await asio::async_read_until(stream, asio::dynamic_buffer(data), '\n', asio::use_awaitable);
      if(data.length() >= 1)
      {
        data.pop_back();
        std::smatch m;
        if(std::regex_match(data, m, re_quit))
          break;
        if(std::regex_match(data, m, re_logout))
        {
          session.send_logout();
          break;
        }
        if(std::regex_match(data, m, re_debug))
        {
          session.send_debug(m[1].str());
          continue;
        }
        if(std::regex_match(data, m, re_date))
        {
          session.send_unsequenced(data);
          continue;
        }
        spdlog::info("unknown command");
        // send_unsequenced(data);
      }
    }
  }
  catch(const std::exception& ex)
  {
    spdlog::info("exception: {}", ex.what());
    session.close();
    co_return 1;
  }
  co_return 0;
}
} // namespace

int main()
{
  int result{};
  try
  {
    asio::io_context context;
    fixme::soupstock::session_config config{"127.0.0.1", "25000", "user1", "password1", "session1"};
    auto client{std::make_shared<fixme::soupstock::client_session<fixme::soupstock::client_handler>>(context, config)};
    client->run();
    client->send_login();
    asio::posix::stream_descriptor stdin{context, STDIN_FILENO};
    asio::co_spawn(
      context,
      [&] -> asio::awaitable<void> {
        result = co_await read_from_stream(stdin, *client);
        co_return;
      },
      asio::detached);
    context.run();
  }
  catch(const std::exception& ex)
  {
    spdlog::warn("Exception: {}", ex.what());
    result = 1;
  }
  return result;
}
