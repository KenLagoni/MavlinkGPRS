/*
	mavPackageFilter.h

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 

#ifndef MAVPACKAGEFILTER_H_
#define MAVPACKAGEFILTER_H_


#if defined (__AVR__) || (__avr__) || (ARDUINO_SAMD_MKRZERO)	 // Arduino MKR Zero
	#include "mavlinkArduino.h"
#else
	#include <stdint.h> 	
#endif

  enum MavFilter_t{
    DISCARD=0,
    FORWARD,
    HANDLE,
    UNKONWN_PACKAGE
  };

class MavPackageFilter
{
	// Public functions to be used on all Messages
	public:
	
	MavPackageFilter(); 
	
	void clear(void);
	void setPackageFilter(uint8_t msgid, MavFilter_t filter);
	MavFilter_t getPackageFilter(uint8_t msgid);
	void setAllPackageFilter(MavFilter_t filter);
	
	// General helper functions and varibels only used by inherited classes.
	protected:
			
	
	// Parameters only used on mother class.
	private:
	const static int MAX_MAVLINK_MSG = 256;
	MavFilter_t filterList[MAX_MAVLINK_MSG];
};

#endif /* MAVPACKAGEFILTER_H_ */