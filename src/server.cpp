#include <iostream>
#include <cmath>
#include <cstdlib>

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <tf/transform_listener.h>
#include "robot_circle_moving/RobotCircleMoving.h"

class RobotDriver
{
	private:
		//! The node handle we'll be using
		ros::NodeHandle nh_;
		//! We will be publishing to the "cmd_vel" topic to issue commands
		ros::Publisher cmd_vel_pub_;
		//! We will be listening to TF transforms as well
		tf::TransformListener listener_;

	public:
		//! ROS node initialization
		RobotDriver(ros::NodeHandle& nh) 
		{ 
			nh_ = nh;
			
			//set up the publisher for the cmd_vel topic
			cmd_vel_pub_ = nh_.advertise<geometry_msgs::Twist>("/cmd_vel_mux/input/navi", 1);
		}

		//! Drive forward a specified distance based on odometry information
		bool driveForwardOdom(double distance, double velocity)
		{
			//wait for the listener to get the first message
			listener_.waitForTransform("base_footprint", "odom", 
					ros::Time(0), ros::Duration(1.0));

			//we will record transforms here
			tf::StampedTransform start_transform;
			tf::StampedTransform current_transform;

			//record the starting transform from the odometry to the base frame
			listener_.lookupTransform("base_footprint", "odom", 
					ros::Time(0), start_transform);

			//we will be sending commands of type "twist"
			geometry_msgs::Twist base_cmd;
			//the command will be to go forward at 0.25 m/s
			base_cmd.linear.y = base_cmd.angular.z = 0;
			base_cmd.linear.x = velocity;

			ros::Rate rate(10.0);
			bool done = false;
			while (!done && nh_.ok())
			{
				//send the drive command
				cmd_vel_pub_.publish(base_cmd);
				rate.sleep();
				//get the current transform
				try
				{
					listener_.lookupTransform("base_footprint", "odom", 
							ros::Time(0), current_transform);
				}
				catch (tf::TransformException ex)
				{
					ROS_ERROR("%s",ex.what());
					break;
				}
				//see how far we've traveled
				tf::Transform relative_transform = 
					start_transform.inverse() * current_transform;
				double dist_moved = relative_transform.getOrigin().length();

				if(dist_moved > distance) done = true;
			}
			if (done) return true;
			return false;
		}

		bool turnOdom(bool clockwise, double radians)
		{
			while(radians < 0) radians += 2*M_PI;
			while(radians > 2*M_PI) radians -= 2*M_PI;

			//wait for the listener to get the first message
			listener_.waitForTransform("base_footprint", "odom", 
					ros::Time(0), ros::Duration(1.0));

			//we will record transforms here
			tf::StampedTransform start_transform;
			tf::StampedTransform current_transform;

			//record the starting transform from the odometry to the base frame
			listener_.lookupTransform("base_footprint", "odom", 
					ros::Time(0), start_transform);

			//we will be sending commands of type "twist"
			geometry_msgs::Twist base_cmd;
			//the command will be to turn at 0.75 rad/s
			base_cmd.linear.x = base_cmd.linear.y = 0.0;
			base_cmd.angular.z = 0.75;
			if (clockwise) base_cmd.angular.z = -base_cmd.angular.z;

			//the axis we want to be rotating by
			tf::Vector3 desired_turn_axis(0,0,1);
			if (!clockwise) desired_turn_axis = -desired_turn_axis;

			ros::Rate rate(10.0);
			bool done = false;
			while (!done && nh_.ok())
			{
				//send the drive command
				cmd_vel_pub_.publish(base_cmd);
				rate.sleep();
				//get the current transform
				try
				{
					listener_.lookupTransform("base_footprint", "odom", 
							ros::Time(0), current_transform);
				}
				catch (tf::TransformException ex)
				{
					ROS_ERROR("%s",ex.what());
					break;
				}
				tf::Transform relative_transform = 
					start_transform.inverse() * current_transform;
				tf::Vector3 actual_turn_axis = 
					relative_transform.getRotation().getAxis();
				double angle_turned = relative_transform.getRotation().getAngle();
				if ( fabs(angle_turned) < 1.0e-2) continue;

				if ( actual_turn_axis.dot( desired_turn_axis ) < 0 ) 
					angle_turned = 2 * M_PI - angle_turned;

				if (angle_turned > radians) done = true;
			}
			if (done) return true;
			return false;
		}

		void moveCyclically( double radius, double velocity, int times )
		{
			geometry_msgs::Twist base_cmd;

			base_cmd.linear.y = 0;
			base_cmd.linear.x = velocity;
			base_cmd.angular.z = velocity / radius;

			double rateFrequency = 10.0;

			ros::Rate rate( rateFrequency );

			int msg_number = times * ( int )( rateFrequency * 2 * M_PI * radius / velocity );
			while ( --msg_number > 0 )
			{
			        cmd_vel_pub_.publish(base_cmd);
			        rate.sleep();
			}

			base_cmd.linear.x = 0;
			base_cmd.angular.z = 0;
			cmd_vel_pub_.publish(base_cmd);
		}
};

RobotDriver* driver;

bool add(robot_circle_moving::RobotCircleMoving::Request  &req,
         robot_circle_moving::RobotCircleMoving::Response &res)
{
	res.sum = 1;

	sleep(5);

	driver->moveCyclically( req.radius, req.velocity, 10 );

	/*
	int n = 20;
	for(int i = 0; i < n; i++)
	{
		double distance = 2 * req.radius * sin( M_PI / (double)n );
		double angle = 2 * M_PI / (double)n;
		
		driver->driveForwardOdom( distance, req.velocity );
		driver->turnOdom(true, angle);
	}
	*/
	
	return true;
}
	
int main(int argc, char** argv)
{
	//init the ROS node
	ros::init(argc, argv, "robot_circle_moving_server"); 
	ros::NodeHandle nh;
	driver = new RobotDriver( nh );
	
	ros::ServiceServer service = nh.advertiseService("robot_circle_moving", add);
	ROS_INFO("Ready move robot");
	ros::spin();
	
	delete driver;
	return 0;
}
