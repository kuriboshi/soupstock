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

#include "client_session.hh"

int main()
{
  asio::io_context context;
  fixme::soup::session_config config{"127.0.0.1", "25000", "user", "password", "session"};
  auto client{std::make_shared<fixme::soup::client>(context, config)};
  asio::co_spawn(context, client->run(), asio::detached);
  context.run();
  return 0;
}
