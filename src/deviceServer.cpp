/*
	deviceServer.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "deviceServer.h" 

 DeviceServer::DeviceServer(int ConnectionPort, int ConnectionType, std::string name, Parameters *parameterList, Waypoints *waypointList) : Device(ConnectionPort, ConnectionType, name, parameterList, waypointList){
	this->myFilter.setAllPackageFilter(DISCARD); // Server will only look for Attitude MSG (using time since boot). It should download mission before sending any data to clients (FORWARD)
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_ATTITUDE, HANDLE);
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_AUTOPILOT_VERSION, HANDLE);	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_STATUSTEXT, HANDLE);
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_PARAM_VALUE, HANDLE);
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_COUNT, HANDLE);	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_ITEM, HANDLE);	
	
	
	this->state=INIT;
	this->system_id=0xFF;    // We are sending as Server.
	this->component_id=0xBE; // We are sending as Server.	
 }
 
 void DeviceServer::timeoutService(void){
	 
 }
 
 void DeviceServer::setRunningModeFilter(void){
	 // lets just forward all packages from drone to clients.
	this->myFilter.setAllPackageFilter(FORWARD); 	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_ATTITUDE, INSPECT);	// to look for reboot
/*	 
	this->myFilter.setAllPackageFilter(DISCARD); 	
	// Packages which goes to clients directly from drone when in running mode.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_HEARTBEAT, FORWARD);
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_ATTITUDE, FORWARD);	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_BATTERY_STATUS, FORWARD);		
	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_BATTERY_STATUS, FORWARD);			
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_CURRENT, FORWARD);		
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_VFR_HUD, FORWARD);		
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_GLOBAL_POSITION_INT, FORWARD);			
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_RC_CHANNELS_RAW, FORWARD);	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_SYS_STATUS, FORWARD);	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_GPS_RAW_INT, FORWARD);
//	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_STATUSTEXT, HANDLE);
*/
 }
 
 void DeviceServer::handleIncommingPackage(mavlink_message_t &msg){
 
	// always check system uptime and see if we should restart statmachine.
	
	// check time from Attitude
	if(msg.msgid == MAVLINK_MSG_ID_ATTITUDE){ // lets get the time since boot from this messages since it is very freqent)
		if(getTimeFromAttitudeMSG(msg) < this->lastmsgTime){
			printf("Reboot detected!, current time=%d new time=%d Going to INIT state now.\n",this->lastmsgTime,getTimeFromAttitudeMSG(msg));
			this->myFilter.setAllPackageFilter(DISCARD); // Server will only look for Attitude MSG (using time since boot). It should download mission before sending any data to clients (FORWARD)
			this->myFilter.setPackageFilter(MAVLINK_MSG_ID_ATTITUDE, HANDLE);
			this->myFilter.setPackageFilter(MAVLINK_MSG_ID_AUTOPILOT_VERSION, HANDLE);
			this->myFilter.setPackageFilter(MAVLINK_MSG_ID_STATUSTEXT, HANDLE);
			this->myFilter.setPackageFilter(MAVLINK_MSG_ID_PARAM_VALUE, HANDLE);
			this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_COUNT, HANDLE);
			this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_ITEM, HANDLE);			
			this->state=INIT;
			this->droneWaypoints->reset();
			this->droneParameters->reset();
			this->droneParameters->statusmsgCount=0;
		}
		this->lastmsgTime=getTimeFromAttitudeMSG(msg);
	}else if(msg.msgid == MAVLINK_MSG_ID_STATUSTEXT){ 
		mavlink_statustext_t statusText;
		mavlink_msg_statustext_decode(&msg, &statusText);	
		this->droneParameters->statusmsg[this->droneParameters->statusmsgCount] = statusText;
		if(this->droneParameters->statusmsgCount<MAX_STATUS_TEXTS)
			this->droneParameters->statusmsgCount++;
		printf("Total Number of Status messages #253=%d\n",this->droneParameters->statusmsgCount); 
	}
 
	switch(this->state)
	{
	  case INIT: 
	  {
		printf("MSG from Drone, we are in INIT face. before BootTime=%d",this->time_boot_ms); 
		if(this->lastmsgTime>0){
			this->time_boot_ms=this->lastmsgTime; 
			this->getAutopilotVersion();
			this->nextPackageTimeout=this->lastmsgTime+1000;
			this->state=GET_VERSION;
		}
		printf(" After BootTime=%d\n",this->time_boot_ms); 
	  }
	  break;
	  
	  case GET_VERSION: 		
	  {
		if(msg.msgid == MAVLINK_MSG_ID_AUTOPILOT_VERSION){
			mavlink_msg_autopilot_version_decode(&msg, &this->droneParameters->autoPilotVersion);			
			printf("AutoPilot version: capabilities=%ld uid=%ld flight_sw_version=%d middleware_sw_version=%d os_sw_version=%d board_version=%d vendor_id=%d product_id=%d\n", 
					this->droneParameters->autoPilotVersion.capabilities,
					this->droneParameters->autoPilotVersion.uid,
					this->droneParameters->autoPilotVersion.flight_sw_version,
					this->droneParameters->autoPilotVersion.middleware_sw_version,
					this->droneParameters->autoPilotVersion.os_sw_version,
					this->droneParameters->autoPilotVersion.board_version,
					this->droneParameters->autoPilotVersion.vendor_id,
					this->droneParameters->autoPilotVersion.product_id);
			
			// lets check if we should download parameters: if ask for them and got to GET_PARAMETERS. If not Check mission.
			if(this->droneParameters->getParameterMode() == UPDATE){
				printf("Downloading all parameters from Drone...\n");
				this->getParameterList(); // Send Parameter List request.
				this->nextPackageTimeout=this->lastmsgTime+5000;				
				this->state=GET_PARAMETERS;
			}else if(this->droneWaypoints->getWaypointMode() == UPDATE){
				printf("Requisting mission from drone...\n");
				this->state=GET_MISSIOM;
			}else{
				printf("Using cached mission and parameters, thus going to RUNNING mode now\n");					
				this->setRunningModeFilter();
				this->state=RUNNING;				
			}
		}else{
			if(this->lastmsgTime>this->nextPackageTimeout){
				this->nextPackageTimeout=this->lastmsgTime+1000;
				this->getAutopilotVersion();	// resend requrst.
				printf("Re-requisting AutoPilot version!\n");
			}
		}
	  }
	  break;

	  case GET_PARAMETERS:  
	  {
		if(msg.msgid == MAVLINK_MSG_ID_PARAM_VALUE){
			mavlink_param_value_t parameter;
			mavlink_msg_param_value_decode(&msg, &parameter);						
			
//			printf("Parameter received MSG ID#%d parameter number=%d total number of parameters=%d parameter string:%s:\n", msg.msgid, parameter.param_index, parameter.param_count, parameter.param_id);
			printf("Parameter received MSG ID#%d parameter number=%d total number of parameters=%d parameter string:", msg.msgid, parameter.param_index, parameter.param_count);
			// Handle that param_id string (char array9 will not have any NULL termination if all 16 chars are used.
			int a=0;
			for(a=0;a<17;a++){
				if(parameter.param_id[a]==0){
					break;
				}
				else{
					printf("%c",parameter.param_id[a]);
				}
			}
			/*
			if(a==17){
				printf(":");	
				for(int b =0;b<17;b++)
					printf(" %02d", parameter.param_id[b]);
			}
			*/
			printf(":\n");	
			
			
			
			this->droneParameters->addParameter(parameter);
			int16_t missingParameter = this->droneParameters->getFirstMissingParameter();
			if(missingParameter == -1){
				if(this->droneWaypoints->getWaypointMode() == UPDATE){
					printf("Requisting mission from drone...\n");
					this->state=GET_MISSIOM;				
				}else{
					printf("Using cached mission, thus goint to RUNNING mode now\n");	
					this->setRunningModeFilter();					
					this->state=RUNNING;									
				}
			}
		}else{
			if(this->lastmsgTime>this->nextPackageTimeout){
				this->nextPackageTimeout=this->lastmsgTime+1000;
				int16_t missingParameter = this->droneParameters->getFirstMissingParameter();
				if(missingParameter == -1){ // we must be done then.				
					printf("Done Downloading all parameters!\n");	
					//if(this->droneParameters->getParameterMode() == UPDATE)					
					this->state=GET_MISSIOM;
				//	this->state=RUNNING;
				}else{ 
					if(missingParameter < 100){ // ask for full list again.
						this->nextPackageTimeout=this->lastmsgTime+5000;	
						printf("Only %d parameters downloaded, Re-requisting FULL Parameter list\n",missingParameter);						
						this->getParameterList(); // Send Parameter List request.
					}else{
						this->getParameterByID((uint16_t)missingParameter);
						printf("Re-requisting Parameter number=%d\n", (uint16_t)missingParameter);						
					}
				}
			}
		}
	  }  
	  break;  

	  case GET_MISSIOM:  
	  {
		this->getMissionCount();
		this->nextPackageTimeout=this->lastmsgTime+1000;
		this->state=MISSION_REQUEST_LIST;		  
	  }	  
	  break;  
	  
	  case MISSION_REQUEST_LIST: 
	  {
		if(msg.msgid == MAVLINK_MSG_ID_MISSION_COUNT ){		
			mavlink_mission_count_t missionCount;
			mavlink_msg_mission_count_decode(&msg, &missionCount);			
			printf("Mission Count received! - target_system=%d target_component=%d mission_count=%d\n",missionCount.target_system, missionCount.target_component, missionCount.count);
			this->droneWaypoints->setNumberOfWaypoints(missionCount.count);
			if(missionCount.count > 0){
				this->droneWaypoints->missionRequestCount=0;
				this->getMission(this->droneWaypoints->missionRequestCount);  // ask for first mission item.
				this->nextPackageTimeout=this->lastmsgTime+1000;
				this->state=MISSION_REQUEST;
			}else{ // no mission to download		
				// Set filter!
				this->setRunningModeFilter();
				this->state=RUNNING;
				printf("No mission to download, go to running mode!\n"); 				
			}
		}else{
			if(this->lastmsgTime>this->nextPackageTimeout){
				this->nextPackageTimeout=this->lastmsgTime+1000;
				this->getMissionCount();	// resend requrst.
				printf("Re-requisting Mission Count!\n");
			}
		}			  
	  }
	  break;
	  
	  case MISSION_REQUEST: 
	  {
		if(msg.msgid == MAVLINK_MSG_ID_MISSION_ITEM){		
			mavlink_mission_item_t missionItem;
			mavlink_msg_mission_item_decode(&msg, &missionItem);			
			printf("Mission ITEM=%d param1=%2.2f param2=%2.2f param3=%2.2f param4=%2.2f x=%2.6f y=%2.6f x=%2.6f seq=%d command=%d target_system=%d target_component=%d frame=%d current=%d autocontinue=%d\n",
					missionItem.seq, missionItem.param1, missionItem.param2, missionItem.param3, missionItem.param4, missionItem.x, missionItem.y, missionItem.z, missionItem.seq, missionItem.command, missionItem.target_system,
					missionItem.target_component, missionItem.frame, missionItem.current, missionItem.autocontinue);						
			if(missionItem.seq >= this->droneWaypoints->getNumberOfWaypoints()-1){
				this->droneWaypoints->addWaypoint(missionItem); /// remember to safe last waypoint :-)
				printf("Mission Download Completed! - Now go to running!\n");
				this->droneWaypoints->saveMission();
				this->setRunningModeFilter();
								
				// print the mission:
				printf("Printing mission\n");
				this->state=RUNNING;
				// Done!
			}else{
				if(this->droneWaypoints->missionRequestCount == missionItem.seq){ // we got the one we asked for
					this->droneWaypoints->addWaypoint(missionItem);
					this->droneWaypoints->missionRequestCount++;
					this->getMission(this->droneWaypoints->missionRequestCount);  // ask for next mission item.
					this->nextPackageTimeout=this->lastmsgTime+1000;					
				}				
			}
		}else{
			if(this->lastmsgTime>this->nextPackageTimeout){
				this->nextPackageTimeout=this->lastmsgTime+1000;
				this->getMission(this->droneWaypoints->missionRequestCount);	// resend requrst.
				printf("Re-requisting Mission Item %d!\n",this->droneWaypoints->missionRequestCount);
			}
		}			  		  
	  }
	  break;
	  
	  case RUNNING: 
	  {
		  
	  }
	  break;
	  	  
 	  default: 
	  break;
	}         	 
 }
 
 
void DeviceServer::getParameterByID(uint16_t id){
	mavlink_message_t msg;
	mavlink_param_request_read_t parmmsg;	
	parmmsg.param_index=((int16_t)id); // -1 means use PARAM field, but we will always use ID number.
	parmmsg.target_system=1;
	parmmsg.target_component=0;
	mavlink_msg_param_request_read_encode(this->system_id,this->component_id, &msg, &parmmsg);
	this->sendMSG(msg);
 } 
	

void DeviceServer::getParameterList(void){
	mavlink_message_t msg;
	mavlink_param_request_list_t parmmsg;	
	parmmsg.target_system=1;
	parmmsg.target_component=0;
	mavlink_msg_param_request_list_encode(this->system_id,this->component_id, &msg, &parmmsg);
	this->sendMSG(msg);
 } 
 

void DeviceServer::getMission(uint16_t index){
	mavlink_message_t msg;
	mavlink_mission_request_t missionItem;	
	missionItem.seq=index;
	missionItem.target_system=1;
	missionItem.target_component=0;
	mavlink_msg_mission_request_encode(this->system_id,this->component_id, &msg, &missionItem);
	this->sendMSG(msg);
	/*
	uint16_t totalLength = mavlink_msg_to_send_buffer(this->txBuffer, &msg); 
	this->sendBufferData(totalLength);
	*/
 } 
 
 
void DeviceServer::getMissionCount(void){
	mavlink_message_t msg;
	mavlink_mission_request_list_t missionmsg;
	missionmsg.target_system=1;
	missionmsg.target_component=0;
	mavlink_msg_mission_request_list_encode(this->system_id,this->component_id, &msg, &missionmsg);
	this->sendMSG(msg);
	/*
	uint16_t totalLength = mavlink_msg_to_send_buffer(this->txBuffer, &msg); 
	this->sendBufferData(totalLength);
	*/
 } 
 
 
void DeviceServer::getAutopilotVersion(void){
	mavlink_message_t msg;
	mavlink_command_long_t cmdLong;
	cmdLong.param1=1;
	cmdLong.param2=0;
	cmdLong.param3=0;
	cmdLong.param4=0;
	cmdLong.param5=0;
	cmdLong.param6=0;
	cmdLong.param7=0;
	cmdLong.target_system=1;
	cmdLong.target_component=0;
	cmdLong.confirmation=0;
	cmdLong.command=MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES;
	mavlink_msg_command_long_encode(this->system_id,this->component_id, &msg, &cmdLong);
	this->sendMSG(msg);
	/*
	uint16_t totalLength = mavlink_msg_to_send_buffer(this->txBuffer, &msg); 
	this->sendBufferData(totalLength);
	*/
 } 
 
uint32_t DeviceServer::getTimeFromAttitudeMSG(mavlink_message_t &msg){ // msg must be attitude! check before this call.
	mavlink_attitude_t attitudemsg;
    mavlink_msg_attitude_decode(&msg, &attitudemsg);
	return attitudemsg.time_boot_ms;
 }