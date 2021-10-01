// Copyright 2015-2021 Tier IV, Inc. All rights reserved.
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

#include <openscenario_interpreter/syntax/traffic_signal_controller_action.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
auto TrafficSignalControllerAction::accomplished() noexcept -> bool { return true; }

auto TrafficSignalControllerAction::endsImmediately() noexcept -> bool { return true; }

auto TrafficSignalControllerAction::run() -> void
{
  auto found = localScope().findElement(traffic_signal_controller_ref);
  if (found and found.is<TrafficSignalController>()) {
    found.as<TrafficSignalController>().changePhaseByName(phase);
  } else {
    THROW_SYNTAX_ERROR(
      "TrafficSignalController ", std::quoted(traffic_signal_controller_ref),
      " is not declared in this scope");
  }
}

auto TrafficSignalControllerAction::start() noexcept -> void {}
}  // namespace syntax
}  // namespace openscenario_interpreter
