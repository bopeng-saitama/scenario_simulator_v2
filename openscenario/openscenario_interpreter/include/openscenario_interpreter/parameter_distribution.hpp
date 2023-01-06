// Copyright 2015 TIER IV, Inc. All rights reserved.
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

#ifndef OPENSCENARIO_INTERPRETER__PARAMETER_DISTRIBUTION_HPP_
#define OPENSCENARIO_INTERPRETER__PARAMETER_DISTRIBUTION_HPP_

#include <unordered_map>
#include <vector>

namespace openscenario_interpreter
{
struct SingleParameterDistributionBase
{
  virtual auto derive() -> std::vector<Object> = 0;
};

struct MultiParameterDistributionBase
{
  virtual auto derive() -> std::vector<std::unordered_map<std::string, Object>> = 0;
};
}  // namespace openscenario_interpreter
#endif  //OPENSCENARIO_INTERPRETER__PARAMETER_DISTRIBUTION_HPP_
