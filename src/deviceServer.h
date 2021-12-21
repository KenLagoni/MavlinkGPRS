/*
	deviceServer.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef DEVICESERVER_H_
#define DEVICESERVER_H_

#include "device.h"

class DeviceServer : public Device
{
	// Public functions to be used on all Messages
	public:	
	DeviceServer(){}; 
	DeviceServer(int ConnectionPort, int ConnectionType, std::string name, Parameters *parameterList, Waypoints *waypointList, bool readOnlyMode); // Contructor for when starting a connection.
	~DeviceServer(){}; // Destructor.
	
	void timeoutService(void); // Ensure running of statemachine for re-request.	
	// General helper functions and varibels only used by inherited classes.
	protected:
	virtual void handleIncommingPackage(mavlink_message_t &msg); 
		
	// Parameters only used on mother class.
	private:
	void getAutopilotVersion(); // will send command #76 with code 520 to drone.
	void getMissionCount(void);
	void getMission(uint16_t index);
	void getParameterList(void);
	void getParameterByID(uint16_t id);
	
	enum DroneState_t{
	  INIT=0,
	  GET_VERSION,
	  GET_PARAMETERS,
	  GET_MISSIOM,
	  MISSION_REQUEST_LIST,
	  MISSION_REQUEST,
	  RUNNING
	};
	DroneState_t state=INIT;
	uint32_t time_boot_ms=0;
	uint32_t getTimeFromAttitudeMSG(mavlink_message_t &msg);
	uint32_t lastmsgTime=0;
	uint32_t nextPackageTimeout=0;

	uint16_t missionRequestCount=0;
	
	void setRunningModeFilter(void);
	bool readOnlyMode = false;
};

#endif /* DEVICESERVER_H_ */