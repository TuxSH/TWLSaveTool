#pragma once

#include <3ds.h>
#include <string>
#include <utility>
#include <algorithm>
#include "errors.h"

extern const int validSizes[7];

namespace TWLCard {
	struct Header {
		std::string gameTitle, gameCode, makerCode;
		bool isTWL; // "DSi enhanced"
		
		std::string generateCodeName(void) const;
		Header(u8* data = NULL);
	 };
	 
	bool isCardTWL(void);
	Header getHeader(void);
	
	Handle openSaveFile(u32 openFlags);
	void closeSaveFile(Handle f);
	
	u64 getSaveFileSize(void);
	u64 getSPISize(void);

	template<class CallbackT>
	void readSaveFile(u8* out, u64 nb, CallbackT cb){
		Handle f = openSaveFile(FS_OPEN_READ);
		Result res;
		
		u32 bytesRead;
		
		u64 offset = 0;
		u64 spiSize = getSPISize();
		
		for(offset = 0; offset < nb; offset += 512){
			u64 expected = (nb-offset < 512) ? (nb-offset) : 512;
			res = FSFILE_Read(f, &bytesRead, offset, out+offset, expected);
			if(res != 0){ closeSaveFile(f); throw Error(res,__FILE__, __LINE__); }
			if(expected != bytesRead) { closeSaveFile(f); throw std::runtime_error("too few bytes were read"); }
			cb(offset, nb);
		}
		cb(nb,nb);
		
		closeSaveFile(f);
	}
	
	template<class CallbackT>
	void writeToSaveFile(u8* in, u64 nb, CallbackT cb){
		Handle f = openSaveFile(FS_OPEN_WRITE);
		Result res;
		
		u32 bytesWritten;
		
		u64 offset = 0;
		
		for(offset = 0; offset < nb; offset += 512){
			u64 expected = (nb-offset < 512) ? (nb-offset) : 512;
			res = FSFILE_Write(f, &bytesWritten, offset, in+offset, expected, FS_WRITE_FLUSH);
			if(res != 0){ closeSaveFile(f); throw Error(res,__FILE__, __LINE__); }
			if(expected != bytesWritten) { closeSaveFile(f); throw std::runtime_error("too few bytes were written"); }
			cb(offset, nb);
		}
		
		cb(nb, nb);
		
		closeSaveFile(f);
	}
	
 }
 
 
 