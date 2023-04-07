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

#include <quaternion_operation/quaternion_operation.h>

#include <algorithm>
#include <memory>
#include <string>
#include <traffic_simulator/entity/vehicle_entity.hpp>
#include <traffic_simulator_msgs/msg/vehicle_parameters.hpp>
#include <vector>

namespace traffic_simulator
{
namespace entity
{
VehicleEntity::VehicleEntity(
  const std::string & name, const CanonicalizedEntityStatus & entity_status,
  const std::shared_ptr<hdmap_utils::HdMapUtils> & hdmap_utils_ptr,
  const traffic_simulator_msgs::msg::VehicleParameters & parameters,
  const std::string & plugin_name)
: EntityBase(name, entity_status, hdmap_utils_ptr),
  loader_(pluginlib::ClassLoader<entity_behavior::BehaviorPluginBase>(
    "traffic_simulator", "entity_behavior::BehaviorPluginBase")),
  behavior_plugin_ptr_(loader_.createSharedInstance(plugin_name)),
  route_planner_(hdmap_utils_ptr_)
{
  behavior_plugin_ptr_->configure(rclcpp::get_logger(name));
  behavior_plugin_ptr_->setVehicleParameters(parameters);
  behavior_plugin_ptr_->setDebugMarker({});
  behavior_plugin_ptr_->setBehaviorParameter(traffic_simulator_msgs::msg::BehaviorParameter());
  behavior_plugin_ptr_->setHdMapUtils(hdmap_utils_ptr_);
}

void VehicleEntity::appendDebugMarker(visualization_msgs::msg::MarkerArray & marker_array)
{
  const auto marker = behavior_plugin_ptr_->getDebugMarker();
  std::copy(marker.begin(), marker.end(), std::back_inserter(marker_array.markers));
}

void VehicleEntity::cancelRequest()
{
  behavior_plugin_ptr_->setRequest(behavior::Request::NONE);
  route_planner_.cancelRoute();
}

auto VehicleEntity::getCurrentAction() const -> std::string
{
  if (not npc_logic_started_) {
    return "waiting";
  } else {
    return behavior_plugin_ptr_->getCurrentAction();
  }
}

auto VehicleEntity::getDefaultDynamicConstraints() const
  -> const traffic_simulator_msgs::msg::DynamicConstraints &
{
  static auto default_dynamic_constraints = traffic_simulator_msgs::msg::DynamicConstraints();
  return default_dynamic_constraints;
}

auto VehicleEntity::getBehaviorParameter() const -> traffic_simulator_msgs::msg::BehaviorParameter
{
  return behavior_plugin_ptr_->getBehaviorParameter();
}

auto VehicleEntity::getEntityType() const -> const traffic_simulator_msgs::msg::EntityType &
{
  static traffic_simulator_msgs::msg::EntityType type;
  type.type = traffic_simulator_msgs::msg::EntityType::VEHICLE;
  return type;
}

auto VehicleEntity::getEntityTypename() const -> const std::string &
{
  static const std::string result = "VehicleEntity";
  return result;
}

auto VehicleEntity::getGoalPoses() -> std::vector<CanonicalizedLaneletPose>
{
  return route_planner_.getGoalPoses();
}

auto VehicleEntity::getObstacle() -> std::optional<traffic_simulator_msgs::msg::Obstacle>
{
  return behavior_plugin_ptr_->getObstacle();
}

auto VehicleEntity::getRouteLanelets(double horizon) -> std::vector<std::int64_t>
{
  if (status_.laneMatchingSucceed()) {
    return route_planner_.getRouteLanelets(
      CanonicalizedLaneletPose(static_cast<EntityStatus>(status_).lanelet_pose, hdmap_utils_ptr_),
      horizon);
  } else {
    return {};
  }
}

auto VehicleEntity::getWaypoints() -> const traffic_simulator_msgs::msg::WaypointsArray
{
  if (!npc_logic_started_) {
    return traffic_simulator_msgs::msg::WaypointsArray();
  }
  try {
    return behavior_plugin_ptr_->getWaypoints();
  } catch (const std::runtime_error & e) {
    if (not status_.laneMatchingSucceed()) {
      THROW_SIMULATION_ERROR(
        "Failed to calculate waypoints in NPC logics, please check Entity : ", name,
        " is in a lane coordinate.");
    } else {
      THROW_SIMULATION_ERROR("Failed to calculate waypoint in NPC logics.");
    }
  }
}

void VehicleEntity::onUpdate(double current_time, double step_time)
{
  EntityBase::onUpdate(current_time, step_time);
  if (npc_logic_started_) {
    behavior_plugin_ptr_->setOtherEntityStatus(other_status_);
    behavior_plugin_ptr_->setEntityTypeList(entity_type_list_);
    behavior_plugin_ptr_->setEntityStatus(std::make_unique<CanonicalizedEntityStatus>(status_));
    behavior_plugin_ptr_->setTargetSpeed(target_speed_);

    std::vector<std::int64_t> route_lanelets = getRouteLanelets();
    behavior_plugin_ptr_->setRouteLanelets(route_lanelets);

    // recalculate spline only when input data changes
    if (previous_route_lanelets_ != route_lanelets) {
      previous_route_lanelets_ = route_lanelets;
      try {
        spline_ = std::make_shared<math::geometry::CatmullRomSpline>(
          hdmap_utils_ptr_->getCenterPoints(route_lanelets));
      } catch (const common::scenario_simulator_exception::SemanticError & error) {
        // reset the ptr when spline cannot be calculated
        spline_.reset();
      }
    }
    behavior_plugin_ptr_->setReferenceTrajectory(spline_);
    behavior_plugin_ptr_->update(current_time, step_time);
    auto status_updated = behavior_plugin_ptr_->getUpdatedStatus();
    if (status_updated->laneMatchingSucceed()) {
      const auto lanelet_pose = status_updated->getLaneletPose();
      if (
        hdmap_utils_ptr_->getFollowingLanelets(lanelet_pose.lanelet_id).size() == 1 &&
        hdmap_utils_ptr_->getLaneletLength(lanelet_pose.lanelet_id) <= lanelet_pose.s) {
        stopAtEndOfRoad();
        return;
      }
    }
    setStatus(*status_updated);
    updateStandStillDuration(step_time);
    updateTraveledDistance(step_time);
  } else {
    updateEntityStatusTimestamp(current_time);
  }
  EntityBase::onPostUpdate(current_time, step_time);
}

void VehicleEntity::requestAcquirePosition(const CanonicalizedLaneletPose & lanelet_pose)
{
  behavior_plugin_ptr_->setRequest(behavior::Request::FOLLOW_LANE);
  if (status_.laneMatchingSucceed()) {
    route_planner_.setWaypoints({lanelet_pose});
  }
  behavior_plugin_ptr_->setGoalPoses({static_cast<geometry_msgs::msg::Pose>(lanelet_pose)});
}

void VehicleEntity::requestAcquirePosition(const geometry_msgs::msg::Pose & map_pose)
{
  behavior_plugin_ptr_->setRequest(behavior::Request::FOLLOW_LANE);
  if (const auto lanelet_pose = hdmap_utils_ptr_->toLaneletPose(map_pose, getBoundingBox(), false);
      lanelet_pose) {
    requestAcquirePosition(CanonicalizedLaneletPose(lanelet_pose.value(), hdmap_utils_ptr_));
  } else {
    THROW_SEMANTIC_ERROR("Goal of the vehicle entity should be on lane.");
  }
}

void VehicleEntity::requestAssignRoute(const std::vector<CanonicalizedLaneletPose> & waypoints)
{
  if (!laneMatchingSucceed()) {
    return;
  }
  behavior_plugin_ptr_->setRequest(behavior::Request::FOLLOW_LANE);
  route_planner_.setWaypoints(waypoints);
  std::vector<geometry_msgs::msg::Pose> goal_poses;
  for (const auto & waypoint : waypoints) {
    goal_poses.emplace_back(static_cast<geometry_msgs::msg::Pose>(waypoint));
  }
  behavior_plugin_ptr_->setGoalPoses(goal_poses);
}

void VehicleEntity::requestAssignRoute(const std::vector<geometry_msgs::msg::Pose> & waypoints)
{
  std::vector<CanonicalizedLaneletPose> route;
  for (const auto & waypoint : waypoints) {
    if (const auto lanelet_waypoint =
          hdmap_utils_ptr_->toLaneletPose(waypoint, getBoundingBox(), false);
        lanelet_waypoint) {
      route.emplace_back(CanonicalizedLaneletPose(lanelet_waypoint.value(), hdmap_utils_ptr_));
    } else {
      THROW_SEMANTIC_ERROR("Waypoint of pedestrian entity should be on lane.");
    }
  }
  requestAssignRoute(route);
}

void VehicleEntity::requestLaneChange(const std::int64_t to_lanelet_id)
{
  behavior_plugin_ptr_->setRequest(behavior::Request::LANE_CHANGE);
  const auto parameter = lane_change::Parameter(
    lane_change::AbsoluteTarget(to_lanelet_id), lane_change::TrajectoryShape::CUBIC,
    lane_change::Constraint());
  behavior_plugin_ptr_->setLaneChangeParameters(parameter);
}

void VehicleEntity::requestLaneChange(const traffic_simulator::lane_change::Parameter & parameter)
{
  behavior_plugin_ptr_->setRequest(behavior::Request::LANE_CHANGE);
  behavior_plugin_ptr_->setLaneChangeParameters(parameter);
}

void VehicleEntity::setAccelerationLimit(double acceleration)
{
  if (acceleration <= 0.0) {
    THROW_SEMANTIC_ERROR("Acceleration limit must be greater than or equal to zero.");
  }
  auto behavior_parameter = getBehaviorParameter();
  behavior_parameter.dynamic_constraints.max_acceleration = acceleration;
  setBehaviorParameter(behavior_parameter);
}

void VehicleEntity::setAccelerationRateLimit(double acceleration_rate)
{
  if (acceleration_rate <= 0.0) {
    THROW_SEMANTIC_ERROR("Acceleration rate limit must be greater than or equal to zero.");
  }
  auto behavior_parameter = getBehaviorParameter();
  behavior_parameter.dynamic_constraints.max_acceleration_rate = acceleration_rate;
  setBehaviorParameter(behavior_parameter);
}

void VehicleEntity::setDecelerationLimit(double deceleration)
{
  if (deceleration <= 0.0) {
    THROW_SEMANTIC_ERROR("Deceleration limit must be greater than or equal to zero.");
  }
  auto behavior_parameter = getBehaviorParameter();
  behavior_parameter.dynamic_constraints.max_deceleration = deceleration;
  setBehaviorParameter(behavior_parameter);
}

void VehicleEntity::setDecelerationRateLimit(double deceleration_rate)
{
  if (deceleration_rate <= 0.0) {
    THROW_SEMANTIC_ERROR("Deceleration rate limit must be greater than or equal to zero.");
  }
  auto behavior_parameter = getBehaviorParameter();
  behavior_parameter.dynamic_constraints.max_deceleration_rate = deceleration_rate;
  setBehaviorParameter(behavior_parameter);
}

void VehicleEntity::setBehaviorParameter(
  const traffic_simulator_msgs::msg::BehaviorParameter & parameter)
{
  behavior_plugin_ptr_->setBehaviorParameter(parameter);
}

void VehicleEntity::setTrafficLightManager(
  const std::shared_ptr<traffic_simulator::TrafficLightManagerBase> & ptr)
{
  EntityBase::setTrafficLightManager(ptr);
  behavior_plugin_ptr_->setTrafficLightManager(traffic_light_manager_);
}
}  // namespace entity
}  // namespace traffic_simulator
