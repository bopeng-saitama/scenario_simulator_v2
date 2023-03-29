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

#include <traffic_simulator/entity/ego_entity_simulation.hpp>
#include <concealer/autoware_universe.hpp>
#include <traffic_simulator/helper/helper.hpp>

namespace traffic_simulator
{
namespace entity
{

template <typename T>
static auto getParameter(const std::string & name, T value = {})
{
  rclcpp::Node node{"get_parameter", "simulation"};

  node.declare_parameter<T>(name, value);
  node.get_parameter<T>(name, value);

  return value;
}

EgoEntitySimulation::EgoEntitySimulation(const traffic_simulator_msgs::msg::VehicleParameters & parameters,
                                         double step_time)
 : autoware(std::make_unique<concealer::AutowareUniverse>())
 , vehicle_model_type_(getVehicleModelType())
 , vehicle_model_ptr_(makeSimulationModel(vehicle_model_type_, step_time, parameters))
 {}

auto toString(const VehicleModelType datum) -> std::string
{
#define BOILERPLATE(IDENTIFIER)      \
case VehicleModelType::IDENTIFIER: \
  return #IDENTIFIER

  switch (datum) {
    BOILERPLATE(DELAY_STEER_ACC);
    BOILERPLATE(DELAY_STEER_ACC_GEARED);
    BOILERPLATE(DELAY_STEER_VEL);
    BOILERPLATE(IDEAL_STEER_ACC);
    BOILERPLATE(IDEAL_STEER_ACC_GEARED);
    BOILERPLATE(IDEAL_STEER_VEL);
  }

#undef BOILERPLATE

  THROW_SIMULATION_ERROR("Unsupported vehicle model type, failed to convert to string");
}

auto EgoEntitySimulation::getVehicleModelType() -> VehicleModelType
{
  const auto vehicle_model_type =
      getParameter<std::string>("vehicle_model_type", "IDEAL_STEER_VEL");

  static const std::unordered_map<std::string, VehicleModelType> table{
      {"DELAY_STEER_ACC", VehicleModelType::DELAY_STEER_ACC},
      {"DELAY_STEER_ACC_GEARED", VehicleModelType::DELAY_STEER_ACC_GEARED},
      {"DELAY_STEER_VEL", VehicleModelType::DELAY_STEER_VEL},
      {"IDEAL_STEER_ACC", VehicleModelType::IDEAL_STEER_ACC},
      {"IDEAL_STEER_ACC_GEARED", VehicleModelType::IDEAL_STEER_ACC_GEARED},
      {"IDEAL_STEER_VEL", VehicleModelType::IDEAL_STEER_VEL},
  };

  const auto iter = table.find(vehicle_model_type);

  if (iter != std::end(table)) {
    return iter->second;
  } else {
    THROW_SEMANTIC_ERROR("Unsupported vehicle_model_type ", vehicle_model_type, " specified");
  }
}

auto EgoEntitySimulation::makeSimulationModel(
    const VehicleModelType vehicle_model_type, const double step_time,
    const traffic_simulator_msgs::msg::VehicleParameters & parameters)
-> const std::shared_ptr<SimModelInterface>
{
  // clang-format off
  const auto acc_time_constant   = getParameter<double>("acc_time_constant",     0.1);
  const auto acc_time_delay      = getParameter<double>("acc_time_delay",        0.1);
  const auto steer_lim           = getParameter<double>("steer_lim",            parameters.axles.front_axle.max_steering);  // 1.0
  const auto steer_rate_lim      = getParameter<double>("steer_rate_lim",        5.0);
  const auto steer_time_constant = getParameter<double>("steer_time_constant",   0.27);
  const auto steer_time_delay    = getParameter<double>("steer_time_delay",      0.24);
  const auto vel_lim             = getParameter<double>("vel_lim",              parameters.performance.max_speed);  // 50.0
  const auto vel_rate_lim        = getParameter<double>("vel_rate_lim",         parameters.performance.max_acceleration);  // 7.0
  const auto vel_time_constant   = getParameter<double>("vel_time_constant",     0.1);
  const auto vel_time_delay      = getParameter<double>("vel_time_delay",        0.1);
  const auto wheel_base          = getParameter<double>("wheel_base",           parameters.axles.front_axle.position_x - parameters.axles.rear_axle.position_x);
  // clang-format on

  switch (vehicle_model_type) {
    case VehicleModelType::DELAY_STEER_ACC:
      return std::make_shared<SimModelDelaySteerAcc>(
          vel_lim, steer_lim, vel_rate_lim, steer_rate_lim, wheel_base, step_time, acc_time_delay,
          acc_time_constant, steer_time_delay, steer_time_constant);

    case VehicleModelType::DELAY_STEER_ACC_GEARED:
      return std::make_shared<SimModelDelaySteerAccGeared>(
          vel_lim, steer_lim, vel_rate_lim, steer_rate_lim, wheel_base, step_time, acc_time_delay,
          acc_time_constant, steer_time_delay, steer_time_constant);

    case VehicleModelType::DELAY_STEER_VEL:
      return std::make_shared<SimModelDelaySteerVel>(
          vel_lim, steer_lim, vel_rate_lim, steer_rate_lim, wheel_base, step_time, vel_time_delay,
          vel_time_constant, steer_time_delay, steer_time_constant);

    case VehicleModelType::IDEAL_STEER_ACC:
      return std::make_shared<SimModelIdealSteerAcc>(wheel_base);

    case VehicleModelType::IDEAL_STEER_ACC_GEARED:
      return std::make_shared<SimModelIdealSteerAccGeared>(wheel_base);

    case VehicleModelType::IDEAL_STEER_VEL:
      return std::make_shared<SimModelIdealSteerVel>(wheel_base);

    default:
      THROW_SEMANTIC_ERROR(
          "Unsupported vehicle_model_type ", toString(vehicle_model_type), " specified");
  }
}

auto EgoEntitySimulation::setAutowareStatus() -> void
{
  const auto current_pose = status_.pose;

  autoware->set([this]() {
    geometry_msgs::msg::Accel message;
    message.linear.x = vehicle_model_ptr_->getAx();
    return message;
  }());

  autoware->set(current_pose);

  autoware->set(getCurrentTwist());

  autoware->update();
}

void EgoEntitySimulation::requestSpeedChange(double value)
{
  Eigen::VectorXd v(vehicle_model_ptr_->getDimX());

  switch (vehicle_model_type_) {
    case VehicleModelType::DELAY_STEER_ACC:
    case VehicleModelType::DELAY_STEER_ACC_GEARED:
      v << 0, 0, 0, value, 0, 0;
      break;

    case VehicleModelType::IDEAL_STEER_ACC:
    case VehicleModelType::IDEAL_STEER_ACC_GEARED:
      v << 0, 0, 0, value;
      break;

    case VehicleModelType::IDEAL_STEER_VEL:
      v << 0, 0, 0;
      break;

    case VehicleModelType::DELAY_STEER_VEL:
      v << 0, 0, 0, value, 0;
      break;

    default:
      THROW_SEMANTIC_ERROR(
          "Unsupported simulation model ", toString(vehicle_model_type_), " specified");
  }

  vehicle_model_ptr_->setState(v);
}

  void EgoEntitySimulation::onUpdate(double time, double step_time, bool npc_logic_started) {
    autoware->spinSome();

    if (npc_logic_started) {
      Eigen::VectorXd input(vehicle_model_ptr_->getDimU());

      switch (vehicle_model_type_) {
        case VehicleModelType::DELAY_STEER_ACC:
        case VehicleModelType::IDEAL_STEER_ACC:
          input << autoware->getGearSign() * autoware->getAcceleration(),
              autoware->getSteeringAngle();
          break;

        case VehicleModelType::DELAY_STEER_ACC_GEARED:
        case VehicleModelType::IDEAL_STEER_ACC_GEARED:
          input << autoware->getGearSign() * autoware->getAcceleration(),
              autoware->getSteeringAngle();
          break;

        case VehicleModelType::DELAY_STEER_VEL:
        case VehicleModelType::IDEAL_STEER_VEL:
          input << autoware->getVelocity(), autoware->getSteeringAngle();
          break;

        default:
          THROW_SEMANTIC_ERROR(
              "Unsupported vehicle_model_type ", toString(vehicle_model_type_), "specified");
      }

      vehicle_model_ptr_->setGear(autoware->getGearCommand().command);
      vehicle_model_ptr_->setInput(input);
      vehicle_model_ptr_->update(step_time);
    }

    updateStatus(time, step_time);
    updatePreviousValuesAndUpdateAutoware();
    setAutowareStatus();
 }

  auto EgoEntitySimulation::getCurrentTwist() const -> geometry_msgs::msg::Twist
  {
    geometry_msgs::msg::Twist current_twist;
    current_twist.linear.x = vehicle_model_ptr_->getVx();
    current_twist.angular.z = vehicle_model_ptr_->getWz();
    return current_twist;
  }

  auto EgoEntitySimulation::getCurrentPose() const -> geometry_msgs::msg::Pose
  {
    Eigen::VectorXd relative_position(3);
    relative_position(0) = vehicle_model_ptr_->getX();
    relative_position(1) = vehicle_model_ptr_->getY();
    relative_position(2) = 0.0;
    relative_position =
        quaternion_operation::getRotationMatrix(initial_pose_.orientation) * relative_position;

    geometry_msgs::msg::Pose current_pose;
    current_pose.position.x = initial_pose_.position.x + relative_position(0);
    current_pose.position.y = initial_pose_.position.y + relative_position(1);
    current_pose.position.z = initial_pose_.position.z + relative_position(2);
    current_pose.orientation = [this]() {
      geometry_msgs::msg::Vector3 rpy;
      rpy.x = 0;
      rpy.y = 0;
      rpy.z = vehicle_model_ptr_->getYaw();
      return initial_pose_.orientation *
             quaternion_operation::convertEulerAngleToQuaternion(rpy);
    }();

    return current_pose;
  }

  auto EgoEntitySimulation::getCurrentAccel(const double step_time) const -> geometry_msgs::msg::Accel {
    geometry_msgs::msg::Accel accel;
    if (previous_angular_velocity_) {
      accel.linear.x = vehicle_model_ptr_->getAx();
      accel.angular.z =
          (vehicle_model_ptr_->getWz() - previous_angular_velocity_.value()) / step_time;
    }
    return accel;
  }

  auto EgoEntitySimulation::getLinearJerk(double step_time) -> double {
    // FIXME: This seems to be an acceleration, not jerk
    if (previous_linear_velocity_) {
      return (vehicle_model_ptr_->getVx() - previous_linear_velocity_.value()) / step_time;
    } else {
      return 0;
    }
  }

  auto EgoEntitySimulation::updatePreviousValuesAndUpdateAutoware() -> void {
    previous_linear_velocity_ = vehicle_model_ptr_->getVx();
    previous_angular_velocity_ = vehicle_model_ptr_->getWz();

    autoware->update();
  }

  auto EgoEntitySimulation::getStatus() const -> const traffic_simulator_msgs::msg::EntityStatus & {
    return status_;
  }

  auto EgoEntitySimulation::setInitialStatus(const traffic_simulator_msgs::msg::EntityStatus & status) -> void {
    status_ = status;
    initial_pose_ = status_.pose;
  }

  auto EgoEntitySimulation::updateStatus(double time, double step_time) -> void {
    traffic_simulator_msgs::msg::EntityStatus status;
    status.time = time;
    status.type = status_.type;
    status.bounding_box = status_.bounding_box;
    status.pose = getCurrentPose();
    status.action_status.twist = getCurrentTwist();
    status.action_status.accel = getCurrentAccel(step_time);
    status.action_status.linear_jerk = getLinearJerk(step_time);

    status_ = status;
  }
}
}

