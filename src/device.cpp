/*
	device.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "device.h" 



 Device::Device(int ConnectionPort, int ConnectionType, std::string name, Parameters *parameterList, Waypoints *waypointList){// Contructor for when starting a connection.
	this->droneWaypoints = waypointList;
	this->droneParameters = parameterList;
	this->myConnection.initConnection(ConnectionPort, ConnectionType);
	this->_name = name;
//	std::cout << "Hi! I'm a new module with name=" << this->_name << "\n";
 }
 

 Device::Device(int fd, std::string name, Parameters *parameterList, Waypoints *waypointList){// Contructor for when starting a connection.
	this->droneWaypoints = waypointList;
	this->droneParameters = parameterList;
//	this->myConnection.initConnection(ConnectionPort, ConnectionType);
	this->_name = name;
	this->myConnection.startConnection(fd);
//	std::cout << "Hi! I'm a new module with name=" << this->_name << "\n";
 }

 Device::Device(int ConnectionPort, int ConnectionType, std::string name){// Contructor for when starting a connection.
	this->myConnection.initConnection(ConnectionPort, ConnectionType);
	this->_name = name;
//	std::cout << "Hi! I'm a new module with name=" << this->_name << "\n";
 }
  
std::string Device::getName(void){
	return this->_name;
}
 
 
 uint16_t Device::service(void){ // return RX FIFO size (number of messages to be handled by main program)
	 // parse all data through filter and then call handleIncomming Package.
	uint16_t nbytes=0;
	bzero(this->rxBuffer, MAXLINE); 
	nbytes = this->myConnection.readData(this->rxBuffer, sizeof(this->rxBuffer));	
//	std::cout << this->_name  <<  ":I got " << nbytes <<"Bytes of data from my connection RX buffer...:\n";
	uint16_t index=0;
	
	while(nbytes > 0){
	  nbytes--;
	  uint8_t byte=this->rxBuffer[index];
	  index++;
	 
		if (mavlink_parse_char(this->chan, byte, &this->msg, &this->status)){
//			printf("MSG ID#%d -> ",msg.msgid);
			switch(this->myFilter.getPackageFilter(msg.msgid))
			{
			  case FORWARD: // Put in RX buffer for main program to read.
			  {
				//std::cout << "FORWARD!\n";
				if(!this->rxFIFO.isFull()){
					this->rxFIFO.push(this->msg);		
				}else{
					uint16_t fifoSize=this->rxFIFO.size();
					std::cout << this->_name  <<  ":Error, my rxFIFO (" << fifoSize << ") is Full! " << nbytes <<"Bytes of data from my rxFIFO\n";
				}

				// debug
				if(msg.msgid == 76){
					mavlink_command_long_t buf;
					mavlink_msg_command_long_decode(&msg, &buf);			
					printf("param1 %f   Param2 %f   Param3 %f   Param4 %f   Param5 %f   Param6 %f   Param7 %f    command %d\n", buf.param1,buf.param2,buf.param3,buf.param4,buf.param5,buf.param6,buf.param7, buf.command);
				}
			  }
			  break;
			  
			  case HANDLE: // We will do somthing with it "here" (in the derived classes.  
				//std::cout << "HANDLE!\n";
				this->handleIncommingPackage(msg); 
			  break;

			  case INSPECT: // We will handle the package and forward it to clients.
			  	  //std::cout << "HANDLE!\n";
				this->handleIncommingPackage(msg);
				
				if(!this->rxFIFO.isFull()){
					this->rxFIFO.push(this->msg);
				}else{
					uint16_t fifoSize=this->rxFIFO.size();
					std::cout << this->_name  <<  ":Error, my rxFIFO (" << fifoSize << ") is Full! " << nbytes <<"Bytes of data from my rxFIFO\n";
				}			
			  break;
		
			  case DISCARD: // lose the data.  
			  /*
				if(!(msg.msgid == 0 ||  msg.msgid == 66) ){ // don't print out heartbeat dischard and the MAVLINK_MSG_ID_REQUEST_DATA_STREAM (66) which is not used.
					printf("%s DISCARD! - MSG ID#%d\n",this->_name.c_str(),msg.msgid);
				}
				*/	
			  	//std::cout << this->_name << ": DISCARD! - MSG#" << msg.msgid << "\n" ;
			  break;  
			  
			  case UNKONWN_PACKAGE: // print that we got a unknown package.
			  default: 
				std::cout << "UNKONWN_PACKAGE!\n";
				//Serial.print("\n" + String(millis())+":MAIN:Unkown      MSG from SIM800L ID" + String(msgSim.msgid));
			  break;
			}          		  
		}
	}
	return this->rxFIFO.size();
 }
 /*
 void Device::sendBufferData(uint16_t length){
	 uint16_t res = this->myConnection.writeData(this->txBuffer, length);
 }
 */
 void Device::sendMSG(mavlink_message_t data){
	if(this->myConnection.getFD() > 1){
		static const uint8_t mavlink_message_crcs[256] = {  50, 124, 137,   0, 237, 217, 104, 119,   0,   0, //  0-9
															 0,  89,   0,   0,   0,   0,   0,   0,   0,   0, // 10-19
														   214, 159, 220, 168,  24,  23, 170, 144,  67, 115, // 20-29
															39, 246, 185, 104, 237, 244, 222, 212,   9, 254, // 30-39
														   230,  28,  28, 132, 221, 232,  11, 153,  41,  39, // 40-49
															78, 196,   0,   0,  15,   3,   0,   0,   0,   0, // 50-59
															 0, 153, 183,  51,  59, 118, 148,  21,   0, 243, // 60-69
														   124,   0,  20,  38,  20, 158, 152, 143,   0,   0, // 70-79
															 0, 106,  49,  22, 143, 140,   5, 150,   0, 231, // 80-89
														   183,  63,  54,   0,   0,   0,   0,   0,   0,   0, // 90-99
														   175, 102, 158, 208,  56,  93, 138, 108,  32, 185, //100-109
															84,  34, 174, 124, 237,   4,  76, 128,  56, 116, //110-119
														   134, 237, 203, 250,  87, 203, 220,  25, 226,  46, //120-129
															29, 223,  85,   6, 229, 203,   1, 195, 109, 168, //130-139
														   181,  47,  72, 131, 127,   0, 103, 154, 178, 200, //140-149
														   134,   0, 208,   0,   0,   0,   0,   0,   0,   0, //150-159
															 0,   0,   0, 127,   0,  21,   0,   0,   1,   0, //160-169
															 0,   0,   0,   0,   0,   0,   0,   0,  47,   0, //170-179
															 0,   0, 229,   0,   0,   0,   0,   0,   0,   0, //180-189
															 0,   0,   0,  71,   0,   0,   0,   0,   0,   0, //190-199
															 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //200-209
															 0,   0,   0,   0,   0,   0,   0,   0,   0,   0, //210-219
															34,  71,  15,   0,   0,   0,   0,   0,   0,   0, //220-229
														   163, 105,   0,  35,   0,   0,   0,   0,   0,   0, //230-239
														  	 0,  90, 104,  85,  95, 130, 184,   0,   8, 204, //240-249
															49, 170,  44,  83,  46,   0};					 //250-256

	//	mavlink_message_t newdata2 = data;
		mavlink_finalize_message_chan(&data, this->system_id, this->component_id, this->chan, data.len, data.len, mavlink_message_crcs[data.msgid]);
		
		uint16_t totalLength=0;
		bzero(this->txBuffer, MAXLINE); 
		totalLength = mavlink_msg_to_send_buffer(this->txBuffer, &data);		
		uint16_t res = this->myConnection.writeData(this->txBuffer, totalLength);
		
		/*
		// DEBUG printf:
		printf("Sending MSG: ");
		for (int a=0;a<totalLength; a++){
			printf("%02x ", this->txBuffer[a]);
		}
		printf("\n");
		*/
		
//		printf("Sending MSG ID=%d\n",data.msgid);
	}else{
		printf("Client not active\n");
	}
 }
  
 mavlink_message_t Device::getMSG(void){
	mavlink_message_t data;
	if(!this->rxFIFO.isEmpty()){
		this->rxFIFO.pop(data);
		
	}
	return data;		
 }
  
 void Device::sendAllParameters(void){
	 
	for(int id=0; id < this->droneParameters->getNumberOfParameters();id++){
		mavlink_message_t msg;
		mavlink_param_value_t parameter = this->droneParameters->getParameterByID(id);
		mavlink_msg_param_value_encode(this->system_id, this->component_id, &msg, &parameter); // SYS.ID=1, COMP.ID=1 (Reply like we where a Pixhawk :-) )
		this->sendMSG(msg);
	}
 }
  	
 void Device::setFD_SET(fd_set* fdset){
	 this->myConnection.setFD_SET(fdset);
 }
  
 void Device::setFD(int fd){
	this->myConnection.setFD(fd); 
 }
 
 int Device::getFD(void){
	return this->myConnection.getFD(); 
 }
 /*
 void Device::startConnection(int fd, std::string name, Parameters *parameterList){
	 this->_name = name;
	 this->
	 this->myConnection.startConnection(fd);
 }*/

 
void Device::sendAck(uint16_t mav_cmd, uint8_t mav_result){
	mavlink_message_t reply;
	mavlink_command_ack_t ack;
	ack.command=mav_cmd;
	ack.result=mav_result;
	mavlink_msg_command_ack_encode(this->system_id,this->component_id, &reply, &ack);
	this->sendMSG(reply);
}
  
