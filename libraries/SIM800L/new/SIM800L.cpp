/*
	sim800l.cpp

	Copyright (c) 2019 Lagoni
	Not for commercial use
 */ 
#include "SIM800l.h"
#include "Arduino.h" // Needed for Serial.print


SIM800L::SIM800L()
{
}

void SIM800L::init(HardwareSerial *ptr1, unsigned long baudrate, int resetPin, String simPin, String apn, String ip, int port, String protocol, uint16_t frameTimeout, uint16_t rssiUpdateRate)
{
	this->SIM800Serial = ptr1;
	this->_baudrate = baudrate;
	this->_resetPin = resetPin;
	this->_simPin = simPin;
	this->_apn = apn;
	this->_remoteIp = ip;
	this->_remotePort = port;
	this->_protocol = protocol;
	this->_frameTimeout = frameTimeout;
	this->_rssiUpdateRate = rssiUpdateRate;

	pinMode(this->_resetPin, OUTPUT); // SIM800L reset pin
			
	// Setup and configure GPS
	this->SIM800Serial->begin(baudrate); // 9600 is default baud after power cycle.. which we don't do here but assumes had been done.		
	this->atStatus = {};
	this->linkState = {};
	this->rxState = {};
	
	// Clear rx/tx buffer:
	this->rxFIFO.clear();
	this->txFIFO.clear();
	
	this->bytesToSend=0; // :-( hate this
	
 }

uint8_t SIM800L::read(void){
	uint8_t data = 0 ;
	if(!(this->rxFIFO.isEmpty())){
		this->rxFIFO.push(data);
	}
	return data;
}

void SIM800L::write(uint8_t data){
	if(!(this->txFIFO.isFull())){
		this->txFIFO.push(data);
	}
}

uint16_t SIM800L::available(void){
	return (uint16_t)this->txFIFO.size();
}

bool SIM800L::isConnected(void){
	return this->atStatus.socketReady;
}

void SIM800L::debugPrint(String text, bool raw){
	if(raw)
		Serial.print(text);
	else
		Serial.print("\n" + String(millis()) + ":SIM800L:" + text);
		
}


String SIM800L::atStateToString(ATReadState state){
	switch(state)
	{
		case AT_MODE:
			return "AT_MODE";
		break;

		case DATA_MODE:
			return "DATA_MODE";
		break;
				
		default:
			return "default";
		break;
	}
}

String SIM800L::linkStateToString(GPRSLinkState_t state){
	switch(state)
	{
		case RESET:
			return "RESET";
		break;

		case STARTUP:
			return "STARTUP";
		break;

		case IDLE:
			return "IDLE";
		break;

		case SENDING_CONNAND:
			return "SENDING_CONNAND";
		break;

		case SENDING_DATA:
			return "SENDING_DATA";
		break;

		case TIMEOUT:
			return "TIMEOUT";
		break;

		case SERVICE:
			return "SERVICE";
		break;
				
		default:
			return "default";
		break;
	}
}

void SIM800L::service(void){
	// timout check.
	this->linkState.nowTime = millis();
	
	this->debugPrint(".",true);
	
	// AT statemachine
	uint16_t bytesToRead = (uint16_t)this->SIM800Serial->available();
	do{
		uint8_t rbyte = 0;
		if(bytesToRead>0){
			rbyte = SIM800Serial->read();
			bytesToRead--;
		
			if(this->parserInput(rbyte)){
				//New information to GPRS link state machine.
				/*
				this->debugPrint("Parsing complete:",false);
				for(int a=0;a<this->rxState.length;a++){
					if(this->rxState.data[a] < 16){
					//Serial.print("0");
						this->debugPrint("0", true);
					}
					this->debugPrint(String(this->rxState.data[a],HEX),true);
					this->debugPrint(" ",true);	
				}	
				*/
				this->debugPrint("LinkState: " + linkStateToString(this->linkState.state) + " rxState: " + atStateToString(this->rxState.state) + " Command Completed=" + String(this->atStatus.commandCompleted) +" Network Attached=" + String(this->atStatus.attached) + " IP=" + this->atStatus.ip +
								 " Socket Ready=" + String(this->atStatus.socketReady) + " ReadyToSend=" + String(this->atStatus.readyToSend) + " SendCompleted=" + String(this->atStatus.sendCompleted) +
								 " SendErrorCount=" + String(this->atStatus.sendErrorCount) + " RSSI=" + String(this->atStatus.rssi) + " Shutdown=" + String(this->atStatus.cipshut) + " Status:" + String(this->atStatus.underVoltageWarnings) +
								 " " + String(this->atStatus.overVoltageWarnings) + ": Init Pramameters:" + String(this->atStatus.ready) + String(this->atStatus.cfun) + String(this->atStatus.cpin) +
								 String(this->atStatus.call) + String(this->atStatus.sms) + String(this->atStatus.cipmux) + String(this->atStatus.cstt) + String(this->atStatus.cifsr) + 
								 String(this->atStatus.ate)+ String(this->atStatus.cipstart) + ":",false);
				
				switch(this->rxState.state)
				{
					case AT_MODE:
					{
						String command = String((char *)this->rxState.data);
						updateATstatus(command);					
					}
					break;
					
					case DATA_MODE: 	
					{
						if(this->rxState.dataModeIndex == 0){ // REceive complete
							this->rxState.state = AT_MODE; 
						}					
					}
					break;
					
					default: 	
					break;
				}
			}
		}
	}while(bytesToRead>0);


	// Linkstate Machine
	switch (this->linkState.state)
	{
		case RESET:	
			this->debugPrint("Enter RESET State!",false);
			// Set RST pin to LOW for 1 sec nad then wmpty buffer and set then to high after 2 sec. This is the ONLY part which bussy waits (uses delay)
			digitalWrite(this->_resetPin, LOW);
			this->resetAllStateMachines();
			delay(2000);
			while (this->SIM800Serial->available()){
				this->SIM800Serial->read(); // empty rx buffer
			}
			digitalWrite(this->_resetPin, HIGH);
			delay(2000);
			this->goToLinkState(STARTUP);
			this->setNextTimeout(20000);  // 60 sec. to starte
			this->atStatus.cipshut = true; // don't do shutdown before asked by CIICR.
		break;

		case STARTUP:	
			if((this->atStatus.ready == true) && (this->atStatus.sms == true) && (this->atStatus.call == true) && (this->atStatus.cfun == true) && (this->atStatus.cpin == true)){ // all of these should come automaticly when we release the reset pin. we will just wait (max 30 sec)
				if(this->atStatus.cipmux == false){
					this->SIM800Serial->print("AT+CIPMUX=1\n"); // set multi connection,
					this->goToLinkState(SENDING_CONNAND);
				}else if(this->atStatus.cstt == false){
					this->SIM800Serial->print("AT+CSTT=\"internet\"\n"); // APNset
					//this->SIM800Serial->print("AT+CSTT=\"" + this->_apn + "\"\n"); // set APN from opperater (Telenor is "internet")
					this->goToLinkState(SENDING_CONNAND);				
				}else if(this->atStatus.cipshut == false){ 
					this->SIM800Serial->print("AT+CIPSHUT\n"); // Shurdown needed before CIICR. (noramlly CIICR will reply with "+PDP: DEACT", which will trigger us to set cipshut=true.
					this->goToLinkState(SENDING_CONNAND);				
				}else if(this->atStatus.attached == false){
					this->SIM800Serial->print("AT+CIICR\n"); // Bring Up Wireless Connection with GPRS 
					this->goToLinkState(SENDING_CONNAND);				
				}else if(this->atStatus.cifsr == false){
					this->SIM800Serial->print("AT+CIFSR\n"); // Get Local IP Address
					this->goToLinkState(SENDING_CONNAND);				
				}else if(this->atStatus.cipstart == false){
					this->SIM800Serial->print("AT+CIPSTART=0,\"" + this->_protocol + "\",\"" + this->_remoteIp +"\",\"" + this->_remotePort + "\"\n"); 
					this->goToLinkState(SENDING_CONNAND);				
				}else if(this->atStatus.ate == false){
					this->SIM800Serial->print("ATE0\n"); // set no echo back.
					this->goToLinkState(SENDING_CONNAND);				
				}else{
					this->goToLinkState(IDLE);
					this->setNextTimeToSend();
					this->setNextRSSITime();
					this->setNextTimeout(2000);
				}								
			}				
		break;

		case IDLE:	
			if( (this->rxFIFO.size() >= UDP_FRAME_SIZE) || (timeToSend() == true) ){ // check if rx fifo is larger than frameing size, if so then send
				if(this->rxFIFO.size() > UDP_FRAME_SIZE){
					this->bytesToSend=UDP_FRAME_SIZE; // max size
				}else{
					this->bytesToSend = this->rxFIFO.size();
				}
				this->SIM800Serial->print("AT+CIPSEND=0," + String(this->bytesToSend) + "\n");
				this->goToLinkState(SENDING_DATA);
				this->setNextTimeout(2000);
			}else if(timeToGetRSSI()){
				this->SIM800Serial->print("AT+CSQ\n");
				this->goToLinkState(SENDING_CONNAND);
				this->setNextRSSITime();
				this->setNextTimeout(1000);  
			}else{
				// NOP
			}
		break;
		
		case SENDING_CONNAND:	
			if(this->atStatus.commandCompleted == true){				
				this->linkState.state = this->linkState.lastState; // go back to where we came from.
			}
		break;
				
		case SENDING_DATA:	
			if((this->atStatus.readyToSend == true) && (this->atStatus.sendCompleted == false)){ // now send the data:
				this->sendData();
			}else if((this->atStatus.readyToSend == true) && (this->atStatus.sendCompleted == true)){ // we are done sending
				this->goToLinkState(IDLE);
				this->setNextTimeToSend();
			}
		break;
		
		case TIMEOUT:		
			this->debugPrint("Enter TIMEOUT State!",false);
			if(this->linkState.lastState == STARTUP){ // something is wrong, we got timeout during STARTUP, lets reboot the module.
				this->goToLinkState(RESET);
			}else if(this->linkState.lastState == SERVICE){
				this->goToLinkState(STARTUP);
				this->setNextTimeout(5000);		  // we give them 5 sec. or reboot.
			}else{			
				this->goToLinkState(SERVICE);  // Go to SERVICE, to update alle connection status.
				// lets set socketConnetion, CG to zero and ask moudle to update them.
				this->atStatus.socketReady=false; // CIPSTATUS will update this and set it accordinly
				this->atStatus.attached = false;  // CGATT? will update this.
				this->setNextTimeout(5000);		  // we give them 5 sec. or reboot.
			}
		break;
		
		case SERVICE:
			this->debugPrint("Enter SERVICE State!",false);
			if(this->atStatus.socketReady == false){
				this->SIM800Serial->print("AT+CIPSTATUS\n"); //  this will update atStatus.socketConnected and atStatus.cipstart
				this->goToLinkState(SENDING_CONNAND);			
			}else if(this->atStatus.attached == false){
				this->SIM800Serial->print("AT+CGATT?\n");   //  this will update atStatus.attached.
				this->goToLinkState(SENDING_CONNAND);							
			}else{
				this->goToLinkState(STARTUP);	// if we have lost connection then it will be reset
			}
		break;
		
		default: 	
		break;
	}
	
	// Timeout Handled last.
	if(this->linkState.nowTime > this->linkState.nextTimeout){ // Set timout
		this->linkState.lastState = this->linkState.state;
		this->linkState.state = TIMEOUT;
		this->debugPrint("Timeout! now=" + String(this->linkState.nowTime) + " threshold=" + String(this->linkState.nextTimeout),false);
	}
	
	//debugP
	
	if(millis()>printtime){
		printtime=millis()+3000;
		this->debugPrint("LinkState: " + linkStateToString(this->linkState.state) + "rxState: " + atStateToString(this->rxState.state) + " Command Completed=" + String(this->atStatus.commandCompleted) +" Network Attached=" + String(this->atStatus.attached) + " IP=" + this->atStatus.ip +
						 " Socket Ready=" + String(this->atStatus.socketReady) + " ReadyToSend=" + String(this->atStatus.readyToSend) + " SendCompleted=" + String(this->atStatus.sendCompleted) +
						 " SendErrorCount=" + String(this->atStatus.sendErrorCount) + " RSSI=" + String(this->atStatus.rssi) +" Status:" + String(this->atStatus.underVoltageWarnings) +
						 " " + String(this->atStatus.overVoltageWarnings) + ": Init Pramameters:" + String(this->atStatus.ready) + String(this->atStatus.cfun) + String(this->atStatus.cpin) +
						 String(this->atStatus.call) + String(this->atStatus.sms) + String(this->atStatus.cipmux) + String(this->atStatus.cstt) + String(this->atStatus.cifsr) + 
						 String(this->atStatus.ate)+ String(this->atStatus.cipstart) + ":",false);	
	}
		
}

void SIM800L::sendData(void){
	uint8_t byte;
	for(int count=0 ; count < this->bytesToSend ; count++){
		this->rxFIFO.pop(byte);
		this->SIM800Serial->write(byte);
	}
	this->bytesToSend=0;
}

void SIM800L::setNextTimeToSend(void){
	// Next time to send
	this->linkState.nextTimeToSend = this->linkState.nowTime + this->_frameTimeout; 
}

bool SIM800L::timeToSend(void){
	if(this->linkState.nowTime > this->linkState.nextTimeToSend){
		return true;
	}
	return false;
}

void SIM800L::setNextRSSITime(void){
	// Next RSSI
	this->linkState.nextRSSITime = this->linkState.nowTime + this->_rssiUpdateRate; 
}

bool SIM800L::timeToGetRSSI(void){
	if(this->linkState.nowTime > this->linkState.nextRSSITime){
		return true;
	}
	return false;
}

void SIM800L::setNextTimeout(uint32_t nextTime){
	this->linkState.nextTimeout = this->linkState.nowTime + nextTime;
}

void SIM800L::resetAllStateMachines(void){
	//Write to log that we are reseing.
	this->atStatus = {};
	this->linkState = {};
	this->rxState = {};	
}

void SIM800L::goToLinkState(GPRSLinkState_t state){
	if(state == SENDING_CONNAND){
		// we are sending a command, lets clear the flag.
		this->atStatus.commandCompleted = false;
	}
	
	if(state == SENDING_DATA){
		// we are sending data, lets clear the flags.
		this->atStatus.readyToSend = false;
		this->atStatus.sendCompleted = false;
	}
	
	this->linkState.lastState = this->linkState.state; // remember where we came from.
	this->linkState.state=state; // set new state.
}


	/*
	First output from SIM800L after reset:
	0D 0A 52 44 59 0D 0A  							<CR><LF>RDY<CR><LF>
	0D 0A 2B 43 46 55 4E 3A 20 31 0D 0A				<CR><LF>+CFUN: 1<CR><LF>
	0D 0A 2B 43 50 49 4E 3A 20 52 45 41 44 59 0D 0A <CR><LF>+CPIN: READY<CR><LF>
	0D 0A 43 61 6C 6C 20 52 65 61 64 79 0D 0A 		<CR><LF>Call Ready<CR><LF> 
	0D 0A 53 4D 53 20 52 65 61 64 79 0D 0A 			<CR><LF>SMS Ready<CR><LF>

	Reply to "AT\n"
	41 54 0D 0A 4F 4B 0D 0A							AT<CR><LF>OK<CR><LF>
	
	Reply to "AT+CGATT?":   	                        
	41 54 2B 43 47 41 54 54 3F 0D 0A 2B 43 47 41 54 54 3A 20 30 0D 0A 0D 0A 4F 4B 0D 0A  	AT+CGATT?<CR><LF>+CGATT: 0<CR><LF><CR><LF>OK<CR><LF>

	Reply to "AT+CSQ":  
	41 54 2B 43 53 51 0D 0A 2B 43 53 51 3A 20 32 39 2C 30 0D 0A 0D 0A 4F 4B 0D 0A			AT+CSQ<CR><LF>+CSQ: 29,0<CR><LF><CR><LF>OK<CR><LF>

	Reply to "AT+CIPMUX=1":
	41 54 2B 43 49 50 4D 55 58 3D 31 0D 0A 4F 4B 0D 0A										AT+CIPMUX=1<CR><LR>OK<CR><LF>

	Reply to "AT+CSTT=\"internet\"":
	41 54 2B 43 53 54 54 3D 22 69 6E 74 65 72 6E 65 74 22 0D 0A 4F 4B 0D 0A					AT+CSTT="internet"<CR><LF>OK<CR><LF>
	
	Reply to "AT+CIICR":
	41 54 2B 43 49 49 43 52 0D 0A 4F 4B 0D 0A												AT+CIICR<CR><LF>OK<CR><LF>
	or
	41 54 2B 43 49 49 43 52 0D 0A 2B 50 44 50 3A 20 44 45 41 43 54 0D 0A 0D 0A 45 52 52 4F 52 0D 0A 	AT+CIICR<CR><LF>+PDP: DEACT<CR><LF><CR><LF>ERROR<CR><LF>

	Reply to "AT+CIFSR":							
	41 54 2B 43 49 46 53 52 0D 0A 38 33 2E 37 33 2E 31 31 31 2E 37 33 0D 0A					AT+CIFSR<CR><LF>83.73.111.73<CR><LF>
	
	Reply to "AT+CIPSHUT":							
	guess:																					AT+CIPSHUT<CR><LF>SHUT OK<CR><LF>
	
	Reply to "AT+CIPSTART=0,\"UDP\",\"81.161.156.81\",\"14450\"":
	41 54 2B 43 49 50 53 54 41 52 54 3D 30 2C 22 55 44 50 22 2C 22 38 31 2E 31 36 31 2E 31 35 36 2E 38 31 22 2C 22 31 34 34 35 30 22 0D 0A 4F 4B 0D 0A 0D 0A 30 2C 20 43 4F 4E 4E 45 43 54 20 4F 4B 0D 0A 										 
	AT+CIPSTART=0,"UDP","81.161.156.81","14450"<CR><LF>OK<CR><LF><CR><LF>0, CONNECT OK<CR><LF>
											 
					
	Reply to "AT+CIPSEND=0"
	0d 0a 3e 20 "<CR><LF>> "
	After send is done:
	0d 0a 30 2c 20 53 45 4e 44 20 4f 4b 0d 0a 												<CR><LF>0, SEND OK<CR><LF>							
	or if failed:
	0d 0a 30 2c 20 53 45 4e 44 20 46 41 49 4c 0d 0a 										<CR><LF>0, SEND FAIL<CR><LF>
	
	AT command reply from "AT+CGATT?" : "41 54 2B 43 47 41 54 54 3F 0D 0A 2B 43 47 41 54 54 3A 20 30 0D 0A"  = "AT+CGATT?<CR><LF>+CGATT: 0<CR><LF>"
	AT command reply from success UDP transmission (socket 0) : "20 0D 0A 30 2C 20 53 45 4E 44 20 4F 4B 0D 0A"  = " <CR><LF>0, SEND OK<CR><LF>"
	
	
	AT command reply from "AT+CIPSTATUS\n" :  53 54 41 54 45 3a 20 49 50 20 50 52 4f 43 45 53 53 49 4e 47 0d 0a 0d 0a
										      43 3a 20 30 2c 30 2c 22 55 44 50 22 2c 22 38 31 2e 31 36 31 2e 31 35 36 2e 38 31 22 2c 22 31 34 34 35 30 22 2c 22 43 4f 4e 4e 45 43 54 45 44 22 0d 0a
										      43 3a 20 31 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 32 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 33 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 34 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										      43 3a 20 35 2c 2c 22 22 2c 22 22 2c 22 22 2c 22 49 4e 49 54 49 41 4c 22 0d 0a
										 
										 In Chars:
										 
										      STATE: IP PROCESSING<CR><LF>
											  <CR><LF>
										      C: 0,0,"UDP","81.161.156.81","14450","CONNECTED"<CR><LF>
										      C: 1,,"","","","INITIAL"<CR><LF>
										      C: 2,,"","","","INITIAL"<CR><LF>
										      C: 3,,"","","","INITIAL"<CR><LF>
										      C: 4,,"","","","INITIAL"<CR><LF>
										      C: 5,,"","","","INITIAL"<CR><LF>
											  
											  
	AT Command reply from "AT+CSQ\n"		  43 53 51 3a 20 33 31 2c 30 0d 0a 0d 0a 4f 4b 0d 0a 0d 0a 
										In Chars:
											  +CSQ: 31,0<CR><LF>
											  <CR><LF>OK<CR><LF>
											  <CR><LF>
											  
	AT Command reply from "AT\n"			  0d 0a 4f 4b 0d 0a 
										In Chars:
											  <CR><LF>OK<CR><LF>	

	When data is revieced:+RECEIVE,<N>,<LEGNTH>:<CR><LF>
	*/
	
	

void SIM800L::goToCommand(ATCommands_t command){
	if((this->atStatus.lastCommand != READY) && (command == READY)){
		// we are going back to READY state, thus indicate command is done!
		this->atStatus.commandCompleted = true;
	}	
	this->atStatus.lastCommand = command;
	this->atStatus.commandIndex=0;
}

	
bool SIM800L::readAck(String command){
	if(command == "OK" || command == "ok")
		return true;
	else
		return false;
}
	
 void SIM800L::updateATstatus(String command){
	
	this->debugPrint("Parsing input String:" + command + ":", false);
	
	switch (this->atStatus.lastCommand)
	{
		case READY: 
			if(command == "> "){						   	      // ready to send data
				this->atStatus.readyToSend = true;
				this->goToCommand(CIPSEND);	
			}else if(command == "AT+CSQ"){      		   		  // AT+CSQ<CR><LF>+CSQ: 29,0<CR><LF><CR><LF>OK<CR><LF>
				this->goToCommand(CSQ);
			}else if(command.substring(0,6) == "STATE:"){		  // CHIPSTATUS reply, see above
				this->goToCommand(CIPSTATUS);
			}else if(command.substring(0,8) == "+RECEIVE"){		  // +RECEIVE,<N>,<LEGNTH>:<CR><LF>											  
				this->rxState.state = DATA_MODE;
				this->rxState.dataModeIndex = this->getReceiveLength(command);	
				this->goToCommand(RECEIVE);
			}else if(command == "AT+CIPSHUT"){					  //AT+CIPSHUT<CR><LF>SHUT OK<CR><LF> (I think)
				this->goToCommand(CIPSHUT);
			}else if(command == "RDY"){
				this->atStatus.ready = true;
			}else if(command == "Call Ready"){
				this->atStatus.call = true;
			}else if(command == "SMS Ready"){
				this->atStatus.sms = true;
			}else if(command == "+CFUN: 1"){
				this->atStatus.cfun = true;		
			}else if(command == "+CPIN: READY"){
				this->atStatus.cpin = true;						  // perhaps this one ony apprears here if sim has no pin?
			}else if(command == "AT"){
				this->goToCommand(AT);
			}else if(command == "AT+CGATT"){
				this->goToCommand(CGATT);
			}else if(command == "AT+CIPMUX=1"){ 		   		  // AT+CIPMUX=1<CR><LR>OK<CR><LF>
				this->goToCommand(CIPMUX);
			}else if(command.substring(0,7) == "AT+CSTT"){ 		  // AT+CSTT="internet"<CR><LF>OK<CR><LF>
				this->goToCommand(CSTT);
			}else if(command == "AT+CIICR"){  			   	 	  // AT+CIICR<CR><LF>OK<CR><LF>					
																  // AT+CIICR<CR><LF>+PDP: DEACT<CR><LF><CR><LF>ERROR<CR><LF>
				this->goToCommand(CIICR);
			}else if(command == "AT+CIFSR"){		  	          // AT+CIFSR<CR><LF>83.73.111.73<CR><LF>
				this->goToCommand(CIFSR);
			}else if(command.substring(0,13) == "AT+CIPSTART=0"){ // AT+CIPSTART=0,"UDP","81.161.156.81","14450"<CR><LF>OK<CR><LF><CR><LF>0, CONNECT OK<CR><LF>
				this->goToCommand(CIPSTART);
			}else if(command == "UNDER-VOLTAGE WARNNING"){
				if(this->atStatus.underVoltageWarnings<255)
					this->atStatus.underVoltageWarnings++;				
			}else if(command == "OVER-VOLTAGE WARNNING"){
				if(this->atStatus.overVoltageWarnings<255)
					this->atStatus.overVoltageWarnings++;				
			}else if(command == "ATE"){
				this->goToCommand(ATE);
			}else{
				// unhandled command input... maybe log it?
			}			
		break;

		case AT: 	
			this->atStatus.at=readAck(command);
			this->goToCommand(READY);
		break;
		
		case ATE: 	
			this->atStatus.ate=readAck(command);
			this->goToCommand(READY);
		break;
		
		case CGATT: 	// index = 1 -> "+CGATT: 0"  index = 2 -> "ok"
			this->atStatus.commandIndex++;
			if(this->atStatus.commandIndex == 1){
				if(command.charAt(8) == 1)
					this->atStatus.attached = true;
				else
					this->atStatus.attached = false;
			}else if(this->atStatus.commandIndex == 2){
				this->readAck(command);
				this->goToCommand(READY);
			}			
		break;

		case CSQ: // AT+CSQ<CR><LF>+CSQ: 29,0<CR><LF><CR><LF>OK<CR><LF>
			this->atStatus.commandIndex++;
			if(this->atStatus.commandIndex == 1){
				this->updateRSSI(command);
			}else if(this->atStatus.commandIndex == 2){
				this->readAck(command);
				this->goToCommand(READY);
			}				
		break;
		
		case CIPMUX: // AT+CIPMUX=1<CR><LR>OK<CR><LF>
			this->atStatus.commandIndex++;
			if(this->atStatus.commandIndex == 1){
				this->atStatus.cipmux = this->readAck(command);
				this->goToCommand(READY);
			}				
		break;

		case CSTT: 	// AT+CSTT="internet"<CR><LF>OK<CR><LF>
			this->atStatus.commandIndex++;
			if(this->atStatus.commandIndex == 1){
				this->atStatus.cstt = this->readAck(command);
				this->goToCommand(READY);
			}			
		break;
		
		case CIICR: // AT+CIICR<CR><LF>OK<CR><LF>
					// AT+CIICR<CR><LF>ERROR<CR><LF>
					// AT+CIICR<CR><LF>+PDP: DEACT<CR><LF><CR><LF>ERROR<CR><LF>
			this->atStatus.commandIndex++;
			if(this->atStatus.commandIndex == 1){
				if(this->readAck(command) == true){
					this->atStatus.attached = true;
					this->goToCommand(READY);
				}else{
					if(command == "+PDP: DEACT"){
						this->atStatus.attached = false;
						this->atStatus.cifsr = false; // We have no IP
						this->atStatus.cipshut = false; // shutdown needed.
						// read this command again.
					}else{
						this->atStatus.cipshut = false; // shutdown needed.
						this->goToCommand(READY); // first read with ERROR
					}
				}
				if(this->atStatus.commandIndex == 2){
					this->goToCommand(READY); // weird!					
				}
			}else if(this->atStatus.commandIndex == 2){
				this->atStatus.attached = this->readAck(command);
				this->goToCommand(READY);
			}					
		break;

		case CIFSR: // AT+CIFSR<CR><LF>83.73.111.73<CR><LF> 
			this->atStatus.ip = command; // no ack, thus ip.
			this->atStatus.cifsr=true;
			this->goToCommand(READY);
		break;
		
		case CIPSHUT: // guess: AT+CIPSHUT<CR><LF>SHUT OK<CR><LF>
			if(command == "SHUT OK"){
				this->atStatus.cipshut = true;				
			}
			this->goToCommand(READY);	
		break;

		case CIPSTART: // AT+CIPSTART=0,"UDP","81.161.156.81","14450"<CR><LF>OK<CR><LF><CR><LF>0, CONNECT OK<CR><LF>
			this->atStatus.commandIndex++;
			if(this->atStatus.commandIndex == 1){
				if(!(this->readAck(command) == true)){
					this->goToCommand(READY);			
				}	
			}else if(this->atStatus.commandIndex == 2){
				if(command == "0, CONNECT OK"){
					this->atStatus.cipstart = true;
					
				}else{
					this->atStatus.cipstart = false;
				}
				this->goToCommand(READY);
			}			
		break;
		
		case CIPSEND: 	
			if(command == "0, SEND OK"){
				this->atStatus.sendErrorCount=0;
			}else if(command == "0, SEND FAIL"){
				if(this->atStatus.sendErrorCount<255){
					this->atStatus.sendErrorCount++;	
				}					
			}else{
				// error?
			}
			this->atStatus.sendCompleted = true;
		break;

		case CIPSTATUS: //	C: 0,0,"UDP","81.161.156.81","14450","CONNECTED"<CR><LF>
						//  C: 1,,"","","","INITIAL"<CR><LF>
						//  C: 2,,"","","","INITIAL"<CR><LF>
						//  C: 3,,"","","","INITIAL"<CR><LF>
						//  C: 4,,"","","","INITIAL"<CR><LF>
						//  C: 5,,"","","","INITIAL"<CR><LF>
			this->atStatus.commandIndex++;
			if(this->atStatus.commandIndex == 1){
				this->updateConnectionStatus(command);
			}else if(this->atStatus.commandIndex == 6){
				this->goToCommand(READY);		
			}else{
				
			}
		break;	
		
		case RECEIVE:
			// wait for recieve to complete:		
		break;
		
		default:
		break;				
	}	
}

uint16_t SIM800L::getReceiveLength(String command){
	
	//When data is revieced:+RECEIVE,<N>,<LEGNTH>:<CR><LF>
	if(command.length() == 13){ // length is 1 char
		this->rxState.dataModeIndex = (((uint8_t)command.charAt(11))-48);
	}else if(command.length() == 14){ // length is 2 char
		this->rxState.dataModeIndex  = (((uint8_t)command.charAt(11))-48)*10 + (((uint8_t)command.charAt(12))-48);
	}else if(command.length() == 15){ // length is 3 char
		this->rxState.dataModeIndex  = (((uint8_t)command.charAt(11))-48)*100 + ((((uint8_t)command.charAt(12))-48)*10) + (((uint8_t)command.charAt(13))-48);						
	}else if(command.length() == 16){ // length is 4 char (maximum i hope) think it is 1400 or so
		this->rxState.dataModeIndex = (((uint8_t)command.charAt(11))-48)*1000 + ((((uint8_t)command.charAt(12))-48)*100) + ((((uint8_t)command.charAt(13))-48)*10) + (((uint8_t)command.charAt(14))-48);							
	}else{
		this->rxState.dataModeIndex  = 0; // Error!
	}
}

bool SIM800L::parserInput(uint8_t inputbyte){ // return true when data is ready but dischards empty strings

/*
	if(inputbyte > 16)
		Serial.print(" " + String(inputbyte,HEX));
	else
		Serial.print(" 0" + String(inputbyte,HEX));
	*/
	switch(this->rxState.state)
	{
		case AT_MODE: 
			if(inputbyte == 0x0A)
			{
				if(this->rxState.data[this->rxState.index-1] == 0x0D){ // check if last two bytes were <CR><LF> 0x0D 0x0A
					// string must be large than 1 (2 bytes) else string is empty:
					if(this->rxState.index > 1){
						this->rxState.data[this->rxState.index-1] = 0;
						this->rxState.length = this->rxState.index-1; // remove the last  two byte 0x0D
						this->rxState.index=0;
						return true; // we have found a <CR><LF> inform user data is ready.
					}else{ // empty string, reset
						this->rxState.index=0;
					}
				}else{ // Error restart with informing 
					this->rxState.index=0;
				}			
			}else{			
				this->rxState.data[this->rxState.index] = inputbyte; // save input to buffer
				this->rxState.index++;
				
				if((this->rxState.index == 2) && (this->rxState.data[0] == 0x3E) &&  (this->rxState.data[1] == 0x20)){ // check to see if this is transaction to DATA_MODE (0x0D 0x0A 0x3E 0x20) 
					this->rxState.index=0;
					return true;
				}
				
				if(this->rxState.index >= AT_BUFFER_SIZE){ // overflow, should never happend!
					this->rxState.index=0;
					//Errro!
				}				
			}
		break;
		
		case DATA_MODE:  
			if(!(this->rxFIFO.isFull())){
				this->rxFIFO.push(inputbyte); // save input to buffer
				if(this->rxState.dataModeIndex > 0)
					this->rxState.dataModeIndex--;
				if(this->rxState.dataModeIndex == 0){
					return true;
				}
			}
		break;
		
		default:
		break;
	}
	return false;
}

void SIM800L::updateRSSI(String command){
	if(command.charAt(7) == ','){ // then signal is only one digit.
		this->atStatus.rssi = (uint8_t)(command.charAt(6)-48); // ascii to int.
					
		if(command.length() == 9){ // get bit error one digit (ber)
			this->atStatus.ber = (uint8_t)(command.charAt(8)-48);
		}else if(command.length() == 10){
			this->atStatus.ber = (uint8_t)((command.charAt(8)-48)*10 + (command.charAt(9)-48)); // ascii to int.
		}else{
			// Error not found.
		}
	}else if(command.charAt(8) == ','){ // then signal is two digit.
		this->atStatus.rssi = (uint8_t)((command.charAt(6)-48)*10 + (command.charAt(7)-48)); // ascii to int.
		
		if(command.length() == 10){ // get bit error one digit (ber)
			this->atStatus.ber = (uint8_t)(command.charAt(9)-48);
		}else if(command.length() == 11){
			this->atStatus.ber = (uint8_t)((command.charAt(9)-48)*10 + (command.charAt(10)-48)); // ascii to int.
		}else{
			// Error not found.	
		}
	}else{
		// Error not found.
	}
}


void SIM800L::updateConnectionStatus(String command){
	// extract connection status from string:
	uint16_t replyLength = command.length();
	String conStatus = "";
	uint16_t index = replyLength-1;
	do{
		if(command.charAt(index-1) == '"'){
			conStatus=command.substring(index,replyLength-1);
			replyLength=0; // stop loop
		}
		index--;
	}while(replyLength > 0);
	
	if(conStatus == "CONNECTED"){
		this->atStatus.socketReady = true;
		this->atStatus.cipstart=true;		
	}
	else{
		this->atStatus.socketReady = false;
		this->atStatus.cipstart=false;		
	}
}

uint8_t SIM800L::getRSSI(void){
	// RSSI from SIM800L is:
	// 0 -> -115dBm
	// 1 -> -111dbm
	// 2-30 -> -110 to -54dbm
	// 31 -> -52dBm or greater
	// 99 Unknown
	
	/* This will be translated to RSSI in % for Mavlink:
	RSSI value:31 Gives:254 Which is 100%
	RSSI value:30 Gives:246 Which is 97%
	RSSI value:29 Gives:239 Which is 94%
	RSSI value:28 Gives:231 Which is 91%
	RSSI value:27 Gives:224 Which is 88%
	RSSI value:26 Gives:217 Which is 85%
	RSSI value:25 Gives:209 Which is 82%
	RSSI value:24 Gives:202 Which is 79%
	RSSI value:23 Gives:195 Which is 76%
	RSSI value:22 Gives:187 Which is 73%
	RSSI value:21 Gives:180 Which is 70%
	RSSI value:20 Gives:172 Which is 68%
	RSSI value:19 Gives:165 Which is 65%
	RSSI value:18 Gives:158 Which is 62%
	RSSI value:17 Gives:150 Which is 59%
	RSSI value:16 Gives:143 Which is 56%
	RSSI value:15 Gives:136 Which is 53%
	RSSI value:14 Gives:128 Which is 50%
	RSSI value:13 Gives:121 Which is 47%
	RSSI value:12 Gives:113 Which is 44%
	RSSI value:11 Gives:106 Which is 41%
	RSSI value:10 Gives:99 Which is 39%
	RSSI value:9 Gives:91 Which is 36%
	RSSI value:8 Gives:84 Which is 33%
	RSSI value:7 Gives:77 Which is 30%
	RSSI value:6 Gives:69 Which is 27%
	RSSI value:5 Gives:62 Which is 24%
	RSSI value:4 Gives:54 Which is 21%
	RSSI value:3 Gives:47 Which is 18%
	RSSI value:2 Gives:40 Which is 15%
	RSSI value:1 Gives:32 Which is 12%
	RSSI value:0 Gives:25 Which is 10%
	*/
	float scaledRSSI = 0;
	
	if(this->atStatus.rssi == 99){
		return 255; // invalid/unknown
	}
	/*
	for(uint8_t a=31; a>0 ; a--){
		scaledRSSI = ((((float)(a))/31*90+10)/100)*254; 
		Serial.print("RSSI value:" + String(a) + " Gives:" + String((uint8_t)scaledRSSI) + " Which is " + String((uint8_t)((scaledRSSI*100.0)/254.0)) + "%\n");
	}
	*/
	scaledRSSI = ((((float)(this->atStatus.rssi))/31*90+10)/100)*254;
	
	return (uint8_t)scaledRSSI;
}
