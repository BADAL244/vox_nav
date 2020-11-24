// Copyright (c) 2020 Fetullah Atas
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
#ifndef BOTANBOT_GPS_WAYPOINT_FOLLOWER__WAYPOINT_FOLLOWER_CLIENT_HPP_
#define BOTANBOT_GPS_WAYPOINT_FOLLOWER__WAYPOINT_FOLLOWER_CLIENT_HPP_

#include "nav2_lifecycle_manager/lifecycle_manager_client.hpp"
#include "nav2_msgs/action/navigate_to_pose.hpp"
#include "nav2_msgs/action/follow_waypoints.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "geometry_msgs/msg/point32.hpp"
#include "nav2_util/geometry_utils.hpp"
#include "robot_localization/srv/to_ll.hpp"
#include "robot_localization/srv/from_ll.hpp"


/**
 * @brief namespace for way point following, points are from a yaml file
 *
 */
namespace botanbot_gps_waypoint_follower
{
enum class ActionStatus
{
  UNKNOWN = 0,
  PROCESSING = 1,
  FAILED = 2,
  SUCCEEDED = 3
};

/**
 * @brief A ros node that drives robot through gievn way points from YAML file
 *
 */
class GPSWayPointFollower : public rclcpp::Node
{
private:
  // shorten the Goal handler Client type
  using WaypointFollowerGoalHandle =
    rclcpp_action::ClientGoalHandle<nav2_msgs::action::FollowWaypoints>;

  // client to connect waypoint follower service(FollowWaypoints)
  rclcpp_action::Client<nav2_msgs::action::FollowWaypoints>::SharedPtr
    waypoint_follower_action_client_;

  // stores the waypoints in a vector with additional info such as
  // "int32[] missed_waypoints" and "uint32
  // current_waypoint"
  nav2_msgs::action::FollowWaypoints::Goal waypoint_follower_goal_;

  // goal handler to query state of goal
  WaypointFollowerGoalHandle::SharedPtr waypoint_follower_goal_handle_;

  // we load poses from Yaml file and store them in this vector
  std::vector<geometry_msgs::msg::PoseStamped> acummulated_poses_;

  // we load poses from Yaml file and store them in this vector
  std::vector<geometry_msgs::msg::Point32> acummulated_gps_waypoints_;

public:
  /**
   * @brief Construct a new Way Point Folllower Demo object
   *
   */
  GPSWayPointFollower();

  /**
   * @brief Destroy the Way Point Folllower Demo object
   *
   */
  ~GPSWayPointFollower();

  /**
   * @brief send robot through each of the pose in poses vector
   *
   * @param poses
   */
  void startWaypointFollowing();

  
  /**
   * @brief 
   * 
   */
  void convertGPSWaypointstoPosesinMap();
  /**
 * @brief given a parameter name on the yaml file, loads this parameter as geometry_msgs::Point.
 *  Note that this parameter needs to be an array of doubles
 *
 * @param param_name
 * @return geometry_msgs::Point
 */
  geometry_msgs::msg::Point32
  loadVectorofDoubleAsPairFromYAML(std::string param_name);

  void resultCallback(
    const rclcpp_action::ClientGoalHandle
    <nav2_msgs::action::FollowWaypoints>::WrappedResult & result);

  void goalResponseCallback(
    std::shared_future<rclcpp_action::ClientGoalHandle
    <nav2_msgs::action::FollowWaypoints>::SharedPtr> future);

  ActionStatus current_goal_status_;

  rclcpp::Client<robot_localization::srv::FromLL>::SharedPtr from_ll_to_map_client_;
};
}  // namespace botanbot_gps_waypoint_follower

#endif  // BOTANBOT_GPS_WAYPOINT_FOLLOWER__WAYPOINT_FOLLOWER_CLIENT_HPP_