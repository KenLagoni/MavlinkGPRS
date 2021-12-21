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
	this->myFilter.setAllPackageFilter(HANDLE);  
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_HEARTBEAT, DISCARD); // Dischard Heartbeats from clients.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_REQUEST_DATA_STREAM, DISCARD); // Dischard Obselete MSG to reqest data stream

	 /*
	this->myFilter.setAllPackageFilter(DISCARD); 
//	this->myFilter.setAllPackageFilter(FORWARD); // This gives write access from client to drone!!!!!!
//	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_HEARTBEAT, DISCARD); // Dischard Heartbeats from clients.
	
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_COMMAND_LONG, HANDLE); // if Groundstation askes for Autopilot version.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST_LIST, HANDLE); // if Groundstation askes for mission count.
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_MISSION_REQUEST, HANDLE);  // if Groundstation askes for mission number. // this is replaced by MAVLINK_MSG_ID_MISSION_REQUEST_INT
	this->myFilter.setPackageFilter(MAVLINK_MSG_ID_PARAM_REQUEST_LIST, HANDLE);  // if Groundstation askes for parameters.
	*/
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

	printf("Client: Handling incoming package from %s with MSG ID#%d", this->getName().c_str(), msg.msgid);

	// allign output text:
	if(msg.msgid<10){
		printf("   - ");
	}else if(msg.msgid<100){
		printf("  - ");
	}else{
		printf(" - ");
	}

/*
	if(msg.msgid == MAVLINK_MSG_ID_ATTITUDE){ // lets get the time since boot from this messages since it is very freqent)
		this->lastmsgTime=getTimeFromAttitudeMSG(msg);
	}
*/
	switch(msg.msgid)
	{
		case MAVLINK_MSG_ID_HEARTBEAT: // #0
		{
			printf("HEARTBEAT from groundstation -> ignored.\n");
		}
		break;

		case MAVLINK_MSG_ID_SYSTEM_TIME: // #2
		{
			mavlink_system_time_t inMSG;
			mavlink_msg_system_time_decode(&msg, &inMSG);
			printf("System Time from groundstation - msg: time_unix_usec(%llu) time_boot_ms(%u)\n", inMSG.time_unix_usec, inMSG.time_boot_ms);
		}
		break;


		case MAVLINK_MSG_ID_MISSION_REQUEST: // #40 - DEPRECATED: Replaced by MISSION_REQUEST_INT (2020-06). A system that gets this request should respond with MISSION_ITEM_INT (as though MISSION_REQUEST_INT was received).
		{
			mavlink_mission_request_t inMSG;
			mavlink_msg_mission_request_decode(&msg, &inMSG);
			
			mavlink_message_t reply;	

			/* OLD Rwply!
			mavlink_mission_request_int_t request;
			mavlink_msg_mission_request_int_decode(&msg, &request);
			mavlink_message_t reply; // #39 - DEPRECATED: Replaced by MISSION_ITEM_INT (2020-06).		
			mavlink_mission_item_t item = this->droneWaypoints->getWaypointByID(request.seq);
			mavlink_msg_mission_item_encode(this->system_id,this->component_id, &reply, &item);
			this->sendMSG(reply);
			printf("Mission Request ITEM=%d param1=%2.2f param2=%2.2f param3=%2.2f param4=%2.2f x=%2.6f y=%2.6f x=%2.6f seq=%d command=%d target_system=%d target_component=%d frame=%d current=%d autocontinue=%d\n",
							request.seq, item.param1, item.param2, item.param3, item.param4, item.x, item.y, item.z, item.seq, item.command, item.target_system, item.target_component, item.frame, item.current, item.autocontinue);			
			printf("Mission Request ITEM=%d\n",request.seq);
			*/
		}
		break;

		case MAVLINK_MSG_ID_MISSION_REQUEST_LIST: // #43
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

		case MAVLINK_MSG_ID_MISSION_REQUEST_INT: // #51
		{
			mavlink_mission_request_int_t inMSG;
			mavlink_msg_mission_request_int_decode(&msg, &inMSG);
			
			mavlink_message_t reply;	
				
			mavlink_mission_item_int_t item = this->droneWaypoints->getMissionItemIntByID(inMSG.seq); // #73 - MAVLINK_MSG_ID_MISSION_ITEM_INT
			mavlink_msg_mission_item_int_encode(this->system_id,this->component_id, &reply, &item);
			this->sendMSG(reply);

			printf("Mission Request int - msg: seq(%u) target_system(%u) target_component(%u)\n", inMSG.seq, inMSG.target_system, inMSG.target_component);
			printf("Reply to Client with: Mission Request ITEM=%d param1=%2.2f param2=%2.2f param3=%2.2f param4=%2.2f x=%d y=%d z=%2.6f seq=%d command=%d target_system=%d target_component=%d frame=%d current=%d autocontinue=%d\n",
							inMSG.seq, item.param1, item.param2, item.param3, item.param4, item.x, item.y, item.z, item.seq, item.command, item.target_system, item.target_component, item.frame, item.current, item.autocontinue);			
			
			/*
			uint16_t seq; 		      // <  Sequence
 			uint8_t target_system;    // <  System ID
 			uint8_t target_component; // <  Component ID
			*/
		}
		break;
	
	 

		case MAVLINK_MSG_ID_REQUEST_DATA_STREAM: // #66
		{
			mavlink_request_data_stream_t inMSG;
			mavlink_msg_request_data_stream_decode(&msg, &inMSG);
			printf("REQUEST_DATA_STREAM - Obsolete msg: req_message_rate(%u) target_system(%u) target_component(%u) req_stream_id(%u) start_stop(%u) \n",
															inMSG.req_message_rate,inMSG.target_system, 
															inMSG.target_component, inMSG.req_stream_id, inMSG.start_stop );

			/*
			uint16_t req_message_rate; 	// < [Hz] The requested message rate
 			uint8_t target_system; 	    // <  The target requested to send the message stream.
 			uint8_t target_component;   // <  The target requested to send the message stream.
 			uint8_t req_stream_id;      // <  The ID of the requested data stream
 			uint8_t start_stop; 	    // <  1 to start sending, 0 to stop sending.
			*/
		}
		break;

		case MAVLINK_MSG_ID_COMMAND_LONG: // #76
		{

			mavlink_command_long_t cmdLong;
			mavlink_msg_command_long_decode(&msg, &cmdLong);
			printf("CMND Long: command=%u, confirmation=%u, param1=%.1f, param2=%.1f, param3=%.1f, param4=%.1f, param5=%.1f, param6=%.1f, param7=%.1f - ", cmdLong.command , cmdLong.confirmation, 
																		cmdLong.param1, cmdLong.param2, cmdLong.param3, cmdLong.param4, cmdLong.param5, cmdLong.param6, cmdLong.param7);
			if(cmdLong.command == MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES){ // lets reply with Auto Pilot Version from memory :-)
				// First Send COMMNAD_ACK:
				this->sendAck(MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES, MAV_RESULT_ACCEPTED);
				printf("Sending ACK (MAV_RESULT_ACCEPTED)\n");

				// Next reply with Autopilot version:
				mavlink_message_t reply;				
				mavlink_msg_autopilot_version_encode(this->system_id,this->component_id, &reply, &this->droneParameters->autoPilotVersion); // MSG #148
				this->sendMSG(reply);
				printf("Reply to Client: command=%u is asking for Autopilot version (%u,%u,%u), will also send all %d status messages", cmdLong.command, this->droneParameters->autoPilotVersion.flight_sw_version, 
														  this->droneParameters->autoPilotVersion.middleware_sw_version, this->droneParameters->autoPilotVersion.os_sw_version,
														  this->droneParameters->statusmsgCount);
				
				// send all status text:
				for(int msgCount=0;msgCount<this->droneParameters->statusmsgCount; msgCount++){					
					mavlink_message_t reply;
					mavlink_msg_statustext_encode(this->system_id,this->component_id, &reply, &this->droneParameters->statusmsg[msgCount]);
					this->sendMSG(reply);
				}
			}

			if(cmdLong.command == MAV_CMD_REQUEST_MESSAGE){ // lets reply with message:
				
				if(cmdLong.param1 == MAVLINK_MSG_ID_AUTOPILOT_VERSION){ // lets reply with Auto Pilot Version from memory :-)
					// First Send COMMNAD_ACK:
					this->sendAck(MAV_CMD_REQUEST_MESSAGE, MAV_RESULT_ACCEPTED);
					printf("Sending ACK (MAV_RESULT_ACCEPTED)\n");

					// Next reply with Autopilot version:
					mavlink_message_t reply;				
					mavlink_msg_autopilot_version_encode(this->system_id,this->component_id, &reply, &this->droneParameters->autoPilotVersion);
					this->sendMSG(reply);
					printf("\n");
					printf("command=%u is asking for Autopilot version (%u,%u,%u), will also send all %d status messages", cmdLong.command, this->droneParameters->autoPilotVersion.flight_sw_version, 
															this->droneParameters->autoPilotVersion.middleware_sw_version, this->droneParameters->autoPilotVersion.os_sw_version,
															this->droneParameters->statusmsgCount);
					
					// send all status text:
					for(int msgCount=0;msgCount<this->droneParameters->statusmsgCount; msgCount++){					
						mavlink_message_t reply;
						mavlink_msg_statustext_encode(this->system_id,this->component_id, &reply, &this->droneParameters->statusmsg[msgCount]);
						this->sendMSG(reply);
					}
				}

				if(cmdLong.param1 == 395){ // #395 - COMPONENT_INFORMATION - not supported stil WIP ion Mavlink protocol:
					// Send COMMNAD_ACK - not supported:
					this->sendAck(395, MAV_RESULT_UNSUPPORTED);
					printf("Sending ACK (MAV_RESULT_UNSUPPORTED)");
				}
			}

			if(cmdLong.command == MAV_CMD_SET_MESSAGE_INTERVAL){ // lets reply with Mavlink protocol 1
				// Send COMMNAD_ACK - not supported:
				this->sendAck(MAV_CMD_SET_MESSAGE_INTERVAL, MAV_RESULT_ACCEPTED);
				printf("Sending ACK (MAV_RESULT_ACCEPTED)");
			}

			if(cmdLong.command == MAV_CMD_REQUEST_PROTOCOL_VERSION){ // lets reply with Mavlink protocol 1
				// Send COMMNAD_ACK - not supported:
				this->sendAck(MAV_CMD_REQUEST_PROTOCOL_VERSION, MAV_RESULT_UNSUPPORTED);
				printf("Sending ACK (MAV_RESULT_UNSUPPORTED)");
			}
			printf("\n");

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
			printf(" Nothing done with this package.\n");
		break;
	}	 
 }
