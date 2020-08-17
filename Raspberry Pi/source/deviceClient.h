/*
	deviceClient.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef DEVICECLIENT_H_
#define DEVICECLIENT_H_

#include "device.h"

class DeviceClient : public Device
{
	// Public functions to be used on all Messages
	public:	
	DeviceClient(){}; 
	DeviceClient(int ConnectionPort, int ConnectionType, std::string name, Parameters *parameterList, Waypoints *waypointList); // Contructor for when starting a connection.
	DeviceClient(int fd, std::string name, Parameters *parameterList, Waypoints *waypointList); // Contructor for exsisting connection (TCP clients connects).
	
	~DeviceClient(){};	// Destructor 
	
	
	// General helper functions and varibels only used by inherited classes.
	protected:
	virtual void handleIncommingPackage(mavlink_message_t &msg); 
		
	// Parameters only used on mother class.
	private:
};

#endif /* DEVICECLIENT_H_ */