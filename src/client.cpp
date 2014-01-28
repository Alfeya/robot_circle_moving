#include <iostream>
#include <cmath>
#include <cstdlib>

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <tf/transform_listener.h>
#include "robot_circle_moving/RobotCircleMoving.h"

int main(int argc, char** argv)
{
	//init the ROS node
	ros::init(argc, argv, "robot_circle_moving_client");

	ros::NodeHandle nh;
	
	ros::ServiceClient client = nh.serviceClient<robot_circle_moving::RobotCircleMoving>("robot_circle_moving");
	robot_circle_moving::RobotCircleMoving srv;	
	srv.request.velocity = atoll(argv[1]);
	srv.request.radius = atoll(argv[2]);
		
	if (client.call(srv))
	{
		ROS_INFO("Sum: %ld", (long int)srv.response.sum);
	}
	else
	{
		ROS_ERROR("Failed to call service robot circle moving");
		return 1;
	}

  return 0;
}
