#include "TWLCard.h"

#include "games.h"

const int validSizes[7] = { 512, 8192, 65536, 262144, 524288, 1048576, 8388608 };

#define __FILE__ "TWLCard.cpp"
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
		isTWL = (data[0x12] & 0x2) != 0;
	}
	
	std::string Header::generateCodeName(void) const {
		using std::string;
		string first = (isTWL) ? string("TWL") : string("NTR");
		return first + string("-") + gameCode + string("-") + makerCode;
	}
	
	
	bool isCardTWL(void) {
		FS_CardType t;
		Result res = FSUSER_GetCardType(&t);
		if(res != 0) throw Error(res,__FILE__, __LINE__);
		return t == CARD_TWL;
	}
	
	Header getHeader(void) {
		u8* data = new u8[0x3b4];
		Result res = FSUSER_GetLegacyRomHeader(MEDIATYPE_GAME_CARD, 0LL, data);
		if(res != 0) throw Error(res,__FILE__, __LINE__);
		Header ret(data);
		delete[] data;
		return ret;
	}
	
	Handle openSaveFile(u32 openFlags) {
		Handle f;
		Result res =  FSUSER_OpenFileDirectly(&f, (FS_Archive){ARCHIVE_CARD_SPIFS, fsMakePath(PATH_EMPTY, NULL)}, 
											  fsMakePath(PATH_UTF16, L"/"), openFlags, 0);
		
		if(res != 0) throw Error(res,__FILE__, __LINE__);
		return f;
	}
	
	void closeSaveFile(Handle f) {
		Result res = FSFILE_Close(f);
		if(res != 0) throw Error(res,__FILE__, __LINE__);
	}
	
	u64 getSaveFileSize(void) {
		int sz = 0;
		u8* buf = new u8[512];
		u8* buf2 = new u8[512];
		
		Handle f;
		u32 bytesRead;
		if(FSUSER_OpenFileDirectly(&f, (FS_Archive){ARCHIVE_CARD_SPIFS, fsMakePath(PATH_EMPTY, NULL)}, 
											  fsMakePath(PATH_UTF16, L"/"), FS_OPEN_READ, 0) != 0) goto end1;
		
		
		if(FSFILE_Read(f, &bytesRead, 0LL, buf, 512) != 0 || bytesRead != 512) goto end;

		for(int offset : validSizes){
			if(FSFILE_Read(f, &bytesRead, offset, buf2, 512) != 0 || bytesRead != 512) goto end;
			if(std::equal(buf, buf+512, buf2)){
				sz = offset;
				break;
			}
		}
		end: FSFILE_Close(f); 
		end1:
		delete[] buf;
		delete[] buf2;
		return (u64) sz;
	}
	
	u64 getSPISize(void){
		Handle f = openSaveFile(FS_OPEN_READ);
		u64 sz;
		Result res = FSFILE_GetSize(f, &sz);
		if(res != 0) throw Error(res,__FILE__, __LINE__);
		closeSaveFile(f);
		return sz;
	}
}
