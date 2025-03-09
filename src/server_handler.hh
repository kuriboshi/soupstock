// soupstock - a soupbintcp library
//
// Copyright 2025 Krister Joas
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

#include "util.hh"

#include <charconv>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <tuple>

namespace fixme::soupstock
{
template<typename Authenticator>
class server_handler
{
  std::shared_ptr<Authenticator> _authenticator;

public:
  server_handler(std::shared_ptr<Authenticator> authenticator)
    : _authenticator(std::move(authenticator))
  {}

  template<typename Session>
  void process_login(Session& session, const std::string_view msg)
  {
    auto [username, password, sessionName, sequenceNumber] =
      std::tuple(trim(msg.substr(0, 6)), trim(msg.substr(6, 10)), trim(msg.substr(16, 10)), trim(msg.substr(26, 20)));
    int sequence;
    auto [ptr, ec] = std::from_chars(sequenceNumber.data(), sequenceNumber.data() + sequenceNumber.length(), sequence);
    if(ec != std::errc{})
    {
      spdlog::info("reject login {}: {}", std::tuple(username, password, sessionName, sequenceNumber),
        std::make_error_code(ec).message());
      return session.reject_login("A");
    }
    if(!_authenticator->authenticate(username, password, sessionName))
    {
      spdlog::info("reject login {}: {}", std::tuple(username, password, sessionName, sequenceNumber),
        std::make_error_code(ec).message());
      return session.reject_login("A");
    }
    _session = sessionName;
    spdlog::info("{}: accept login {}", _session, std::tuple(username, password, sessionName, sequenceNumber));
    session.accept_login(_session, fmt::format("{:>10}{:>20}", sessionName, sequence));
    session.replay_sequenced(sequence);
    return;
  }

  template<typename Session>
  void process_unsequenced(Session& session, const std::string_view msg)
  {
    if(msg == "date")
      session.send_sequenced(fmt::format("{:%Y-%m-%d %H:%M:%S}",
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())));
  }

private:
  std::string _session;
};
} // namespace fixme::soupstock
