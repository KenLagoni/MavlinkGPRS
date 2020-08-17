/*
	sim800l.h

	Copyright (c) 2019 Lagoni
	Not for commercial use
 */ 

#ifndef SIM800L_h
#define SIM800L_h

#include "Arduino.h" // Needed for Strings and Serial
#include "RingBuf.h" 

#define DEBUD_MODE	0

#define SOFT_TIMEOUT        		    500  // 500ms
#define SOFT_TIMEOUT_RECOVER 		   3000  // 3000ms (Time to recover from soft reset)
#define HARD_TIMEOUT_COUNT   		     10  // Number of softresets in a row to trigger hard reset.
		

#define SIM800L_DATA_BUFFER_SIZE     	512
#define SEND_WINDOW_TIMEFREQ			100 // every 100ms data must be send, if any.	
#define AT_BUFFER_SIZE					128
#define FIFO_SIZE 						1024
#define UDP_FRAME_SIZE 				    512
				
class SIM800L
{
  public:
    SIM800L();
	SIM800L(HardwareSerial *ptr1, unsigned long baudrate, int resetPin, String simPin, String apn, String ip, int port, String protocol, uint16_t frameTimeout, uint16_t rssiUpdateRate);
	void init(HardwareSerial *ptr1, unsigned long baudrate, int resetPin, String simPin, String apn, String ip, int port, String protocol, uint16_t frameTimeout, uint16_t rssiUpdateRate);
	
	uint16_t service(void); // Service the modules and returns rxFIFO size.
	uint8_t read(void);
	void write(uint8_t data);
//	uint16_t available(void);
	uint8_t getRSSI(void);
	bool isConnected(void);
	String getStatusMSG(void);
	String getConnectionStatus(void);
	
  private:
	HardwareSerial  *SIM800Serial = NULL;       // Pointer to the serial port used
	unsigned long _baudrate;
	int _resetPin;
	String _simPin;
	String _apn;
	String _remoteIp;
	String _remotePort;
	String _protocol;
	uint16_t _frameTimeout;
	uint16_t _rssiUpdateRate;
	    
	// Supported commands:
	enum ATCommands_t{
	  READY=0,
	  AT,
	  ATE,
	  CGATT,
	  CSQ,
	  CIPMUX,
	  CSTT,
	  CIICR,
	  CIFSR,
	  CIPSHUT,
	  CIPSTART,
	  CIPSEND,
	  CIPSTATUS,
	  RECEIVE
	};
	
	// Command struct:
	struct ATstatus_t 
	{
		bool commandCompleted; 		// This is to indicate if we are waiting for command to finish.
		
		// Init status, Set to true when set correectly:
		bool ready; 		 // set to true when RDY has been revieced.
		bool cfun;			 // set to true when CFUN has been revieced.
		bool cpin;		     // set to true when CPIN has been revieced.
		bool call;			 // set to true when Calls Ready has been revieced.
		bool sms;			 // set to true when SMS Ready has been revieced.
		bool cipmux;		 // set to true when CIPMUX has been revieced.
		bool cstt;			 // set to true when CSTT has been revieced.
		bool cifsr;			 // set to true when APN has been revieced.
		bool ate;		     // set to true when ATE has been revieced.
		//bool cipstart;       // set to true when reply from CIPSTART is Connected.
		
		// AT ack.
		bool at;			 // set to true when AT OK revieced.
		
		// soft restart
		bool cipshut;		 // set to true when "SHUT OK" from CIPSHUT has revieced.
				
		bool attached;		 // indicates if the GPRS is attached to the network (CGATT reply), not if the UDP socket is connected.
		bool socketReady;	 // indicates if the socket is connected or not (result of CIPSTATUS)
		
		bool readyToSend;  	 // This indicates that the "> " has been recieved and that the SIM800L is now ready for the bianry data.
		bool sendCompleted; // This inducates that the send process has ended and that the SIM800 is now ready for more AT commands.
		
		uint8_t sendErrorCount; // number of send errors in a row, will clear to 0 when transmission is successfull.
		uint8_t underVoltageWarnings; // number of underVoltageWarnings recieved. This will not clear, but stop at 255
		uint8_t overVoltageWarnings;  // number of overVoltageWarnings recieved. This will not clear, but stop at 255
		
		uint32_t totalErrorCount; 	// counts the total number of failed transmission.
		uint32_t totalServiceCount; // count the total number of services.
		uint32_t totalByteTransmittedCount; // count the total number of bytes transmitted.
				
		String ip;				  // The current IP on the network, from CIPSTART
		ATCommands_t lastCommand; // current command state
		ATCommands_t beforeRecieve; 
		uint8_t commandIndex; 	  // index counter to keep track of replyes in a command
		
		uint8_t rssi;			  // Last known RSSI, from CSQ
		uint8_t ber;			  // Last known Bit Error Rate, from CSQ
	};	
	ATstatus_t atStatus;	
		
	// Link statemachine
	enum GPRSLinkState_t{
	  RESET=0,
	  ENABLE_ECHO_BACK,
	  DISABLE_ECHO_BACK,
	  STARTUP,
	  IDLE,
	  SENDING_CONNAND,
	  SENDING_DATA,
	  TIMEOUT,
	  SERVICE
	};
	
	struct linkState_t
	{
		uint32_t nowTime;
		uint32_t nextTimeout;		
		uint32_t nextTimeToSend;
		bool timeToSend;
		bool timeToService;
		uint32_t nextRSSITime;
		GPRSLinkState_t state = RESET;
		GPRSLinkState_t lastState = RESET;
	};	
	linkState_t linkState;


	// rx input statemachine
	enum ATReadState{
	  AT_MODE=0,
	  DATA_MODE
	};

	struct rxState_t
	{
		ATReadState state = AT_MODE;
		uint8_t data[AT_BUFFER_SIZE];
		uint16_t length = 0;
		uint16_t index = 0;
		uint16_t dataModeIndex = 0;
	};	
	rxState_t rxState;
	
	//RX/TX FIFO data_
	RingBuf<uint8_t, FIFO_SIZE> rxFIFO;	
	RingBuf<uint8_t, FIFO_SIZE> txFIFO;
	uint16_t bytesToSend;
	
	// Private helper funcions:
	bool parserInput(uint8_t nextByte); // returns true when data in rxState must be handled by the link statemachine.
	void updateATstatus(String command); // takes a string AT command and updates the atStatus accordintly.
	void goToCommand(ATCommands_t command);
	void goToLinkState(GPRSLinkState_t state);
	void updateRSSI(String command);
	void updateConnectionStatus(String command);
	void setNextTimeout(uint32_t nextTime);
	void reset(void);
	void setNextRSSITime(void);
	bool timeToGetRSSI(void);
	void sendData(void);
	bool readAck(String command);
	uint16_t getReceiveLength(String command);
	
	//Debug:
	void debugPrint(String text, bool nl);
	String linkStateToString(GPRSLinkState_t state);
	String atStateToString(ATReadState state);
	String aTCommandsToString(ATCommands_t command);
	void completeDebugPrint(void);
	uint16_t debugIdleCounter=0;
	
	uint16_t led=0;
	void simulateError(void); // only for debug ofcause!
	
	uint32_t lastDebugTime=0;
	uint32_t lastTotalByteCount=0;
};

	
#endif
