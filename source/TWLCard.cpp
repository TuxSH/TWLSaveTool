#include "TWLCard.h"

namespace TWLCard {
	Header::Header(u8* data) {
		if(data == NULL) return;
		char in1[13] = {0}, in2[5] = {0}, in3[3] = {0};
		
		std::copy(data, data + 12, in1);
		std::copy(data + 12, data + 16, in2);
		std::copy(data + 16, data + 18, in3);
		
		gameTitle = std::string(in1);
		gameCode = std::string(in2);
		makerCode = std::string(in3);
	}
	
	
	bool isCardTWL(void) {
		FS_CardType t;
		Result res = FSUSER_GetCardType(&t);
		if(res != 0) throw Error(res);
		return t == CARD_TWL;
	}
	
	Header getHeader(void) {
		u8 data[18] = {0};
		Result res = FSUSER_GetLegacyRomHeader2(18, MEDIATYPE_GAME_CARD, 0LL, data);
		if(res != 0) throw Error(res);
		return Header(data);
	}
	
	Handle openSaveFile(u32 openFlags) {
		Handle f;
		Result res =  FSUSER_OpenFileDirectly(&f, (FS_Archive){ARCHIVE_CARD_SPIFS, fsMakePath(PATH_EMPTY, NULL)}, 
											  fsMakePath(PATH_UTF16, L"/"), openFlags, 0);
		
		if(res != 0) throw Error(res);
		return f;
	}
	
	void closeSaveFile(Handle f) {
		Result res = FSFILE_Close(f);
		if(res != 0) throw Error(res);
	}
	
	u64 getSaveFileSize(void) {
		Handle f = openSaveFile(FS_OPEN_READ);
		u64 sz;
		Result res = FSFILE_GetSize(f, &sz);
		if(res != 0) throw Error(res);
		closeSaveFile(f);
		return sz;
	}
	
	void readSaveFile(u8* out, u64 nb) {
		Handle f = openSaveFile(FS_OPEN_READ);
		Result res;
		
		if(nb == 0){
			FSFILE_GetSize(f, &nb);
			if(res != 0) throw Error(res);
		}
		
		u32 bytesRead;
		
		res = FSFILE_Read(f, &bytesRead, 0LL, out, (u32)nb);
		
		if(res != 0) throw Error(res);
		if(nb != bytesRead) throw std::runtime_error("too few bytes were read");
		
		closeSaveFile(f);
	}
	
	void writeToSaveFile(u8* out, u64 nb) {
		if(nb == 0)
			nb = getSaveFileSize();
		
		Handle f = openSaveFile(FS_OPEN_WRITE);
		Result res;
		
		if(nb == 0){
			FSFILE_GetSize(f, &nb);
			if(res != 0) throw Error(res);
		}
		
		u32 bytesWritten;
		
		res = FSFILE_Write(f, &bytesWritten, 0LL, out, (u32)nb, FS_WRITE_FLUSH);
		
		if(res != 0) throw Error(res);
		if(nb != bytesWritten) throw std::runtime_error("too few bytes were written");
		
		closeSaveFile(f);
	}

}
