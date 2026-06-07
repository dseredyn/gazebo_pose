#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose.hpp>

#include <gz/transport/Node.hh>
#include <gz/msgs/pose_v.pb.h>

#include "gazebo_pose_interfaces/srv/get_object_pose.hpp"

struct StoredPose
{
  geometry_msgs::msg::Pose pose;
  rclcpp::Time stamp;
};

class GazeboPoseServiceNode : public rclcpp::Node
{
public:
  GazeboPoseServiceNode()
  : rclcpp::Node("gazebo_pose_service_node")
  {
    gz_pose_topic_ = this->declare_parameter<std::string>(
      "gz_pose_topic", "/world/default/pose/info");

    service_name_ = this->declare_parameter<std::string>(
      "service_name", "get_gazebo_object_pose");

    const bool ok = gz_node_.Subscribe(
      gz_pose_topic_,
      &GazeboPoseServiceNode::onGzPoseV,
      this);

    if (!ok) {
      throw std::runtime_error("Failed to subscribe to Gazebo topic: " + gz_pose_topic_);
    }

    srv_ = this->create_service<gazebo_pose_interfaces::srv::GetObjectPose>(
      service_name_,
      std::bind(
        &GazeboPoseServiceNode::onGetObjectPose,
        this,
        std::placeholders::_1,
        std::placeholders::_2));

    RCLCPP_INFO(
      get_logger(),
      "Subscribed to Gazebo topic '%s', serving ROS service '%s'",
      gz_pose_topic_.c_str(),
      service_name_.c_str());
  }

private:
  void onGzPoseV(const gz::msgs::Pose_V& msg)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    const rclcpp::Time now = this->now();

    for (int i = 0; i < msg.pose_size(); ++i) {
      const auto& gz_pose = msg.pose(i);
      const std::string name = gz_pose.name();

      if (name.empty()) {
        continue;
      }

      geometry_msgs::msg::Pose pose;
      pose.position.x = gz_pose.position().x();
      pose.position.y = gz_pose.position().y();
      pose.position.z = gz_pose.position().z();

      pose.orientation.x = gz_pose.orientation().x();
      pose.orientation.y = gz_pose.orientation().y();
      pose.orientation.z = gz_pose.orientation().z();
      pose.orientation.w = gz_pose.orientation().w();

      poses_[name] = StoredPose{pose, now};
    }
  }

  void onGetObjectPose(
    const std::shared_ptr<gazebo_pose_interfaces::srv::GetObjectPose::Request> req,
    std::shared_ptr<gazebo_pose_interfaces::srv::GetObjectPose::Response> res)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = poses_.find(req->name);

    if (it == poses_.end()) {
      // Helpful fallback for scoped names such as default::yellow_cylinder.
      std::vector<std::string> matches;
      for (const auto& [name, stored] : poses_) {
        if (endsWith(name, "::" + req->name) || endsWith(name, "/" + req->name)) {
          matches.push_back(name);
        }
      }

      if (matches.size() == 1) {
        it = poses_.find(matches.front());
      } else if (matches.empty()) {
        res->success = false;
        res->message = "Object not found: " + req->name;
        return;
      } else {
        res->success = false;
        res->message = "Multiple matches for object: " + req->name;
        return;
      }
    }

    res->success = true;
    res->message = "Found object";
    res->pose = it->second.pose;
    res->stamp = it->second.stamp;
  }

  static bool endsWith(const std::string& value, const std::string& suffix)
  {
    if (suffix.size() > value.size()) {
      return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
  }

  std::string gz_pose_topic_;
  std::string service_name_;

  gz::transport::Node gz_node_;

  std::mutex mutex_;
  std::unordered_map<std::string, StoredPose> poses_;

  rclcpp::Service<gazebo_pose_interfaces::srv::GetObjectPose>::SharedPtr srv_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  try {
    auto node = std::make_shared<GazeboPoseServiceNode>();
    rclcpp::spin(node);
  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
