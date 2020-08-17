/*
	sim800l.h

	Copyright (c) 2019 Lagoni
	Not for commercial use
 */ 

#ifndef SIM800L_h
#define SIM800L_h

#include "Arduino.h" // Needed for Strings and Serial
//#include "MavlinkData.h"
#include <SD.h> 	 // for File
#include "MavlinkLite.h" // for SerialData struct

#define DEBUG_SIM800L 0

struct ATReply_t
{
	String prefix;
	String response1;
	String response2;
	bool error;
};

struct SIM800LStatus_t
{
	bool readyOk;
	bool cfunOk;
	bool pinOk;
	bool callOk;
	bool smsOk;
	bool muxOk;
	bool apnOk;	
	bool gprsEnabled;
	bool ipOk;
	bool GPRSConnected;
	bool shutdown;
	bool ateSet;
	String ip;
	bool txmsgReady;
	bool pendingTXdata;
	uint16_t incommingPackageLength;
};
	
class SIM800L
{
  public:
    SIM800L();
	void init(HardwareSerial *ptr1, unsigned long baudrate, File *fileptr, int _resetPin);
	void service();
    ATReply_t sendCommand(String data, uint16_t timeout);
	ATReply_t writeData(uint8_t data[], uint16_t length, uint16_t timeout);

	// new:
	void startUdpSocket(String ip, String port, String apn);
	void writeUdpData(uint8_t data[], uint16_t length);
	void writeUdpData(SerialData_t msg);
	//readUdpData(void);

  private:
  SerialData_t msg;
  File *logFile = NULL;
  uint16_t charcount = 0;
  void printStringAsHEXBytes(String data);
  bool sendOnlyOnceDebug = true;
  int resetPin;
  void restart(void);
  
  enum StateMachine{
	  LOOKING_FOR_START,
	  PREFIX,
	  LINEFEED1,
	  LINEFEED2,
	  LINEFEED3,
	  RESPONSE1,
	  RESPONSE2,
	  SEND_RESPONSE,
	  END
  };
  
  
  enum GPRSLinkState{
	  RESET,
	  SETUP_MUX,
	  IP_INITIAL,
	  IP_START,
	  IP_CONFIG,
	  IP_GPRSACT,
	  IP_STATUS,
	  IP_PROCESSING,
	  WAIT_TO_SEND,
	  START_SEND,
	  SENDING,
	  PDP_DEACT	 
  };

  enum ATReadSIM800L{
	  LOOKING_FOR_CR,
	  READ_PREFIX,
	  READ_RESPONSE1,
	  HANDLE_RESPONSE1,
	  READ_RESPONSE2,
	  HANDLE_RESPONSE2,
	  WAIT_FOR_0x20,
	  WAIT_FOR_0x3E,
	  READ_INCOMMING_DATA,
	  READ_ACK
  };

  SIM800LStatus_t status;
  ATReply_t msgreply;
//  bool make_UDP_data(uint8_t* data, uint8_t message_id);
  String base64_encode(uint8_t bytes_to_encode[], int in_len);
  
  StateMachine state = LOOKING_FOR_START; // ENUM used by state maschine in update();
  ATReadSIM800L rxState = LOOKING_FOR_CR;
  GPRSLinkState linkState = RESET;

  HardwareSerial  *SIM800Serial = NULL;       // Pointer to the serial port used
  
  void waitForReply(uint16_t timeout);
  void resetRXdata(void);
  void serviceGPRSLink(void);
  void serviceRXdata(void);
  String outputData;
  void debugPrint(String);
  void debugPrintTimeStamp(String);
  
  uint16_t CRC = 0;			// Variable for calculated CRC.
  uint16_t CRC_RESULT = 0;	// Variable for calculated CRC result.
  uint16_t index=0;
  uint8_t readByte=0;
  
  const String base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
  // for UDP GPRS
  String ip;
  String port;
  String apn;
  
  //Send error count:
  uint32_t sendErrorCount =0;
};

	
#endif
