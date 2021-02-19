/*
	waypoints.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "waypoints.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>


using namespace std;
// Constructor
 Waypoints::Waypoints(std::string filePath, std::string fileName, DataMode_t mode){
	this->reset();
	/*
	if(filePath.back() == '/')
	{
		this->_filePath = filePath;
	}else
	{
		this->_filePath = filePath + "/";
	}
		
	this->_fileName = fileName;
	*/
	this->fileMode = mode;
	
	if(this->fileMode == READ_ONLY){ // else skip load and thus main will ask for them.
		printf("Loading all waypoints from file %s", this->_fileName.c_str());
		this->loadWaypoints();
	}
} 
 
 void Waypoints::reset(void){
	for(uint32_t index=0;index<MAX_PARAMETERS; index++){
		waypoints[index].param1=0;
		waypoints[index].param2=0;
		waypoints[index].param3=0;
		waypoints[index].param4=0;
		waypoints[index].x=0;
		waypoints[index].y=0;
		waypoints[index].z=0;		
		waypoints[index].seq=0;
		waypoints[index].command=0;
		waypoints[index].target_system=0;		
		waypoints[index].target_component=0;	
		waypoints[index].frame=0;		
		waypoints[index].current=0;			
		waypoints[index].autocontinue=0;		
	}	
	char buf[100];
	time_t now = time(0);
	// convert now to tm struct for UTC
	tm *gmtm = gmtime(&now);
	sprintf(buf,"%02d-%02d-%04d_%02d-%02d-%02d_Waypoints.txt",gmtm->tm_mday,gmtm->tm_mday,gmtm->tm_year+1900,gmtm->tm_hour,gmtm->tm_min,gmtm->tm_sec); 
	this->_fileName = buf;
 }
 
 void Waypoints::addWaypoint(mavlink_message_t msg){
	 if(msg.msgid == MAVLINK_MSG_ID_MISSION_ITEM){
		mavlink_mission_item_t way;
		mavlink_msg_mission_item_decode(&msg, &way);
		this->addWaypoint(way);
	 }
 }
 
 void Waypoints::addWaypoint(mavlink_mission_item_t msg){
	
	
	// set the local memory.
	if(msg.seq<MAX_PARAMETERS){
		waypoints[msg.seq].param1=msg.param1;
		waypoints[msg.seq].param2=msg.param2;
		waypoints[msg.seq].param3=msg.param3;
		waypoints[msg.seq].param4=msg.param4;
		waypoints[msg.seq].x=msg.x;
		waypoints[msg.seq].y=msg.y;
		waypoints[msg.seq].z=msg.z;		
		waypoints[msg.seq].seq=msg.seq;
		waypoints[msg.seq].command=msg.command;
		waypoints[msg.seq].target_system=msg.target_system;		
		waypoints[msg.seq].target_component=msg.target_component;	
		waypoints[msg.seq].frame=msg.frame;		
		waypoints[msg.seq].current=msg.current;			
		waypoints[msg.seq].autocontinue=msg.autocontinue;	
	/*	
		if(totalParameterCount==0){
			totalParameterCount=msg.param_count;
			printf("Total parameter count in system=%d\n",totalParameterCount);
		}*/

	//	this->saveWaypoints();	
		/*
		if(this->getFirstMissingParameter() == -1){ // done with all parameters, thus save them
			printf("Saving parameters to file %s\n",this->_fileName);
			this->saveParameters();		
		}*/
	}else{
		printf("Waypoint Index Error, requested=%d but is only=%d\n",msg.seq,MAX_PARAMETERS);
	}		
 }

 mavlink_mission_item_t Waypoints::getWaypointByID(uint16_t id){
	 mavlink_mission_item_t msg;
	 if(id < MAX_PARAMETERS){
		 msg = waypoints[id];
	 }else{
		 msg = {};
	 }
	 return msg;
 }
 

 DataMode_t Waypoints::getWaypointMode(void){
	return this->fileMode;
 }

void Waypoints::loadWaypoints(void){
	
	ifstream myfile(_fileName);
	if (myfile.is_open()) {
		string line;
			
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
			
		int waypointIndex=0;
		int waypointCount=0;
		waypointIndex=PARAM_1;
		while (getline(myfile, line)) {
			
			switch(waypointIndex)
			{
				case PARAM_1:
					waypoints[waypointCount].param1 = (float)std::stof(line);
					waypointIndex=PARAM_2;					
				break;

				case PARAM_2:
					waypoints[waypointCount].param2 = (float)std::stof(line);
					waypointIndex=PARAM_3;					
				break;
				
				case PARAM_3:
					waypoints[waypointCount].param3 = (float)std::stof(line);
					waypointIndex=PARAM_4;					
				break;

				case PARAM_4:
					waypoints[waypointCount].param4 = (float)std::stof(line);
					waypointIndex=PARAM_X;					
				break;

				case PARAM_X:
					waypoints[waypointCount].x = (float)std::stof(line);
					waypointIndex=PARAM_Y;					
				break;

				case PARAM_Y:
					waypoints[waypointCount].y = (float)std::stof(line);
					waypointIndex=PARAM_Z;					
				break;
				
				case PARAM_Z:
					waypoints[waypointCount].z = (float)std::stof(line);
					waypointIndex=PARAM_SEQ;					
				break;		

				case PARAM_SEQ:
					waypoints[waypointCount].seq = (uint16_t)std::stof(line);
					waypointIndex=PARAM_COMMAND;					
				break;	

				case PARAM_COMMAND:
					waypoints[waypointCount].command = (uint16_t)std::stof(line);
					waypointIndex=PARAM_TARGET_SYSTEM;					
				break;	

				case PARAM_TARGET_SYSTEM:
					waypoints[waypointCount].target_system = (uint8_t)std::stof(line);
					waypointIndex=PARAM_TARGET_COMPONENT;					
				break;	
				
				case PARAM_TARGET_COMPONENT:
					waypoints[waypointCount].target_component = (uint8_t)std::stof(line);
					waypointIndex=PARAM_FRAME;					
				break;	
				
				case PARAM_FRAME:
					waypoints[waypointCount].frame = (uint8_t)std::stof(line);
					waypointIndex=PARAM_CURRENT;					
				break;	
				
				case PARAM_CURRENT:
					waypoints[waypointCount].current = (uint8_t)std::stof(line);
					waypointIndex=PARAM_AUTOTUNE;					
				break;				

				case PARAM_AUTOTUNE:
					waypoints[waypointCount].autocontinue = (uint8_t)std::stof(line);
					waypointCount++;
					waypointIndex=PARAM_1;					
				break;	
			
				default: 	
				break;
			}	
			printf(":%s:\n", line.c_str());	
		}
		myfile.close();
		this->totalWaypointCount = waypointCount;
		printf("Waypoint Loading complete (%d) complete!\n", this->totalWaypointCount);		
	}else{
		printf("Open waypoint file failed!\n");		
	}
	
	
}

void Waypoints::saveWaypoints(void){
	
	printf("Saving all waypoints to file.\n");		
	FILE * SystemLogFile;
	SystemLogFile = fopen(_fileName.c_str(),"w");
	std::string floatString  = "";
	// loop and save the array:
	for(uint32_t index=0;index<this->totalWaypointCount; index++){
		floatString = to_string(waypoints[index].param1);	
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file

		floatString = to_string(waypoints[index].param2);	
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file

		floatString = to_string(waypoints[index].param3);	
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file

		floatString = to_string(waypoints[index].param4);	
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file

		floatString = to_string(waypoints[index].x);	
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file

		floatString = to_string(waypoints[index].y);	
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file

		floatString = to_string(waypoints[index].z);	
		fprintf(SystemLogFile,"%s\n",floatString.c_str());                   // String to file
		fprintf(SystemLogFile,"%d\n",waypoints[index].seq);         		 // String to file
		fprintf(SystemLogFile,"%d\n",waypoints[index].command);         	 // String to file
		fprintf(SystemLogFile,"%d\n",waypoints[index].target_system);        // String to file
		fprintf(SystemLogFile,"%d\n",waypoints[index].target_component);     // String to file
		fprintf(SystemLogFile,"%d\n",waypoints[index].frame);         		 // String to file
		fprintf(SystemLogFile,"%d\n",waypoints[index].current);         	 // String to file
		fprintf(SystemLogFile,"%d\n",waypoints[index].autocontinue);         // String to file
	}	
			
	fclose (SystemLogFile); // must close after opening
	printf("Save complete!\n");	
}

uint16_t Waypoints::getNumberOfWaypoints(void){
	return this->totalWaypointCount;
}

void Waypoints::setNumberOfWaypoints(uint16_t totalNumberOfWaypoints){
	this->totalWaypointCount = totalNumberOfWaypoints;
}
void Waypoints::saveMission(void){
	this->saveWaypoints();
}
 