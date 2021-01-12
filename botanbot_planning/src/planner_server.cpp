// Copyright (c) 2018 Intel Corporation
// Copyright (c) 2019 Samsung Research America
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

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "builtin_interfaces/msg/duration.hpp"
#include "botanbot_planning/planner_server.hpp"

using namespace std::chrono_literals;

namespace botanbot_planning
{

PlannerServer::PlannerServer()
: Node("botanbot_planning_server_rclcpp_node"),
  pc_loader_("botanbot_planning", "botanbot_planning::PlannerCore"),
  default_ids_{"GridBased"},
  default_types_{"nav2_navfn_planner/NavfnPlanner"}
{
  RCLCPP_INFO(get_logger(), "Creating");

  // Declare this node's parameters
  declare_parameter("planner_plugins", default_ids_);
  declare_parameter("expected_planner_frequency", 1.0);
  get_parameter("planner_plugins", planner_ids_);

  if (planner_ids_ == default_ids_) {
    for (size_t i = 0; i < default_ids_.size(); ++i) {
      declare_parameter(default_ids_[i] + ".plugin", default_types_[i]);
    }
  }
  planner_types_.resize(planner_ids_.size());
  auto node = shared_from_this();

  for (size_t i = 0; i != planner_ids_.size(); i++) {
    try {
      planner_types_[i] =
        this->get_parameter(default_ids_[i] + ".plugin").as_string();
      botanbot_planning::PlannerCore::Ptr planner =
        pc_loader_.createUniqueInstance(planner_types_[i]);
      RCLCPP_INFO(
        get_logger(), "Created global planner plugin %s of type %s",
        planner_ids_[i].c_str(), planner_types_[i].c_str());
      planners_.insert({planner_ids_[i], planner});
    } catch (const pluginlib::PluginlibException & ex) {
      RCLCPP_FATAL(
        get_logger(), "Failed to create global planner. Exception: %s",
        ex.what());
    }
  }

  for (size_t i = 0; i != planner_ids_.size(); i++) {
    planner_ids_concat_ += planner_ids_[i] + std::string(" ");
  }

  RCLCPP_INFO(
    get_logger(),
    "Planner Server has %s planners available.", planner_ids_concat_.c_str());

  double expected_planner_frequency;
  get_parameter("expected_planner_frequency", expected_planner_frequency);
  if (expected_planner_frequency > 0) {
    max_planner_duration_ = 1 / expected_planner_frequency;
  } else {
    RCLCPP_WARN(
      get_logger(),
      "The expected planner frequency parameter is %.4f Hz. The value should to be greater"
      " than 0.0 to turn on duration overrrun warning messages", expected_planner_frequency);
    max_planner_duration_ = 0.0;
  }

  // Initialize pubs & subs
  plan_publisher_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("plan", 1);

  this->action_server_ = rclcpp_action::create_server<ComputePathToPose>(
    this->get_node_base_interface(),
    this->get_node_clock_interface(),
    this->get_node_logging_interface(),
    this->get_node_waitables_interface(),
    "compute_path_to_pose",
    std::bind(&PlannerServer::handle_goal, this, std::placeholders::_1, std::placeholders::_2),
    std::bind(&PlannerServer::handle_cancel, this, std::placeholders::_1),
    std::bind(&PlannerServer::handle_accepted, this, std::placeholders::_1));
}

PlannerServer::~PlannerServer()
{
  RCLCPP_INFO(get_logger(), "Destroying");
  planners_.clear();
  action_server_.reset();
  plan_publisher_.reset();
  RCLCPP_INFO(get_logger(), "Shutting down");
}

rclcpp_action::GoalResponse PlannerServer::handle_goal(
  const rclcpp_action::GoalUUID & uuid,
  std::shared_ptr<const ComputePathToPose::Goal> goal)
{
  RCLCPP_INFO(
    this->get_logger(), "Received goal request in order to compute a path to pose");
  (void)uuid;
  return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
}

rclcpp_action::CancelResponse PlannerServer::handle_cancel(
  const std::shared_ptr<GoalHandleComputePathToPose> goal_handle)
{
  RCLCPP_INFO(this->get_logger(), "Received request to cancel goal");
  (void)goal_handle;
  return rclcpp_action::CancelResponse::ACCEPT;
}

void PlannerServer::handle_accepted(const std::shared_ptr<GoalHandleComputePathToPose> goal_handle)
{
  // this needs to return quickly to avoid blocking the executor, so spin up a new thread
  std::thread{std::bind(&PlannerServer::computePlan, this, std::placeholders::_1),
    goal_handle}.detach();
}

void
PlannerServer::computePlan(const std::shared_ptr<GoalHandleComputePathToPose> goal_handle)
{
  auto start_time = steady_clock_.now();
  RCLCPP_INFO(this->get_logger(), "Executing goal");
  rclcpp::Rate loop_rate(1);
  const auto goal = goal_handle->get_goal();
  auto feedback = std::make_shared<ComputePathToPose::Feedback>();
  /*auto & sequence = feedback->sequence;
  sequence.push_back(0);
  sequence.push_back(1);*/
  auto result = std::make_shared<ComputePathToPose::Result>();

  //result->path = getPlan(start, goal->pose, goal->planner_id);

  if (result->path.poses.size() == 0) {
    RCLCPP_WARN(
      get_logger(), "Planning algorithm %s failed to generate a valid"
      " path to (%.2f, %.2f)", goal->planner_id.c_str(),
      goal->pose.pose.position.x, goal->pose.pose.position.y);
    //action_server_->terminate_current();
    return;
  }

  RCLCPP_DEBUG(
    get_logger(),
    "Found valid path of size %u to (%.2f, %.2f)",
    result->path.poses.size(), goal->pose.pose.position.x,
    goal->pose.pose.position.y);

  // Publish the plan for visualization purposes
  //publishPlan(result->path);

  auto cycle_duration = steady_clock_.now() - start_time;
  result->planning_time = cycle_duration;

  if (max_planner_duration_ && cycle_duration.seconds() > max_planner_duration_) {
    RCLCPP_WARN(
      get_logger(),
      "Planner loop missed its desired rate of %.4f Hz. Current loop rate is %.4f Hz",
      1 / max_planner_duration_, 1 / cycle_duration.seconds());
  }

  /*/catch (std::exception & ex) {
     RCLCPP_WARN(
       get_logger(), "%s plugin failed to plan calculation to (%.2f, %.2f): \"%s\"",
       goal->planner_id.c_str(), goal->pose.pose.position.x,
       goal->pose.pose.position.y, ex.what());
     // TODO(orduno): provide information about fail error to parent task,
     //               for example: couldn't get costmap update
     //action_server_->terminate_current();
   }*/
}

std::vector<geometry_msgs::msg::PoseStamped>
PlannerServer::getPlan(
  const geometry_msgs::msg::PoseStamped & start,
  const geometry_msgs::msg::PoseStamped & goal,
  const std::string & planner_id)
{
  RCLCPP_DEBUG(
    get_logger(), "Attempting to a find path from (%.2f, %.2f) to "
    "(%.2f, %.2f).", start.pose.position.x, start.pose.position.y,
    goal.pose.position.x, goal.pose.position.y);

  if (planners_.find(planner_id) != planners_.end()) {
    return planners_[planner_id]->createPlan(start, goal);
  } else {
    if (planners_.size() == 1 && planner_id.empty()) {
      RCLCPP_WARN_ONCE(
        get_logger(), "No planners specified in action call. "
        "Server will use only plugin %s in server."
        " This warning will appear once.", planner_ids_concat_.c_str());
      return planners_[planners_.begin()->first]->createPlan(start, goal);
    } else {
      RCLCPP_ERROR(
        get_logger(), "planner %s is not a valid planner. "
        "Planner names are: %s", planner_id.c_str(),
        planner_ids_concat_.c_str());
    }
  }
  return std::vector<geometry_msgs::msg::PoseStamped>();
}

void
PlannerServer::publishPlan(const std::vector<geometry_msgs::msg::PoseStamped> & path)
{
  if (this->count_subscribers(plan_publisher_->get_topic_name()) > 0) {
    //plan_publisher_->publish(std::move(msg));
  }
}
}  // namespace botanbot_planning


int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<botanbot_planning::PlannerServer>();
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}