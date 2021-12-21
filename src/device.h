/*
	device.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdint.h> 
#include "connection.h"
#include "mavPackageFilter.h"
#include "parameters.h"
#include "waypoints.h"

//FIFO
#include "RingBuf.h"

// for string
#include <string>
#include <iostream> // cout debug

// for Mavlink
#include "c_library_v1-master/common/mavlink.h" 
#include "c_library_v1-master/ardupilotmega/mavlink.h" 


#define MAXLINE 1024 
#define FIFO_SIZE 256

class Device
{
	// Public functions to be used on all Messages
	public:	
	Device(){}; 
	Device(int ConnectionPort, int ConnectionType, std::string name, Parameters *parameterList, Waypoints *waypointList); // Contructor for when starting a connection.
	Device(int fd, std::string name, Parameters *parameterList, Waypoints *waypointList); // Contructor for exsisting connection (TCP clients connects).
	Device(int ConnectionPort, int ConnectionType, std::string name); // Contructor for Device only using connection.
	
	virtual ~Device(){}; //destructor
	
//	virtual ~Device(){};			// Destructor is virtual to ensure correct destructor is used when deleting telegrams.	
	uint16_t service(void); 
	void sendMSG(mavlink_message_t data);
	mavlink_message_t getMSG(void);
	
	// Get and set for private connection:
	void setFD_SET(fd_set* fdset);
	void setFD(int fd);
	int getFD(void);
	std::string getName(void);
//	void startConnection(int fd, std::string name, Parameters *parameterList);
	
	// General helper functions and varibels only used by inherited classes.
	protected:
	//virtual void handleIncommingPackage(mavlink_message_t &msg)=0; 
	virtual void handleIncommingPackage(mavlink_message_t &msg)=0; 
//void sendBufferData(uint16_t length);
	void sendAllParameters(void);
	void sendAck(uint16_t mav_cmd, uint8_t mav_result);
	
	Connection myConnection;
	MavPackageFilter myFilter;
	Parameters *droneParameters=NULL;
	Waypoints *droneWaypoints=NULL;
 	uint8_t txBuffer[MAXLINE]; 

	// ID to use as sender.
	uint8_t system_id;
	uint8_t component_id;	
	
	// Parameters only used on mother class.
	private:
	uint8_t rxBuffer[MAXLINE]; 
	RingBuf<mavlink_message_t, FIFO_SIZE> rxFIFO;
	std::string _name = "";
	
	//Mavlink Parser
	mavlink_status_t status;
	mavlink_message_t msg;
	int chan = MAVLINK_COMM_0;
	uint8_t myMSGcount=0;

	
};

#endif /* DEVICE_H_ */