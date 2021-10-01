// Copyright 2015-2020 Tier IV, Inc. All rights reserved.
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

#ifndef OPENSCENARIO_INTERPRETER__SYNTAX__TRAFFIC_SIGNAL_CONDITION_HPP_
#define OPENSCENARIO_INTERPRETER__SYNTAX__TRAFFIC_SIGNAL_CONDITION_HPP_

#include <openscenario_interpreter/reader/attribute.hpp>
#include <openscenario_interpreter/scope.hpp>
#include <openscenario_interpreter/syntax/arrow.hpp>
#include <openscenario_interpreter/syntax/color.hpp>
#include <openscenario_interpreter/syntax/string.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
/* ---- TrafficSignalCondition -------------------------------------------------
 *
 *  Considered true if a referenced traffic signal (e.g. from an OpenDRIVE
 *  file) reaches a specific states. Signal IDs are listed in the TrafficSignal
 *  list of the RoadNetwork together with their states and their controllers to
 *  enable dynamic signal modelling.
 *
 *  <xsd:complexType name="TrafficSignalCondition">
 *    <xsd:attribute name="name" type="String" use="required"/>
 *    <xsd:attribute name="state" type="String" use="required"/>
 *  </xsd:complexType>
 *
 * -------------------------------------------------------------------------- */
struct TrafficSignalCondition
{
  const String name;

  const String state;

  template <typename Node>
  explicit TrafficSignalCondition(const Node & node, Scope & scope)
  : name(readAttribute<String>("name", node, scope)),
    state(readAttribute<String>("state", node, scope))
  {
  }

  Arrow current_arrow;  // for description

  Color current_color;  // for description

  auto description() const -> String;

  auto evaluate() -> Element;
};
}  // namespace syntax
}  // namespace openscenario_interpreter

#endif  // OPENSCENARIO_INTERPRETER__SYNTAX__TRAFFIC_SIGNAL_CONDITION_HPP_
