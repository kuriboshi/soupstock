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

#include "server_handler.hh"
#include "server_session.hh"
#include "util.hh"

#include <fmt/ranges.h>
#include <fmt/std.h>
#include <spdlog/spdlog.h>
#include <system_error>

namespace fixme::soupstock
{
/// @brief Accepts TCP connections, creating server sessions for each connection.
class server
{
public:
  /// @brief Start accepting connections on a specific port.
  ///
  /// @param context The asio::io_context object. Need to create the acceptor.
  /// @param port The port on which the server accepts connections.
  server(asio::io_context& context, short port)
    : _acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
  {
    accept();
  }

private:
  /// @brief Creates a server session when a new connection is accepted.
  void accept()
  {
    _acceptor.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
      if(!ec)
        create_session(std::move(socket));
      else
        spdlog::info("error: {}", ec);
      accept();
    });
  }

  /// @brief Create a server session using the specifed socket.
  ///
  /// @param socket The socket used for bidirectional communication with the client.
  void create_session(asio::ip::tcp::socket socket)
  {
    spdlog::info(
      "creating session on: {}:{}", socket.remote_endpoint().address().to_string(), socket.remote_endpoint().port());
    std::make_shared<soupstock::server_session<server_handler>>(std::move(socket))->run();
  }

  /// @brief The acceptor.
  asio::ip::tcp::acceptor _acceptor;
};
} // namespace fixme::soupstock

int main(int argc, char* argv[])
{
  asio::io_context context;
  fixme::soupstock::server s(context, 25000);
  context.run();
  return 0;
}
