/*
	mavPackageFilter.cpp

	Copyright (c) 2020 Lagoni
	Not for commercial use
 */ 
#include "mavPackageFilter.h"



// Constructor

 MavPackageFilter::MavPackageFilter(){
	this->clear();
 }
  
 void MavPackageFilter::clear(void){
	for(int index=0;index<MAX_MAVLINK_MSG;index++){
		filterList[index]=UNKONWN_PACKAGE;
	}
 }
 
 void MavPackageFilter::setAllPackageFilter(MavFilter_t filter){ 
	for(int index=0;index<MAX_MAVLINK_MSG;index++){
			filterList[index]=filter;
	}
 }
  
 void MavPackageFilter::setPackageFilter(uint8_t msgid, MavFilter_t filter){ 
	if(msgid<MAX_MAVLINK_MSG)
		filterList[msgid]=filter;
 }
 
 MavFilter_t MavPackageFilter::getPackageFilter(uint8_t msgid){
	if(msgid<MAX_MAVLINK_MSG)
		return filterList[msgid];	 
	else
		return UNKONWN_PACKAGE;
 }
  
 