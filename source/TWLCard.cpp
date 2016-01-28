/*
 *  This file is part of TWLSaveTool.
 *  Copyright (C) 2015-2016 TuxSH
 *
 *  TWLSaveTool is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/
 */

#include "TWLCard.h"

TWLCard::Header::Header(u8* data) {
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

bool TWLCard::isTWL(void) const {
	return twl;
}

TWLCard::Header TWLCard::cardHeader(void) const {
	return h;
}

u32 TWLCard::saveSize(void) const {
	return SPIGetCapacity(cardType_);
}

CardType TWLCard::cardType(void) const {
	return cardType_;
}

std::string TWLCard::generateFileName(void) const {
	std::string name(h.gameTitle);
	
	for(int i = 0; i < name.size(); ++i) {
		if(!( (name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= 'a' && name[i] <= 'z') || (name[i] >= '0' && name[i] <= '9') || name[i] == ' '))
			name[i] = '_';
	}
	
	return name+".sav";
}

void TWLCard::backupSaveFile(u8* out, void (*cb)(u32, u32)) const {
	u32 sz = saveSize();
	u32 sectorSize = (sz < 0x10000) ? sz : 0x10000;
	
	cb(0, sz);
	for(u32 i = 0; i < sz/sectorSize; ++i) {
		Result res = SPIReadSaveData(cardType_, sectorSize*i, out + sectorSize*i, sectorSize);
		if(res != 0) throw Error(res, __FILE__, __LINE__);
		cb(sectorSize*(i+1), sz);
	}
}

void TWLCard::restoreSaveFile(u8* in, void (*cb)(u32, u32)) const {
	u32 sz = saveSize();
	u32 pageSize = SPIGetPageSize(cardType_);
	
	cb(0, sz);
	for(u32 i = 0; i < sz/pageSize; ++i){
		Result res = SPIWriteSaveData(cardType_, pageSize*i, in + pageSize*i, pageSize);
		if(res != 0) throw Error(res, __FILE__, __LINE__);
		cb(pageSize*(i+1), sz);
	}	
}

void TWLCard::eraseSaveData(void (*cb)(u32, u32)) const {
	u32 pos;
	u32 sz = SPIGetCapacity(cardType_);
	Result res;
	
	cb(0, sz);

	for(pos = 0; pos < sz; pos += 0x10000) {
		res = SPIEraseSector(cardType_, pos);
		if(res != 0) throw Error(res, __FILE__, __LINE__);
		cb((sz < pos + 0x10000) ? sz : pos + 0x10000, sz);
	}
	
	
}

void TWLCard::backupSaveFile(std::string const& filename, void (*cb)(u32, u32)) const {
	FILE* f = fopen(filename.c_str(), "wb+");
	if(f == NULL) throw std::runtime_error("cannot open file");
	
	u8* out = new u8[saveSize()];
	try { backupSaveFile(out, cb); }
	catch(Error const& e) {
		delete[] out;
		fclose(f);
		throw e;
	}
	fwrite(out, saveSize(), 1, f);
	fclose(f);
	
	delete[] out;
}

void TWLCard::restoreSaveFile(std::string const& filename, void (*cb)(u32, u32)) const {
	FILE* f = fopen(filename.c_str(), "rb");
	if(f == NULL) throw std::runtime_error("cannot open file");
	
	u8* in = new u8[saveSize()];
	fread(in, saveSize(), 1, f);
	fclose(f);
	try { restoreSaveFile(in, cb); }
	catch(Error const& e) {
		delete[] in;
		throw e;
	}
	
	delete[] in;
}

TWLCard::TWLCard(void) : twl(false), h(Header()), cardType_(NO_CHIP) {
	FS_CardType t;
	Result res = FSUSER_GetCardType(&t);
	if(res != 0) throw Error(res,__FILE__, __LINE__);
	twl = t == CARD_TWL;
	
	if(!twl) return;
	
	u8* data = new u8[0x3b4];
	res = FSUSER_GetLegacyRomHeader(MEDIATYPE_GAME_CARD, 0LL, data);
	if(res != 0) { delete[] data; throw Error(res,__FILE__, __LINE__); }
	h = Header(data);
	delete[] data;
	
	res = SPIGetCardType(&cardType_, (h.gameCode[0] == 'I') ? 1 : 0); // automatic infrared chip detection often fails
	if(res != 0) { throw Error(res,__FILE__, __LINE__); }
}
