/***************************************************************************
 *  include/base_libs/joystick_policy.h
 *  --------------------
 * 
 *  Copyright (c) 2011, Edward T. Kaszubski ( ekaszubski@gmail.com )
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of seabee3-ros-pkg nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 **************************************************************************/

#ifndef BASE_LIBS_BASE_LIBS_JOYSTICK_POLICY_H_
#define BASE_LIBS_BASE_LIBS_JOYSTICK_POLICY_H_

#include <base_libs/node_handle_policy.h>
#include <base_libs/multi_publisher.h>
#include <base_libs/multi_subscriber.h>
#include <geometry_msgs/Twist.h>
#include <joy/Joy.h>

namespace base_libs
{

BASE_LIBS_DECLARE_POLICY( Joystick, NodeHandlePolicy )

BASE_LIBS_DECLARE_POLICY_CLASS( Joystick )
{
	BASE_LIBS_MAKE_POLICY_NAME( Joystick )

public:
	ros::MultiPublisher<> multi_pub_;
	ros::MultiSubscriber<> multi_sub_;
	
	BASE_LIBS_DECLARE_POLICY_CONSTRUCTOR( Joystick )
	{
		printPolicyActionStart( "create", this );
		
		preInit();
		
		printPolicyActionDone( "create", this );
	}
	
	void preInit()
	{
		ros::NodeHandle & nh_rel = NodeHandlePolicy::getNodeHandle();
		
		multi_pub_.addPublishers<geometry_msgs::Twist>( nh_rel, { "cmd_vel" } );
		multi_sub_.addSubscriber( nh_rel, "joystick", &JoystickPolicy::joystickCB, this );
		
		auto linear_axis_scales = ros::ParamReader<double, 3>::readParams( nh_rel, "linear_axis", "_scale" );
		auto angular_axis_scales = ros::ParamReader<double, 3>::readParams( nh_rel, "angular_axis", "_scale" );
		
		auto axis_indices = ros::ParamReader<double, 6>::readParams( nh_rel, "axis", "_index" );
	}

	void joystickCB( const joy::Joy::ConstPtr & msg )
	{
		
	}
};

}

#endif // BASE_LIBS_BASE_LIBS_JOYSTICK_POLICY_H_
