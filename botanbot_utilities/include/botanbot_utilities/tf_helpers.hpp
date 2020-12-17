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

#ifndef BOTANBOT_UTILITIES__TF_HELPERS_HPP_
#define BOTANBOT_UTILITIES__TF_HELPERS_HPP_

#include <string>
#include <memory>
#include "tf2_ros/buffer.h"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"

namespace botanbot_utilities
{
/**
 * @brief Transform a PoseStamped from one frame to another while catching exceptions
 *
 * Also returns immediately if the frames are equal.
 * @param tf Smart pointer to TFListener
 * @param frame Frame to transform the pose into
 * @param in_pose Pose to transform
 * @param out_pose Place to store the resulting transformed pose
 * @return True if successful transform
 */
bool transformPose(
  const std::shared_ptr<tf2_ros::Buffer> tf,
  const std::string frame,
  const geometry_msgs::msg::PoseStamped & in_pose,
  geometry_msgs::msg::PoseStamped & out_pose,
  rclcpp::Duration & transform_tolerance
);

}  // namespace botanbot_utilities

#endif  // BOTANBOT_UTILITIES__TF_HELPERS_HPP_