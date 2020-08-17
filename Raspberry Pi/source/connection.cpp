/*
	Connection.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "connection.h"

// Constructor

 Connection::Connection(){
	this->_fd=0;
	this->_port=0;
	this->_type=0;
	bzero(&this->_servaddr, sizeof(this->_servaddr));	
	bzero(&this->_cliaddr, sizeof(this->_cliaddr));	
 }
 
 Connection::Connection(int port, int type){
	this->_fd=0;
	this->_port=0;
	this->_type=0;
	bzero(&this->_servaddr, sizeof(this->_servaddr));	
	bzero(&this->_cliaddr, sizeof(this->_cliaddr));	
	this->initConnection(port, type);
 }
 
 
 void Connection::initConnection(int port, int type){
	 
	this->_type=type;
	this->_port=port;
	bzero(&this->_servaddr, sizeof(this->_servaddr));	
	bzero(&this->_cliaddr, sizeof(this->_cliaddr));	
	
	this->_servaddr.sin_family = AF_INET; 
	this->_servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	this->_servaddr.sin_port = htons(this->_port); 

	//printf("Connection Class:1\n");
	
	if(this->_type == SOCK_STREAM){
	    // create listening TCP socket 
		this->_fd = socket(AF_INET, SOCK_STREAM, 0); 
		// binding server addr structure to this->_fd 
		bind(this->_fd, (struct sockaddr*)&this->_servaddr, sizeof(this->_servaddr)); 
		listen(this->_fd, 10); 		
	}else if(this->_type == SOCK_DGRAM){
		// create UDP socket 
		this->_fd = socket(AF_INET, SOCK_DGRAM, 0); 
		// binding server addr structure to this->_fd  
		bind(this->_fd, (struct sockaddr*)&this->_servaddr, sizeof(this->_servaddr)); 	 
	}else{
		this->_fd=0;
		// unknown
	}
 }
  
 void Connection::startConnection(int fd){	
	socklen_t len; 
	len=sizeof(this->_cliaddr);
	this->_fd = accept(fd, (struct sockaddr*)&this->_cliaddr, &len);
	this->isValid=true;
 }
   

 void Connection::setFD_SET(fd_set* fdset){
	FD_SET(this->_fd, fdset); 
 }
  
 int Connection::getFD(){
	return this->_fd;
 }

 void Connection::setFD(int fd){
	this->_fd = fd;
 }
 
 uint16_t Connection::readData(void *buffer, uint16_t maxLength){ // returns number of bytes read.
	 ssize_t n;
	 socklen_t len; 
	 len=sizeof(this->_cliaddr);
	 n = recvfrom(this->_fd, buffer, maxLength, 0, (struct sockaddr*)&this->_cliaddr, &len); 
	
	 if((n < 1)){ // cloding the connection.	 
		 printf("Connection: reading with result=%d, thus closing\n",n);	 
		 close(this->_fd);
		 this->_fd=0;
		 return 0;
	 }
	 if(this->isValid==false)
		this->isValid=true;
	 return n;
 }
  
 uint16_t Connection::writeData(void *buffer, uint16_t length){ // returns?
	ssize_t n;
	socklen_t len; 
	len=sizeof(this->_cliaddr);	
	
	if(this->isValid){
		n = sendto(this->_fd, buffer, length, 0, (struct sockaddr *)&this->_cliaddr, len);
		
		if((n < 1)){ // cloding the connection.	 
			printf("Connection: writing length=%d in FD=%d with result=%d, thus closing\n",length,this->_fd,n);		 
			close(this->_fd);
			this->_fd=0;
			return 0;
		}
		return n;		
	}
	
	return 0;
 }
