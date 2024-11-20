# soupstock - a soupbintcp library
#
# Copyright 2024 Krister Joas
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

all: debug

.PHONY: cmake_debug
build/debug/configured:
	cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja -B build/debug
	touch build/debug/configured

.PHONY: cmake_release
build/release/configured:
	cmake -DCMAKE_BUILD_TYPE=Release -G Ninja -B build/release
	touch build/release/configured

.PHONY: debug
debug: build/debug/configured
	cmake --build build/debug

.PHONY: release
release: build/release/configured
	cmake --build build/release

.PHONY: xcode
xcode:
	cmake -Wno-dev -G Xcode -B build/xcode

.PHONY: test
test: debug
	ctest --preset debug
