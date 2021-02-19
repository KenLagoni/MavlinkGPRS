/*
	DeviceClient.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "deviceClient.h" 

 DeviceClient::DeviceClient(int ConnectionPort, int ConnectionType, std::string name, Parameters *parameterList, Waypoints *waypointList) : Device(ConnectionPort, ConnectionType, name, parameterList, waypointList){
	this->myFilter.setAllPackageFilter(DISCARD); 
//	this->myFilter.setAllPackageFilter(FORWARD); // This gives write access from client to drone!!!!!!
//	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_HEARTBEAT, DISCARD); // Dischard Heartbeats from clients.
	
	
	// Handle request for parameters
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, HANDLE); // if Groundstation askes for Autopilot version.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST_LIST, HANDLE); // if Groundstation askes for mission count.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST, HANDLE);  // if Groundstation askes for mission number.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_PARAM_REQUEST_LIST, HANDLE);  // if Groundstation askes for parameters.
	
	// Lets see what happens if we send MSG 66 (MAVLINK_MSG_ID_REQUEST_DATA_STREAM) to drone: (this is a debug test.)
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_REQUEST_DATA_STREAM, FORWARD);  // if Groundstation askes for parameters. --- Nothing it apprears
	
	
	
	// Lets see what happens if we send MSG 66 (MAVLINK_MSG_ID_REQUEST_DATA_STREAM) to drone: (this is a debug test.)
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, FORWARD);  // if Groundstation askes for parameters. 
	//this->myFilter.setAllPackageFilter(FORWARD);  //DEBUG!!!!! FORWARD ALL MESSAGES
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, DISCARD);  
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_SYSTEM_TIME, FORWARD);	
	
	
	this->system_id=1;    // We are sending as Drone back to clients.
	this->component_id=1; // We are sending as Drone back to clients.
 }
 
 DeviceClient::DeviceClient(int fd, std::string name, Parameters *parameterList, Waypoints *waypointList) : Device(fd, name, parameterList, waypointList) {
	this->myFilter.setAllPackageFilter(DISCARD); 
//	this->myFilter.setAllPackageFilter(FORWARD); // This gives write access from client to drone!!!!!!
//	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_HEARTBEAT, DISCARD); // Dischard Heartbeats from clients.
	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, HANDLE); // if Groundstation askes for Autopilot version.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST_LIST, HANDLE); // if Groundstation askes for mission count.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST, HANDLE);  // if Groundstation askes for mission number. // this is replaced by MAVLINK_MSG_ID_MISSION_REQUEST_INT
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_PARAM_REQUEST_LIST, HANDLE);  // if Groundstation askes for parameters.
	
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST_INT, HANDLE);  // if Groundstation askes for mission, should reply with MISSION_ITEM_INT.
		
		
		
	// Lets see what happens if we send MSG 66 (MAVLINK_MSG_ID_REQUEST_DATA_STREAM) to drone: (this is a debug test.)
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_REQUEST_DATA_STREAM, FORWARD);  // if Groundstation askes for parameters. --- Nothing it apprears
	
	// Lets see what happens if we send MSG 66 (MAVLINK_MSG_ID_REQUEST_DATA_STREAM) to drone: (this is a debug test.)	
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, FORWARD);  // if Groundstation askes for parameters.
	//this->myFilter.setAllPackageFilter(FORWARD);  //DEBUG!!!!! FORWARD ALL MESSAGES
	//this->myFilter.setPackageFilter(MAVLINK_MSG_ID_SYSTEM_TIME, FORWARD);  
		
	///MISSION_REQUEST_INT We need to handle this

	this->system_id=1;    // We are sending as Drone back to clients.
	this->component_id=1; // We are sending as Drone back to clients.	 	 
 }
  
 void DeviceClient::handleIncommingPackage(mavlink_message_t &msg){

	printf("%s handling incoming package: MSG ID#%d  \n", this->getName().c_str(), msg.msgid);
/*
	if(msg.msgid == MAVLINK_MSG_ID_ATTITUDE){ // lets get the time since boot from this messages since it is very freqent)
		this->lastmsgTime=getTimeFromAttitudeMSG(msg);
	}
*/
	switch(msg.msgid)
	{
		case MAVLINK_MSG_ID_COMMAND_LONG: 
		{
			mavlink_command_long_t cmdLong;
			mavlink_msg_command_long_decode(&msg, &cmdLong);
			if(cmdLong.command == MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES){ // lets reply with Auto Pilot Version from memory :-)
				mavlink_message_t reply;
				mavlink_msg_autopilot_version_encode(this->system_id,this->component_id, &reply, &this->droneParameters->autoPilotVersion);
				this->sendMSG(reply);
				printf("CMND Long asking for Autopilot version, will also send all %d status messages\n", this->droneParameters->statusmsgCount);
				
				// send all status text:
				for(int msgCount=0;msgCount<this->droneParameters->statusmsgCount; msgCount++){					
					mavlink_message_t reply;
					mavlink_msg_statustext_encode(this->system_id,this->component_id, &reply, &this->droneParameters->statusmsg[msgCount]);
					this->sendMSG(reply);
				}
					

			}
		}
		break;
		
		case MAVLINK_MSG_ID_MISSION_REQUEST_LIST: 
		{
			mavlink_mission_request_list_t request;
			mavlink_msg_mission_request_list_decode(&msg, &request);
			
			mavlink_message_t reply;
			mavlink_mission_count_t missionCount;		
			missionCount.count=this->droneWaypoints->getNumberOfWaypoints();
			missionCount.target_component=request.target_component; 
			missionCount.target_system=request.target_system; 
			mavlink_msg_mission_count_encode(this->system_id,this->component_id, &reply, &missionCount);
			this->sendMSG(reply);
			printf("Mission Request list, mission count=%d\n",this->droneWaypoints->getNumberOfWaypoints());
		}
		break;

		case MAVLINK_MSG_ID_MISSION_REQUEST: 
		{
			mavlink_mission_request_int_t request;
			mavlink_msg_mission_request_int_decode(&msg, &request);
			mavlink_message_t reply;		
			mavlink_mission_item_t item = this->droneWaypoints->getWaypointByID(request.seq);
			mavlink_msg_mission_item_encode(this->system_id,this->component_id, &reply, &item);
			this->sendMSG(reply);
			printf("Mission Request ITEM=%d param1=%2.2f param2=%2.2f param3=%2.2f param4=%2.2f x=%2.6f y=%2.6f x=%2.6f seq=%d command=%d target_system=%d target_component=%d frame=%d current=%d autocontinue=%d\n",
							request.seq, item.param1, item.param2, item.param3, item.param4, item.x, item.y, item.z, item.seq, item.command, item.target_system, item.target_component, item.frame, item.current, item.autocontinue);			
			printf("Mission Request ITEM=%d\n",request.seq);
		}
		break;
		
		case MAVLINK_MSG_ID_PARAM_REQUEST_LIST: 
		{
			this->sendAllParameters();
			printf("Asking for parameters\n");
		}
		break;
/*
		case MAVLINK_MISSION_REQUEST_INT:
		{
			
		}
		break;
*/

		default:
		break;
	}	 
 }