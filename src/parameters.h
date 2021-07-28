/*
	parameters.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "c_library_v1-master/common/mavlink.h" 
#include "c_library_v1-master/ardupilotmega/mavlink.h" 

#include <string>
#include <ctime> // for time / date
#include <iostream> // cout debug

#define MAX_PARAMETERS 4000 
#define MAX_STATUS_TEXTS 256 

enum DataMode_t{
  READ_ONLY=0,
  UPDATE
};

class Parameters
{
	// Public functions to be used on all Messages
	public:
	
	Parameters(); 
	Parameters(std::string filePath, std::string fileName, DataMode_t mode); 

   	void addParameter(mavlink_message_t msg);
	void addParameter(mavlink_param_value_t msg);
	
	mavlink_param_value_t getParameterByID(uint16_t id);
	uint16_t getNumberOfParameters(void);
	int16_t getFirstMissingParameter(void);
	int16_t getMissingParameters(void);
	DataMode_t getParameterMode(void);
	
	mavlink_autopilot_version_t autoPilotVersion;
	
	// General helper functions and varibels only used by inherited classes.
	
	// should be moved to a waypoint file:
	uint16_t missionCount=0;
	mavlink_mission_item_t waypoints[512];
	mavlink_statustext_t statusmsg[MAX_STATUS_TEXTS];
	uint16_t statusmsgCount=0;
	void reset(void);
	
	protected:
			
	
	// Parameters only used on mother class.
	private:
	enum Parameter_t{
	  PARAM_VALUE=0,
	  PARAM_COUNT,
	  PARAM_INDEX,
	  PARAM_ID,
	  PARAM_TYPE
	};
	
	bool compareParameters(mavlink_param_value_t *msg1, mavlink_param_value_t *msg2);	//true of they are the same.

	
	mavlink_param_value_t parameters[MAX_PARAMETERS];
	uint16_t totalParameterCount = 0;	
		
	DataMode_t fileMode = READ_ONLY;
	std::string _filePath = "";
	std::string _fileName = "";

	void loadParameters(void);
	void saveParameters(void);
	
};

#endif /* PARAMETERS_H_ */
