#pragma once
#include <3ds.h>
#include <string>
#include <utility>
#include <algorithm>
#include "errors.h"


namespace TWLCard {
	struct Header {
		std::string gameTitle, gameCode, makerCode;		
		Header(u8* data = NULL);
	 };
	 
	bool isCardTWL(void);
	Header getHeader(void);
	
	Handle openSaveFile(u32 openFlags);
	void closeSaveFile(Handle f);
	
	u64 getSaveFileSize(void);
	void readSaveFile(u8* out, u64 nb = 0);
	void writeToSaveFile(u8* in, u64 nb = 0);
 }
 
 
 