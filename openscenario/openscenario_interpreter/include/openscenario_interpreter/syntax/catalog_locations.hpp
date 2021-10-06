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

#ifndef OPENSCENARIO_INTERPRETER__SYNTAX__CATALOG_LOCATIONS_HPP_
#define OPENSCENARIO_INTERPRETER__SYNTAX__CATALOG_LOCATIONS_HPP_

#include <openscenario_interpreter/syntax/catalog_location.hpp>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace openscenario_interpreter
{
inline namespace syntax
{
/* ---- CatalogLocations -------------------------------------------------------
 *
 *  <xsd:complexType name="CatalogLocations">
 *    <xsd:all>
 *      <xsd:element name="VehicleCatalog" minOccurs="0" type="VehicleCatalogLocation"/>
 *      <xsd:element name="ControllerCatalog" minOccurs="0" type="ControllerCatalogLocation"/>
 *      <xsd:element name="PedestrianCatalog" minOccurs="0" type="PedestrianCatalogLocation"/>
 *      <xsd:element name="MiscObjectCatalog" minOccurs="0" type="MiscObjectCatalogLocation"/>
 *      <xsd:element name="EnvironmentCatalog" minOccurs="0" type="EnvironmentCatalogLocation"/>
 *      <xsd:element name="ManeuverCatalog" minOccurs="0" type="ManeuverCatalogLocation"/>
 *      <xsd:element name="TrajectoryCatalog" minOccurs="0" type="TrajectoryCatalogLocation"/>
 *      <xsd:element name="RouteCatalog" minOccurs="0" type="RouteCatalogLocation"/>
 *    </xsd:all>
 *  </xsd:complexType>
 *
 * -------------------------------------------------------------------------- */

struct CatalogLocations : public std::unordered_map<String, CatalogLocation>
{
  explicit CatalogLocations() = default;

  template <typename Node, typename Scope>
  explicit CatalogLocations(const Node & node, Scope & outer_scope)
  {
#define ELEMENT(TYPE)                                                   \
  callWithElements(node, #TYPE "Catalog", 0, 1, [&](auto && node) {     \
    emplace(                                                            \
      std::piecewise_construct, std::forward_as_tuple(#TYPE "Catalog"), \
      std::forward_as_tuple(node, outer_scope));                        \
  })
    ELEMENT(Vehicle);
    ELEMENT(Controller);
    ELEMENT(Pedestrian);
    ELEMENT(MiscObject);
    ELEMENT(Environment);
    ELEMENT(Maneuver);
    ELEMENT(Trajectory);
    ELEMENT(Route);
#undef ELEMENT

    outer_scope.global().catalog_locations = *this;
  }
};

}  // namespace syntax
}  // namespace openscenario_interpreter

#endif  // OPENSCENARIO_INTERPRETER__SYNTAX__CATALOG_LOCATIONS_HPP_
