#include <vector>
#include <math.h>
#include <stdlib.h>

#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <seabee3_driver/SetDesiredDepth.h>
#include <seabee3_driver/SetDesiredRPY.h>
#include <seabee3_driver_base/KillSwitch.h>

/* SeaBee3 Mission States */
#define STATE_INIT          0
#define STATE_DO_GATE       1
#define STATE_FIRST_BUOY    2
#define STATE_SECOND_BUOY   3

// max speed sub can go
#define MAX_SPEED              75.0

// pose error must be below GATE_POSE_ERR_THRESH
// in order to sleep for fwd gate time
#define GATE_POSE_ERR_THRESH   10

// quick hack to calculate barbwire position based on
// percentage of img dims
#define IMG_WIDTH              320.0
#define IMG_HEIGHT             240.0

// minimum diff needed between current pose and last pose
// in order for beestem  message to be sent out
#define MIN_POSE_DIFF         3.0

// ######################################################################
// The various types of SensorVotes
enum SensorType { PATH, FIRST_BUOY, SECOND_BUOY, LOCALIZATION, SENSOR_TYPE_SIZE = 4};

// A vote for what the pose of the sub should be according to
// one of our sensors (i.e. salient point or path found in bottom cam)
struct SensorVote
{
  //public:
  // Represents a pose that a SensorVote can set
  struct SensorPose
  {
    float val; // value of pose
    float weight; // weight of pose
    float decay; // how fast the weight should decay (0 = none)
  };
  enum SensorType type; // sensor type
  SensorPose heading; // sensor's vote for absolute heading
  SensorPose depth; // sensor's vote for relative depth
  bool init; // whether or not the SensorVote has a value set

  SensorVote()
  {
    type = PATH;
    heading.val = 0.0;
    heading.weight = 0.0;
    heading.decay = 0.0;
    depth.val = 0.0;
    depth.weight = 0.0;
    depth.decay = 0.0;
    init = false;
  }

  SensorVote(SensorType t)
  {
    type = t;
    heading.val = 0.0;
    heading.weight = 0.0;
    heading.decay = 0.0;
    depth.val = 0.0;
    depth.weight = 0.0;
    depth.decay = 0.0;
    init = false;
  }
};
// ######################################################################

// Stores the sub's current mission state
unsigned int itsCurrentState;

// Command-line parameters
double itsGateTime;
double itsGateDepth;
double itsHeadingCorrScale;
double itsDepthCorrScale;
double itsSpeedCorrScale;

// Variables to store the sub's current pose according to messages
// received from the BeeStemI
int its_current_heading;
int its_current_ex_pressure;
int its_current_int_pressure;


// Stores the current state of the kill switch according to BeeStemI
char itsKillSwitchState;

// Vector that stores all of the sub's SensorVotes
std::vector<SensorVote> itsSensorVotes;

// The combined error of our current heading and depth from our desired heading and depth
float itsPoseError;

// Stores the last values we set for our heading, depth, and speed.
// These are used to limit the number of messages sent to the BeeStemI
// by ensuring that we only send a message if the current pose differs
// from the last pose by MIN_POSE_DIFF amount
//float itsLastCompositeHeading, itsLastCompositeDepth, itsLastSpeed;

// Stores our currently desired dive value. Used to store intermediate dive values when
// as we dive in 4 stages in the beginning.
int itsDepthVal;
int itsDepthError;

// The current dive state we are on (1-4)
char itsDiveCount;

// Used to store the heading we want to use to go through the gate at the beginning of the mission.
int itsHeadingVal;
int itsHeadingError;

// Stores whether or not PID is currently enabled. Prevents us from sending multiple disable PID messages
// to the BeeStem in the evolve() loop.
bool itsPIDEnabled;

// Whether or not we should set a new speed in the control loop
bool itsSpeedEnabled;

// The last time weights were decayed
ros::Time itsLastDecayTime;

ros::Subscriber kill_switch_sub;
ros::ServiceClient driver_depth;
ros::ServiceClient driver_rpy;
//ros::ServiceClient driver_calibrate;
ros::Publisher driver_speed;

void enablePID()
{
  // enable PID
}

void disablePID()
{
  // set speed to 0
  // disable PID
}

void initSensorVotes()
{
  itsSensorVotes = std::vector<SensorVote>();
  itsSensorVotes.reserve(SENSOR_TYPE_SIZE);
  itsSensorVotes[PATH] = SensorVote(PATH);
  itsSensorVotes[FIRST_BUOY] = SensorVote(FIRST_BUOY);
  itsSensorVotes[SECOND_BUOY] = SensorVote(SECOND_BUOY);
  itsSensorVotes[LOCALIZATION] = SensorVote(LOCALIZATION);
}

void resetWeights()
{
  for(unsigned int i = 0; i < itsSensorVotes.size(); i++)
    {
      itsSensorVotes[i].heading.weight = 0.0;
      itsSensorVotes[i].depth.weight = 0.0;
    }
}

void resetSensorVoteValues()
{
  for(unsigned int i = 0; i < itsSensorVotes.size(); i++)
    {
      itsSensorVotes[i].heading.val = 0.0;
      itsSensorVotes[i].depth.val = 0.0;
    }
}

void decayWeights()
{
  for(unsigned int i = 0; i < itsSensorVotes.size(); i++)
    {
      SensorVote sv = itsSensorVotes[i];

      if(sv.heading.weight > 0.0)
	{
	  if(sv.heading.decay >= 0.0 && sv.heading.decay <= 1.0)
	    sv.heading.weight *= 1.0 - sv.heading.decay;
	}

      if(sv.depth.weight > 0.0)
	{
	  if(sv.depth.decay >= 0.0 && sv.depth.decay <= 1.0)
	    sv.depth.weight *= 1.0 - sv.depth.decay;
	}

      itsSensorVotes[i] = sv;
    }
}

// ######################################################################
void set_depth(int depth)
{
  seabee3_driver::SetDesiredDepth depth_srv;
  depth_srv.request.Mode = 1;
  depth_srv.request.Mask = 1;
  depth_srv.request.DesiredDepth = depth;

  if (driver_depth.call(depth_srv))
    {
      //ROS_INFO("Set Desired Depth: %d", depth_srv.response.CurrentDesiredDepth);
      itsDepthError = depth_srv.response.ErrorInDepth;
    }
  else
    {
      ROS_ERROR("Failed to call service setDesiredDepth");
    }
}

void set_speed(int speed)
{
  geometry_msgs::Twist cmd_vel;
  cmd_vel.linear.x = speed; //speed
  cmd_vel.linear.y = 0;
  cmd_vel.linear.z = 0;
  
  cmd_vel.angular.x = 0;
  cmd_vel.angular.y = 0;
  cmd_vel.angular.z = 0;

  driver_speed.publish(cmd_vel);
}

void set_heading(int heading)
{
  seabee3_driver::SetDesiredRPY rpy_srv;
  rpy_srv.request.Mode.x = 0;
  rpy_srv.request.Mode.y = 0;
  rpy_srv.request.Mode.z = 1;
  rpy_srv.request.Mask.x = 0;
  rpy_srv.request.Mask.y = 0;
  rpy_srv.request.Mask.z = 1;
  rpy_srv.request.DesiredRPY.x = 0;
  rpy_srv.request.DesiredRPY.y = 0;
  rpy_srv.request.DesiredRPY.z = heading;

  if (driver_rpy.call(rpy_srv))
    {
      //      ROS_INFO("Set Desired RPY: %f", rpy_srv.response.CurrentDesiredRPY.z);
      itsHeadingError = rpy_srv.response.ErrorInRPY.z;
    }
  else
    {
      ROS_ERROR("Failed to call service setDesiredRPY");
    }
}

void killSwitchCallback(const seabee3_driver_base::KillSwitchConstPtr & msg)
{
  itsKillSwitchState = msg->Value;
}

// ######################################################################
void state_init()
{

  // First stage of dive, should only happen once
  if(itsDepthVal == -1 && itsDiveCount < 4)
    {
      ROS_INFO("Doing initial dive");

      // Give diver 5 seconds to point sub at gate
      sleep(5);
      
      // Make sure other SensorVotes do not have weights
      // before going through gate
      resetWeights();

      // Set our desired heading to our current heading
      // and set a heading weight
      itsSensorVotes[PATH].heading.val = 0;
      itsSensorVotes[PATH].heading.weight = 1.0;


      // Set our desired depth to a fourth of itsGateDepth.
      // This is done because we are diving in 4 stages.
      // Also set a depth weight.
      itsDepthVal = itsGateDepth / 4;
      itsSensorVotes[PATH].depth.val = itsDepthVal;
      itsSensorVotes[PATH].depth.weight = 1.0;

      // Increment the dive stage we are on
      itsDiveCount++;

      // Indicate that PATH SensorVote vals have been set
      itsSensorVotes[PATH].init = true;

      enablePID();
    }
  // Dive States 2 through 4
  else if(itsDepthVal != -1 &&
          abs(itsDepthError) <= 4 && itsDiveCount < 4)
    {
      // Add an additional fourth of itsGateDepth to our desired depth
      itsDepthVal = itsGateDepth / 4;
      itsSensorVotes[PATH].depth.val = itsDepthVal;
      itsSensorVotes[PATH].depth.weight = 1.0;

      // increment the dive state
      itsDiveCount++;

      //ROS_INFO("Diving stage %d", itsDiveCount);

      //ROS_INFO("itsDepthVal: %d, itsErr: %d", itsDepthVal, abs(itsDepthVal - its_current_ex_pressure));
    }
  // After going through all 4 stages of diving, go forward through the gate
  else if(itsDepthVal != -1 &&
          abs(itsDepthError) <= 4 && itsDiveCount >= 4)
    {
      itsCurrentState = STATE_DO_GATE;

      ROS_INFO("State Init -> State Do Gate");
    }
}


// ######################################################################
void state_do_gate()
{
  ROS_INFO("Moving towards gate...");

  // Set speed to MAX_SPEED
  set_speed(MAX_SPEED);
  // Sleep for itsGateFwdTime
  sleep(itsGateTime);

  ROS_INFO("Finished going through gate...");

  // Start following saliency to go towards flare
  itsCurrentState = STATE_FIRST_BUOY;
}


// ######################################################################
void state_first_buoy()
{
  ROS_INFO("Hitting First Buoy...");

  // Enable speed based on heading and depth error
  if(!itsSpeedEnabled)
    itsSpeedEnabled = true;

  // Decay the weight we place on PATH SensorVote's heading
  itsSensorVotes[FIRST_BUOY].heading.decay = 0.005;
  //ROS_INFO("heading weight: %f",itsSensorVotes[BUOY].heading.weight);

  //check for state transition: i.e. buoy is large enough to be
  // considered a "hit" and update to next state
}

// ######################################################################
void state_second_buoy()
{
  ROS_INFO("Hitting Second Buoy");

  //itsCurrentState = STATE_DO_BARBWIRE;
}

int main(int argc, char** argv)
{
  itsCurrentState = STATE_INIT;
  itsKillSwitchState = 1;
  itsPoseError = -1.0;
  //itsLastCompositeHeading = 0.0;
  //itsLastCompositeDepth = 0.0;
  //  itsLastSpeed = 0.0;
  itsDepthVal = -1;
  itsDepthError = 0;
  itsDiveCount = 0;
  itsHeadingVal = -1;
  itsHeadingError = 0;
  itsPIDEnabled = false;
  itsSpeedEnabled = false;
  itsLastDecayTime = ros::Time(-1);
  // initialize SensorVotes
  initSensorVotes();
  disablePID();

  ros::init(argc, argv, "seabee3_mission_control");
  ros::NodeHandle n;

  // initialize command-line parameters
  n.param("gate_time", itsGateTime, 40.0);
  n.param("gate_depth", itsGateDepth, 85.0);
  n.param("heading_corr_scale", itsHeadingCorrScale, 125.0);
  n.param("depth_corr_scale", itsDepthCorrScale, 100.0);
  n.param("speed_corr_scale", itsSpeedCorrScale, 1.0);

  kill_switch_sub = n.subscribe("seabee3/KillSwitch", 100, killSwitchCallback);
	
  //driver_calibrate = n.serviceClient<xsens_node::CalibrateRPYOri>("xsens/CalibrateRPYOri");
  driver_depth = n.serviceClient<seabee3_driver::SetDesiredDepth>("seabee3/setDesiredDepth");
  driver_rpy = n.serviceClient<seabee3_driver::SetDesiredRPY>("seabee3/setDesiredRPY");
  driver_speed = n.advertise<geometry_msgs::Twist>("seabee3/cmd_vel", 1);

  while(ros::ok())
    {	  
      resetSensorVoteValues();

      // If the kill switch is active, act based on the sub's 
      // current state.
      if(itsKillSwitchState == 0)
	{
	  switch(itsCurrentState)
	    {
	    case STATE_INIT:
	      state_init();
	      break;
	    case STATE_DO_GATE:
	      state_do_gate();
	      break;
	    case STATE_FIRST_BUOY:
	      state_first_buoy();
	      break;
	    case STATE_SECOND_BUOY:
	      state_second_buoy();
	      break;
	    }

	  // only decay the weights every 1 second
	  if(itsLastDecayTime == ros::Time(-1) ||
	     (ros::Time::now() - itsLastDecayTime) > ros::Duration(1))
	    {	      
	      itsLastDecayTime = ros::Time::now();
	      decayWeights();
	    }
	      
	  // Calculate the composite heading and depth based on the SensorPose
	  // values and weights of the SensorVotes
	  float compositeHeading = 0.0;
	  int totalHeadingWeight = 0;
	      
	  float compositeDepth = 0.0;
	  float totalDepthWeight = 0.0;
	      
	  for(unsigned int i = 0; i < itsSensorVotes.size(); i++)
	    {
	      SensorVote sv = itsSensorVotes[i];
		  
	      compositeHeading += sv.heading.val * sv.heading.weight;
	      totalHeadingWeight += sv.heading.weight;
		  
	      compositeDepth += sv.depth.val * sv.depth.weight;
	      totalDepthWeight += sv.depth.weight;
	    }
	      
	  // If at lease one heading weight is set,
	  // calculate final heading and send it
	  if(totalHeadingWeight != 0.0)
	    {
		  
	      compositeHeading /= totalHeadingWeight;
		  
	      // make sure current heading differs from last heading by MIN_POSE_DIFF
	      //if(abs(itsLastCompositeHeading - compositeHeading) > MIN_POSE_DIFF)
	      set_heading(compositeHeading);
	    }
	      
	  // If at lease one depth weight is set,
	  // calculate final depth and send it
	  if(totalDepthWeight != 0.0)
	    {
	      compositeDepth /= totalDepthWeight;
		  
	      // make sure current depth differs from last depth by MIN_POSE_DIFF
	      //if(abs(itsLastCompositeDepth - compositeDepth) > MIN_POSE_DIFF)
	      set_depth(compositeDepth);
	    }
	      
	      
	  // Calculate the speed we should be going based on heading and depth error
	  int headingErr = compositeHeading - its_current_heading;
	  int depthErr = compositeDepth - its_current_ex_pressure;
	      
	  itsPoseError = sqrt((float)(headingErr*headingErr + depthErr*depthErr));
	      
	  float speedCorr = itsPoseError * itsSpeedCorrScale;
	      
	  // Make sure correction doesn't go above max speed
	  if(speedCorr > MAX_SPEED)
	    speedCorr = MAX_SPEED;
	      
	  // If setting speed is enabled, send corrected speed to BeeStemI
	  if(itsSpeedEnabled)
	    set_speed(MAX_SPEED - speedCorr);
	      
// 	  itsLastCompositeHeading = compositeHeading;
// 	  itsLastCompositeDepth = compositeDepth;
// 	  itsLastSpeed = MAX_SPEED - speedCorr;
	}
      else
	{
	  ROS_INFO("Waiting for kill switch...\n");
	  itsPIDEnabled = false;
	  disablePID();
	  
	  itsDepthVal = -1;
	  itsDepthError = 0;
	  itsHeadingVal = -1;
	  itsHeadingError = 0; 
	  itsCurrentState = STATE_INIT;
	}

      ros::Rate(20).sleep();
    }
	
  return 0;
}