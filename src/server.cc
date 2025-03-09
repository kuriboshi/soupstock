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
#include <unordered_map>
#include <unordered_set>

namespace fixme::soupstock
{
class authenticator
{
public:
  struct string_view_hash
  {
    using is_transparent = void;
    std::size_t operator()(std::string_view sv) const { return std::hash<std::string_view>{}(sv); }
  };

  struct string_view_equal
  {
    using is_transparent = void;
    bool operator()(std::string_view lhs, std::string_view rhs) const { return lhs == rhs; }
  };

  bool authenticate(std::string_view username, std::string_view password, std::string_view sessionName)
  {
    if(auto session{_user_sessions.find(username)};
      session != _user_sessions.end() && session->second.contains(sessionName))
    {
      auto user{_users.find(username)};
      return user != _users.end() && user->second == password;
    }
    return false;
  }

  void add_user(std::string_view user, std::string_view password) { _users.try_emplace(std::string(user), password); }
  void add_session(std::string_view user, std::string_view session)
  {
    auto place = _user_sessions.try_emplace(std::string(user)).first;
    place->second.emplace(session);
  }

private:
  std::unordered_map<std::string, std::string, string_view_hash, string_view_equal> _users;
  std::unordered_map<std::string, std::unordered_set<std::string, string_view_hash, string_view_equal>,
    string_view_hash, string_view_equal>
    _user_sessions;
};

/// @brief Accepts TCP connections, creating server sessions for each connection.
class server
{
public:
  /// @brief Start accepting connections on a specific port.
  ///
  /// @param context The asio::io_context object. Need to create the acceptor.
  /// @param port The port on which the server accepts connections.
  server(std::shared_ptr<authenticator> authenticator, asio::io_context& context, short port)
    : _acceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      _authenticator(std::move(authenticator))
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
    std::make_shared<soupstock::server_session<server_handler, authenticator>>(std::move(socket), _authenticator)
      ->run();
  }

  /// @brief The acceptor.
  asio::ip::tcp::acceptor _acceptor;
  std::shared_ptr<authenticator> _authenticator;
};
} // namespace fixme::soupstock

int main(int argc, char* argv[])
{
  try
  {
    asio::io_context context;
    auto authenticator{std::make_shared<fixme::soupstock::authenticator>()};
    authenticator->add_user("user1", "password1");
    authenticator->add_session("user1", "session1");
    fixme::soupstock::server s(std::move(authenticator), context, 25000);
    context.run();
  }
  catch(const std::exception& ex)
  {
    spdlog::warn("{}", ex.what());
    return 1;
  }
  return 0;
}
