/*
	Connection.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 

class Connection
{
	// Public functions to be used on all Messages
	public:
	
	Connection(); // need for array int.
    Connection(int port, int type); //constructor with port and type. This will run the startConnection(port, type)
	//Connection(int fd, struct sockaddr_in); //constructor with file destriptor and client address (used when greated from TCP listen). 
	
	void startConnection(int fd);
	void setFD_SET(fd_set* fdset);
	int getFD();
	void setFD(int fd);
	uint16_t readData(void *buffer, uint16_t length);
	uint16_t writeData(void *buffer, uint16_t length);
	void initConnection(int port, int type);
	/*	
	void AddData(RadioData_t *msg); // Add to TX FIFO.
	void AddData(Telegram *msg);
	RadioData_t * GetData();
	RadioData_t * GetDataIncludingRSSI();

	int Available(); // returns number of Telegrams in RX FIFO.
	void Service();
	*/

	// General helper functions and varibels only used by inherited classes.
	protected:
	/*
	enum RFProtocolStates_t {
		RX_IDLE = 0,
		WAITING_FOR_REPLY,
		TX_WITHOUT_REPLY,
		TX_WITH_REPLY,
	};
	
	virtual uint32_t milliSeconds() = 0;
	virtual RFProtocolStates_t RXHandler() = 0;
	virtual RFProtocolStates_t TXHandler() = 0;
	void _WakeUp();
	void _PowerDown();
	*/
			
	// Parameters only used on mother class.
	private:
	int _fd, _port, _type;
	struct sockaddr_in _servaddr;
	struct sockaddr_in _cliaddr;
	bool isValid = false;
	/*
	unsigned long timeoutStart = 0;
	RadioData_t rxbuffer; // When using "GetData()" the Telegram is removed from FIFO and only RadioData is saved. Telegram is den deleted.
	
	//String base64_encode(byte bytes_to_encode[], int in_len);*/
};


#endif /* CONNECTION_H_ */