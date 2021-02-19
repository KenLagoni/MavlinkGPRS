/*
	waypoints.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef WAYPOINTS_H_
#define WAYPOINTS_H_

#include "c_library_v1-master/common/mavlink.h" 
#include "c_library_v1-master/ardupilotmega/mavlink.h" 

#include <string>
#include <ctime> // for time / date
#include <iostream> // cout debug
#include "parameters.h" // for DataMode_t


class Waypoints
{
	// Public functions to be used on all Messages
	public:
	
	Waypoints(){}; 
	Waypoints(std::string filePath, std::string fileName, DataMode_t mode); 

    void addWaypoint(mavlink_mission_item_t msg);
	void addWaypoint(mavlink_message_t msg);
	void saveMission(void);
	
	mavlink_mission_item_t getWaypointByID(uint16_t id);
	uint16_t getNumberOfWaypoints(void);
	void setNumberOfWaypoints(uint16_t totalNumberOfWaypoints);
	DataMode_t getWaypointMode(void);
	uint16_t missionRequestCount=0;
	
	void reset(void);
	// General helper functions and varibels only used by inherited classes.
	protected:
			
	
	// Parameters only used on mother class.
	private:
	enum Wayppoint_t{
	  PARAM_1=0,
	  PARAM_2,
	  PARAM_3,
	  PARAM_4,
	  PARAM_X,
	  PARAM_Y,
	  PARAM_Z,
	  PARAM_SEQ,
	  PARAM_COMMAND,
	  PARAM_TARGET_SYSTEM,
	  PARAM_TARGET_COMPONENT,	  
	  PARAM_FRAME,
	  PARAM_CURRENT,
	  PARAM_AUTOTUNE
	};
	
	mavlink_mission_item_t waypoints[MAX_PARAMETERS];
	uint16_t totalWaypointCount = 0;	
		
	DataMode_t fileMode = READ_ONLY;
	std::string _filePath = "";
	std::string _fileName = "";
	
	void loadWaypoints(void);
	void saveWaypoints(void);
	
};

#endif /* WAYPOINTS_H_ */