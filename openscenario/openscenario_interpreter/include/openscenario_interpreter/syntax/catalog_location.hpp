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

#ifndef OPENSCENARIO_INTERPRETER__SYNTAX__CATALOG_LOCATION_HPP_
#define OPENSCENARIO_INTERPRETER__SYNTAX__CATALOG_LOCATION_HPP_

#include <openscenario_interpreter/reader/element.hpp>
#include <openscenario_interpreter/syntax/catalog.hpp>
#include <openscenario_interpreter/syntax/directory.hpp>

#include <boost/filesystem.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
/* ---- CatalogLocation --------------------------------------------------------
 *
 *  <xsd:complexType name="CatalogLocations">
 *    <xsd:all>
 *      <xsd:element name="VehicleCatalog" type="VehicleCatalogLocation" minOccurs="0"/>
 *      <xsd:element name="ControllerCatalog" type="ControllerCatalogLocation" minOccurs="0"/>
 *      <xsd:element name="PedestrianCatalog" type="PedestrianCatalogLocation" minOccurs="0"/>
 *      <xsd:element name="MiscObjectCatalog" type="MiscObjectCatalogLocation" minOccurs="0"/>
 *      <xsd:element name="EnvironmentCatalog" type="EnvironmentCatalogLocation" minOccurs="0"/>
 *      <xsd:element name="ManeuverCatalog" type="ManeuverCatalogLocation" minOccurs="0"/>
 *      <xsd:element name="TrajectoryCatalog" type="TrajectoryCatalogLocation" minOccurs="0"/>
 *      <xsd:element name="RouteCatalog" type="RouteCatalogLocation" minOccurs="0"/>
 *    </xsd:all>
 *  </xsd:complexType>
 *
 * -------------------------------------------------------------------------- */
struct CatalogLocation
{
  template <typename Node, typename Scope>
  explicit CatalogLocation(const Node & node, Scope & outer_scope)
  : directory(readElement<Directory>("Directory", node, outer_scope))
  {
  }

  const Directory directory;
  std::unordered_map<std::string, std::string> catalog_files;
};
}  // namespace syntax
}  // namespace openscenario_interpreter

#endif  // OPENSCENARIO_INTERPRETER__SYNTAX__CATALOG_LOCATION_HPP_
