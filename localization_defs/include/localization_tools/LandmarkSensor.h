#include <vector>
#include <localization_tools/LocalizationParticle.h>
#include <landmark_map/LandmarkMap.h>
#include <sonar_node/SonarScanArray.h>
#include <localization_tools/Util.h> //includes math.h

//used to convert sensor data into particle weights
template <typename _T>
class LandmarkSensor
{
public:

	struct SensorType
	{
		const static int buoy_finder = 		0; //buoy
		const static int sonar_sensor = 	1; //pinger
		const static int pipe_finder = 		2; //pipe
	};

	struct Flags
	{
		const static double voteFailed = -1;
	};

	LandmarkSensor(double scale)
	{
		mScale = scale;
		inited = false;
	}

	~LandmarkSensor()
	{
		//delete mVirtualMsg;
	}

	virtual void recordMessage(const _T& realMsg)
	{
		mRealMsg = realMsg;
		calculateInitialOffset();
		inited = true;
	}
	
	void recordVirtualMessage(const _T& virtualMsg)
	{
		mVirtualMsg = virtualMsg;
	}

	virtual void calculateInitialOffset() = 0;

	virtual double getScaledVote() = 0;

	static _T generatetVirtualSensorMessage ( const LocalizationParticle & part, const LandmarkMap & map );

	double mScale;
	_T mRealMsg;
	_T mVirtualMsg;
	double mVote;
	double mScaledVote; //value calculated from last reading
	int mSensorType;

	bool inited;
};

class SonarSensor : public LandmarkSensor<sonar_node::SonarScanArray>
{
public:
	SonarSensor(double scale) : 
	LandmarkSensor<sonar_node::SonarScanArray>(scale)
	{
		mSensorType = SensorType::sonar_sensor;
	}
	
	virtual void calculateInitialOffset()
	{
		//
	}
	
	virtual double getScaledVote()
	{
		mVote = 0;
		double error = 0.0;
		for(unsigned int i = 0; i < mRealMsg.ScanArray.size(); i ++)
		{
			error += fabs( mRealMsg.ScanArray[i].Distance - mVirtualMsg.ScanArray[i].Distance ) * fabs( LocalizationUtil::angleDistRel(mRealMsg.ScanArray[i].Heading, mVirtualMsg.ScanArray[i].Heading) );
		}
		
		mVote = error > 0.0 ? 1.0 / error : 1.0; //simple scaling of sensor vote with respect to error
		
		mScaledVote = mVote * mScale;
		return mScaledVote;
	}
	
	static sonar_node::SonarScanArray generateVirtualSensorMessage ( const LocalizationParticle & part, const LandmarkMap & map )
	{
		sonar_node::SonarScanArray result;
		std::vector<Landmark> landmarks = map.fetchLandmarksById(Landmark::LandmarkId::Pinger);
		for(int i = 0; i < landmarks.size(); i ++)
		{
			cv::Point2d theVector = LocalizationUtil::vectorTo(part.mState.mCenter, landmarks[i].mCenter);
			sonar_node::SonarScan theScan;
			theScan.Id = static_cast<LandmarkTypes::Pinger*>( &( landmarks[i] ) )->mId;
			
			theScan.Distance = theVector.x;
			theScan.Heading = theVector.y;
			
			result.ScanArray.push_back( theScan );
		}
		return result;
	}
};

/*class BuoyFinder: public LandmarkSensor
{
public:
	BuoyFinder() : 
	LandmarkSensor
	{
		mSensorType = LandmarkSensor::SensorType::detector_buoy;
	}

	BuoySensor(double scale)
	{
		mSensorType = LandmarkSensor::SensorType::detector_buoy;
		mScale = scale;
	}

	virtual void calculateInitialOffset()
	{
		//
	}

	virtual double getScaledVote()
	{
		if (mRealMsg->ice_isA("::RobotSimEvents::PipeColorSegmentMessage"))
		{
			RobotSimEvents::BuoyColorSegmentMessagePtr realMsg = RobotSimEvents::BuoyColorSegmentMessagePtr::dynamicCast(mRealMsg);
			VirtualSensorMessage::BuoyColorSegmentMessage * virtualMsg = static_cast<VirtualSensorMessage::BuoyColorSegmentMessage*>(mVirtualMsg);
			mVote = 1.0f;
			mScaledVote = mVote * mScale;
			return mScaledVote;
		}
		return Flags::voteFailed;
	}

	static VirtualSensorMessage::VirtualSensorMessage * generatetVirtualSensorMessage(LocalizationParticle & part, LocalizationMap & map)
	{
		VirtualSensorMessage::BuoyColorSegmentMessage * msg = new VirtualSensorMessage::BuoyColorSegmentMessage;
		return msg;
	}
};*/

/*class CompassSensor: public LandmarkSensor
{
public:
	double mInitialOffset;
	CompassSensor()
	{
		mSensorType = LandmarkSensor::SensorType::compass;
		mInitialOffset = 0;
	}

	CompassSensor(double scale)
	{
		mSensorType = LandmarkSensor::SensorType::compass;
		mInitialOffset = 0;
		mScale = scale;
	}

	virtual void calculateInitialOffset()
	{
		RobotSimEvents::IMUDataServerMessagePtr realMsg = RobotSimEvents::IMUDataServerMessagePtr::dynamicCast(mRealMsg);
		mInitialOffset = realMsg->orientation[2];
	}

	virtual double getScaledVote()
	{
		if (mRealMsg->ice_isA("::RobotSimEvents::IMUDataServerMessage"))
		{
			double realOrientation = 0.0f;
			double virtualOrientation = 0.0f;
			RobotSimEvents::IMUDataServerMessagePtr realMsg = RobotSimEvents::IMUDataServerMessagePtr::dynamicCast(mRealMsg);
			VirtualSensorMessage::IMUDataServerMessage * virtualMsg = static_cast<VirtualSensorMessage::IMUDataServerMessage*>(mVirtualMsg);
			realOrientation = realMsg->orientation[2] - mInitialOffset;
			virtualOrientation = virtualMsg->orientation[2];
			mVote = 1.0f - LocalizationUtil::linearAngleDiffRatio(realOrientation, virtualOrientation);
			mScaledVote = mVote * mScale;
			return mScaledVote;
		}
		return Flags::voteFailed;
	}

	static VirtualSensorMessage::VirtualSensorMessage * generatetVirtualSensorMessage(LocalizationParticle & part, LocalizationMap & map)
	{
		VirtualSensorMessage::IMUDataServerMessage * msg = new VirtualSensorMessage::IMUDataServerMessage;
		msg->orientation.resize(3);
		msg->orientation[2] = LocalizationUtil::polarToEuler(part.mState.mAngle); // conventional -> euler
		return msg;
	}
};*/

/*class PipeSensor: public LandmarkSensor
{
public:
	PipeSensor()
	{
		mSensorType = LandmarkSensor::SensorType::detector_pipe;
	}

	PipeSensor(double scale)
	{
		mSensorType = LandmarkSensor::SensorType::detector_pipe;
		mScale = scale;
	}

	virtual void calculateInitialOffset()
	{
		//
	}

	virtual double getScaledVote()
	{
		if (mRealMsg->ice_isA("::RobotSimEvents::PipeColorSegmentMessage"))
		{
			mVote = 0.0f;
			RobotSimEvents::PipeColorSegmentMessagePtr realMsg = RobotSimEvents::PipeColorSegmentMessagePtr::dynamicCast(mRealMsg);
			VirtualSensorMessage::PipeColorSegmentMessage * virtualMsg = static_cast<VirtualSensorMessage::PipeColorSegmentMessage*>(mVirtualMsg);
			if(virtualMsg->pipeIsVisible && realMsg->size > 1500) //make sure we see a decent size orange blob
			{
				mVote += realMsg->size / (2.0f * 8000.0f);
				if(mVote > 0.5f)
					mVote = 0.5f;
			}
			mVote += 0.5f;
			mScaledVote = mVote * mScale;
			return mScaledVote;
		}
		return Flags::voteFailed;
	}

	static VirtualSensorMessage::VirtualSensorMessage * generatetVirtualSensorMessage(LocalizationParticle & part, LocalizationMap & map)
	{
		VirtualSensorMessage::PipeColorSegmentMessage * msg = new VirtualSensorMessage::PipeColorSegmentMessage;
		msg->pipeIsVisible = false;
		for(unsigned int i = 0; i < map.mMapEntities.size(); i ++)
		{
			if(map.mMapEntities[i].mObjectType == LocalizationMapEntity::ObjectType::pipe)
			{
				double distToPipe = sqrt(pow(part.mState.mPoint.i - map.mMapEntities[i].mCenter.i, 2) + pow(part.mState.mPoint.j - map.mMapEntities[i].mCenter.j, 2));
				double largerLength = map.mMapEntities[i].mDim.i;
				if(map.mMapEntities[i].mDim.j > largerLength)
				{
					largerLength = map.mMapEntities[i].mDim.j;
				}
				if(distToPipe <= largerLength / 2)
				{
					msg->pipeIsVisible = true;
				}
			}
		}
		return msg;
	}
};*/

/*class RectangleSensor: public LandmarkSensor
{
public:
	RectangleSensor()
	{
		mSensorType = LandmarkSensor::SensorType::detector_pipe;
	}

	RectangleSensor(double scale)
	{
		mSensorType = LandmarkSensor::SensorType::detector_pipe;
		mScale = scale;
	}

	virtual void calculateInitialOffset()
	{
		//
	}

	virtual double getScaledVote()
	{
		if (mRealMsg->ice_isA("::RobotSimEvents::VisionRectangleMessage"))
		{
			RobotSimEvents::VisionRectangleMessagePtr realMsg = RobotSimEvents::VisionRectangleMessagePtr::dynamicCast(mRealMsg);
			VirtualSensorMessage::VisionRectangleMessage * virtualMsg = static_cast<VirtualSensorMessage::VisionRectangleMessage*>(mVirtualMsg);
			int realOrientation = 0; //fuck it I'm just gonna average all of them
			for(unsigned int i = 0; i < realMsg->quads.size(); i ++)
			{
				realOrientation += LocalizationUtil::eulerToPolar(realMsg->quads[i].angle);
			}
			realOrientation /= realMsg->quads.size();
			mVote = 0;
			if(realMsg->quads.size() > 0)
			{
				mVote += virtualMsg->rectangleDistance <= 20 ? 0.5 * -(virtualMsg->rectangleDistance - 20) / 20 : 0;
				if(virtualMsg->rectangleIsVisible)
				{
					mVote += 0.5 - 0.5 * LocalizationUtil::linearAngleDiffRatio(realOrientation, virtualMsg->rectangleRelOrientation);
				}
			}
			mScaledVote = mVote * mScale;
			return mScaledVote;
		}
		return Flags::voteFailed;
	}

	static VirtualSensorMessage::VirtualSensorMessage * generatetVirtualSensorMessage(LocalizationParticle & part, LocalizationMap & map)
	{
		VirtualSensorMessage::VisionRectangleMessage * msg = new VirtualSensorMessage::VisionRectangleMessage;
		msg->rectangleIsVisible = false;
		int closestRectIndex = -1;
		double closestRectDist = -1;
		for(unsigned int i = 0; i < map.mMapEntities.size(); i ++)
		{
			if((map.mMapEntities[i].mShapeType == LocalizationMapEntity::ShapeType::rect || map.mMapEntities[i].mShapeType == LocalizationMapEntity::ShapeType::square) && map.mMapEntities[i].mInteractionType != LocalizationMapEntity::InteractionType::external)
			{
				double distToRect = sqrt(pow(part.mState.mPoint.i - map.mMapEntities[i].mCenter.i, 2) + pow(part.mState.mPoint.j - map.mMapEntities[i].mCenter.j, 2));
				double largerLength = map.mMapEntities[i].mDim.i;
				if(map.mMapEntities[i].mDim.j > largerLength)
				{
					largerLength = map.mMapEntities[i].mDim.j;
				}
				if(distToRect <= largerLength / 2)
				{
					msg->rectangleIsVisible = true;
				}
				if(distToRect >= closestRectDist)
				{
					closestRectDist = distToRect;
					closestRectIndex = i;
				}
			}
		}
		msg->rectangleDistance = closestRectDist;
		msg->rectangleRelOrientation = map.mMapEntities[closestRectIndex].mOrientation;
		return msg;
	}
};*/
