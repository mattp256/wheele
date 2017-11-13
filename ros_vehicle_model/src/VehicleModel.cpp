#include "VehicleModel.hpp"

using namespace std;
using namespace CIRC;

/**********************************************************************
** Wheel-E Vehicle Model
** Inputs:
**	- Platform Command Curvature (1/m):  reciprocal of radius of
**	  curvature of center of robot.  Positive value is leftward turn,
**	  negative rightward, 0 is straight.
**	- Platform Command Throttle (m/s):  speed of the robot.  Positive
**	  is forward, negative reverse.
**
** Outputs:
**	- Platform Throttle Left:  throttle command for left side motors.
**	  -1.0 is full reverse, 0.0 is stopped, +1.0 is full forward.
**	- Platform Throttle Right:  throttle command for right side motors.
**	  -1.0 is full reverse, 0.0 is stopped, +1.0 is full forward.
**	- Platform Steering Left (rad):  wheel angle for left side servos.
**	  Positive is clockwise, negative counterclockwise.
**	- Platform Steering Right (rad):  wheel angle for right side servos.
**	  Positive is clockwise, negative counterclockwise.
**********************************************************************/

//Default constructor.
VehicleModel::VehicleModel()
{
    //Topic you want to publish
    pub_ = nh_.advertise<wheele_msgs::ThrustSteer>("thrust_and_steer", 1);

    //Topic you want to subscribe
    sub_ = nh_.subscribe("wheele_cmd_vel", 1, &VehicleModel::driveCallback, this);

    //parameters
    //nh_.param("max_raw_cmd", raw_cmd_max_, raw_cmd_max_);
    //nh_.param("scale_linear_mps", linear_scale_, linear_scale_);
    //nh_.param("botwidth_m", botwidth_, botwidth_);
}

//Default destructor.
/*VehicleModel::~VehicleModel()
{
}*/

void VehicleModel::driveCallback(const wheele_msgs::SpeedCurve& spdCrv)
  {
	//Calculate steering and throttle commands based on speed and curvature
	//Publish results for Micro Maestro

    	wheele_msgs::ThrustSteer drive_cmd;
    	float speed, curvature;	// From "platform control message"
	float throttleLeft, throttleRight;	// Published values
	float steerLeft, steerRight;		// Published values
    	
	speed = spdCrv.speed;
	curvature = spdCrv.curvature;
	
	calculateSteerThrottle(
		speed, curvature,
		&throttleLeft, &throttleRight, &steerLeft, &steerRight);
    	
	drive_cmd.steerLeft = steerLeft;
	drive_cmd.throttleLeft = throttleLeft;
	/*Polysync did:
	publish steerLeft, publish throttleLeft
	//this_thread::sleep_for(chrono::milliseconds(1));
	publish steerRight, publish throttleRight
	WE CAN ALSO DO THIS IF NEEDED WITH SEPARATE MESSAGES AND PUBLISHED TOPICS
	*/
	drive_cmd.steerRight = steerRight;
	drive_cmd.throttleRight = throttleRight;

	/****
	I believe we do not need to add a timestamp ourselves here, VERIFY
	*****/
    	pub_.publish(drive_cmd);
  }

//*********************************************************************
// Calculate the steering and throttle commands based on the speed
// and curvature passed.
//*********************************************************************
void VehicleModel::calculateSteerThrottle(
	float speed, float curvature,	// Inputs
	float *throttleLeft, float *throttleRight,	// Outputs
	float *steerLeft, float *steerRight)
{
	float speedLeft;
	float speedRight;
	float absThrottleLeft;
	float absThrottleRight;
	float maxAbsThrottle;
	float throttleMultiplier;

	// Caclulate left/right wheel speeds in m/s
	speedLeft = speed * sqrt(pow(curvature * WHEELBASE_LENGTH/2, 2) + pow(1 - curvature * WHEELBASE_WIDTH / 2, 2));
	speedRight = speed * sqrt(pow(curvature * WHEELBASE_LENGTH/2, 2) + pow(1 + curvature * WHEELBASE_WIDTH / 2, 2));

	// Calculate left/right wheel angles in radians
	*steerLeft = atan(curvature * WHEELBASE_LENGTH / (2 - WHEELBASE_WIDTH));
	*steerRight = atan(curvature * WHEELBASE_LENGTH / (2 + WHEELBASE_WIDTH));

	// Normalize wheel speeds to obtain left/right throttle values
	*throttleLeft = speedLeft / FULL_THROTTLE_SPEED;
	*throttleRight = speedRight / FULL_THROTTLE_SPEED;

	// Limit throttle to +/- 1.0, keeping the ratio left/right constant to maintain curvature
	absThrottleLeft = fabs(*throttleLeft);
	absThrottleRight = fabs(*throttleRight);
	maxAbsThrottle = (absThrottleLeft > absThrottleRight) ? absThrottleLeft : absThrottleRight;
	throttleMultiplier = (maxAbsThrottle > 1.0) ? (1.0 / maxAbsThrottle) : 1.0;
	*throttleLeft *= throttleMultiplier;
	*throttleRight *= throttleMultiplier;
}
