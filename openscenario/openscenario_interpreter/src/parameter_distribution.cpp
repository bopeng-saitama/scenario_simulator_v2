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

#include <openscenario_interpreter/parameter_distribution.hpp>

namespace openscenario_interpreter
{
auto openscenario_interpreter::mergeParameterDistributionImpl(
  const ParameterDistribution & distribution, std::string single_parameter_name,
  SingleParameterDistribution && single_distribution) -> ParameterDistribution
{
  ParameterDistribution merged_distribution;
  merged_distribution.reserve(distribution.size() * single_distribution.size());

  for (const auto & single_parameter_value : single_distribution) {
    for (const auto & list : distribution) {
      merged_distribution.emplace_back(list);
      merged_distribution.back()->at(single_parameter_name) = single_parameter_value;
    }
  }
  return merged_distribution;
}

auto mergeParameterDistributionImpl(
  std::vector<std::shared_ptr<std::unordered_map<std::string, Object>>> distribution,
  std::vector<std::shared_ptr<std::unordered_map<std::string, Object>>> additional_distribution)
  -> std::vector<std::shared_ptr<std::unordered_map<std::string, Object>>>
{
  ParameterDistribution merged_distribution;
  merged_distribution.reserve(distribution.size() * additional_distribution.size());

  for (const auto & additional_parameter_list : additional_distribution) {
    for (const auto & list : distribution) {
      merged_distribution.emplace_back(list);
      merged_distribution.back()->insert(
        additional_parameter_list->begin(), additional_parameter_list->end());
    }
  }
  return merged_distribution;
}
}  // namespace openscenario_interpreter