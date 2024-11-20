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

#include "session.hh"
#include <fmt/format.h>
#include <fmt/std.h>
#include <system_error>

namespace fixme
{
class server
{
public:
  server(asio::io_context& io_context, short port)
    : _acceptor(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
  {
    accept();
  }

private:
  void accept()
  {
    _acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
      if(!ec)
        create_session(std::move(socket));
      else
        fmt::println("error: {}", ec);
      accept();
    });
  }

  void create_session(asio::ip::tcp::socket socket)
  {
    fmt::println(
      "creating session on: {}:{}", socket.remote_endpoint().address().to_string(), socket.remote_endpoint().port());
    std::make_shared<soup::session>(std::move(socket))->run();
  }

  asio::ip::tcp::acceptor _acceptor;
};
} // namespace fixme

int main(int argc, char* argv[])
{
  asio::io_context io_context;
  fixme::server s(io_context, 25000);
  io_context.run();
  return 0;
}
