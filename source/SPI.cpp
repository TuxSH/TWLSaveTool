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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "SPI.h"
#include <cstring>

static Handle pxiDevHandle;
static int pxiDevRefCount;

// Deliberately written in C! (except for a few lines)

u8* fill_buf = NULL;

Result pxiDevInit(void) {
	Result res;
	if (AtomicPostIncrement(&pxiDevRefCount)) return 0;
	res = srvGetServiceHandle(&pxiDevHandle, "pxi:dev");
	if (R_FAILED(res)) AtomicDecrement(&pxiDevRefCount);
	return res;
}

void pxiDevExit(void) {
	if (AtomicDecrement(&pxiDevRefCount)) return;
	svcCloseHandle(pxiDevHandle);
}

typedef union {
	u8 asU8;
	TransferOption asTrOp;
} Conv1;

typedef union {
	u64 asU64;
	WaitOperation asWaitOp;
} Conv2;

Result PXIDEV_SPIMultiWriteRead(
	u64 header, u32 headerSize, TransferOption headerTransferOption, WaitOperation headerWaitOperation,
	void* writeBuffer1, u32 wb1Size, TransferOption wb1TransferOption, WaitOperation wb1WaitOperation,
	void* readBuffer1, u32 rb1Size, TransferOption rb1TransferOption, WaitOperation rb1WaitOperation,
	void* writeBuffer2, u32 wb2Size, TransferOption wb2TransferOption, WaitOperation wb2WaitOperation,
	void* readBuffer2, u32 rb2Size, TransferOption rb2TransferOption, WaitOperation rb2WaitOperation,
	u64 footer, u32 footerSize, TransferOption footerTransferOption
) {
	
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();
	
	cmdbuf[0] = 0xd0688;
	
	Conv1 c1;
	Conv2 c2;
	
#define PACK_HEADER_FOOTER(what, where)\
	*(u64*)(cmdbuf + where) = what;\
	cmdbuf[where + 2] = what##Size;\
	c1.asTrOp = what##TransferOption; cmdbuf[where + 3] = (u32) c1.asU8;\

#define PACK_BUFFER(lg, sh, type, n, cst)\
	cmdbuf[27 + 4*type + 2*n] = (sh##Size << 8) | cst;\
	cmdbuf[28 + 4*type + 2*n] = (u32) lg;\
	cmdbuf[7 + 4*type + 8*n] = sh##Size;\
	c1.asTrOp = sh##TransferOption; cmdbuf[8 + 4*type + 8*n] = (u32) c1.asU8;\
	c2.asWaitOp = sh##WaitOperation; *(u64*)(cmdbuf + 9 + 4*type + 8*n) = c2.asU64;
	
	PACK_HEADER_FOOTER(header, 1);
	c2.asWaitOp = headerWaitOperation; *(u64*)(cmdbuf + 5) = c2.asU64;
	
	PACK_BUFFER(writeBuffer1, wb1, 0, 0, 0x06);
	PACK_BUFFER(readBuffer1, rb1, 1, 0, 0x24);
	PACK_BUFFER(writeBuffer2, wb2, 0, 1, 0x16);
	PACK_BUFFER(readBuffer2, rb2, 1, 1, 0x34);
	
	PACK_HEADER_FOOTER(footer, 23);
	
#undef PACK_HEADER_FOOTER
#undef PACK_BUFFER

	if(R_FAILED(ret = svcSendSyncRequest(pxiDevHandle))) return ret;
	return (Result)cmdbuf[1];
}

Result SPIWriteRead(CardType type, void* cmd, u32 cmdSize, void* answer, u32 answerSize, void* data, u32 dataSize) {
	TransferOption transferOp = { BAUDRATE_4MHZ }, transferOp2 = { BAUDRATE_1MHZ };
	WaitOperation waitOp = { WAIT_NONE };
	
	bool b = type == FLASH_512KB_INFRARED || type == FLASH_256KB_INFRARED;
	return PXIDEV_SPIMultiWriteRead(
		0LL, (b) ? 1 : 0, (b) ? transferOp2 : transferOp, waitOp, // header
		cmd, cmdSize, transferOp, waitOp,
		answer, answerSize, transferOp, waitOp,
		data, dataSize, transferOp, waitOp,
		NULL, 0, transferOp, waitOp,
		0LL, 0, transferOp // footer
	);
}

Result SPIWaitWriteEnd(CardType type) {
	u8 cmd = SPI_CMD_RDSR, statusReg = 0;
	Result res = 0;
	
	do{
		res = SPIWriteRead(type, &cmd, 1, &statusReg, 1, 0, 0);
		if(res) return res;
	} while(statusReg & SPI_FLG_WIP);
	
	return 0;
}

Result SPIEnableWriting(CardType type) {
	u8 cmd = SPI_CMD_WREN, statusReg = 0;
	Result res = SPIWriteRead(type, &cmd, 1, NULL, 0, 0, 0);

	if(res || type == EEPROM_512B) return res; // Weird, but works (otherwise we're getting an infinite loop for that chip type).
	cmd = SPI_CMD_RDSR;
	
	do{
		res = SPIWriteRead(type, &cmd, 1, &statusReg, 1, 0, 0);
		if(res) return res;
	} while(statusReg & ~SPI_FLG_WEL);
	
	return 0;
}

Result SPIReadJEDECIDAndStatusReg(CardType type, u32* id, u8* statusReg) {
	u8 cmd = SPI_FLASH_CMD_RDID;
	u8 reg = 0;
	u8 idbuf[3] = { 0 };
	u32 id_ = 0;
	Result res = SPIWaitWriteEnd(type);
	
	if(res) return res;
	if((res = SPIWriteRead(type, &cmd, 1, idbuf, 3, 0, 0))) return res;
	id_ = (idbuf[0] << 16) | (idbuf[1] << 8) | idbuf[2];
	cmd = SPI_CMD_RDSR;
	if((res = SPIWriteRead(type, &cmd, 1, &reg, 1, 0, 0))) return res;
	
	if(id) *id = id_;
	if(statusReg) *statusReg = reg;
	
	return 0;
}

u32 SPIGetPageSize(CardType type) {
	u32 EEPROMSizes[] = { 16, 32, 128 };
	if(type == NO_CHIP || (int) type > 10) return 0;
	else if((int) type <= 2) return EEPROMSizes[(int) type];
	else return 256;
}

u32 SPIGetCapacity(CardType type) {
	u32 sz[] = { 9, 13, 16, 18, 18, 19, 19, 20, 23, 19, 19 };
	
	if(type == NO_CHIP) return 0;
	else return 1 << sz[(int) type];
}

Result SPIWriteSaveData(CardType type, u32 offset, void* data, u32 size) {
	u8 cmd[4] = { 0 };
	u32 cmdSize = 4;
	
	u32 end = offset + size;
	u32 pos = offset;
	if(size == 0) return 0;
	u32 pageSize = SPIGetPageSize(type);
	if(pageSize == 0) return 0xC8E13404;
	
	Result res = SPIWaitWriteEnd(type);
	if(res) return res;
	
	size = (size <= SPIGetCapacity(type) - offset) ? size : SPIGetCapacity(type) - offset; 

	while(pos < end) {
		switch(type) {
			case EEPROM_512B:
				cmdSize = 2;
				cmd[0] = (pos >= 0x100) ? SPI_512B_EEPROM_CMD_WRHI : SPI_512B_EEPROM_CMD_WRLO;
				cmd[1] = (u8) pos;
				break;
			case EEPROM_8KB:
			case EEPROM_64KB:
				cmdSize = 3;
				cmd[0] = SPI_CMD_PP;
				cmd[1] = (u8)(pos >> 8);
				cmd[2] = (u8) pos;
				break;
			case FLASH_256KB_1:
			/*	
			This is what is done in the official implementation, but I think it's wrong
				cmdSize = 4;
				cmd[0] = SPI_CMD_PP;
				cmd[1] = (u8)(pos >> 16);
				cmd[2] = (u8)(pos >> 8);
				cmd[3] = (u8) pos;
				break;
			*/
			case FLASH_256KB_2:
			case FLASH_512KB_1:
			case FLASH_512KB_2:
			case FLASH_1MB:
			case FLASH_512KB_INFRARED:
			case FLASH_256KB_INFRARED:
				cmdSize = 4;
				cmd[0] = SPI_FLASH_CMD_PW;
				cmd[1] = (u8)(pos >> 16);
				cmd[2] = (u8)(pos >> 8);
				cmd[3] = (u8) pos;
				break;
			case FLASH_8MB:
				return 0xC8E13404; // writing is unsupported (so is reading? need to test)
			default:
				return 0; // never happens
		}
		
		u32 remaining = end - pos;
		u32 nb = pageSize - (pos % pageSize);
		
		u32 dataSize = (remaining < nb) ? remaining : nb;
		
		if( (res = SPIEnableWriting(type)) ) return res;
		if( (res = SPIWriteRead(type, cmd, cmdSize, NULL, 0, (void*) ((u8*) data - offset + pos), dataSize)) ) return res;
		if( (res = SPIWaitWriteEnd(type)) ) return res;
		
		pos = ((pos / pageSize) + 1) * pageSize; // truncate
	}
	
	return 0;
}

Result _SPIReadSaveData_512B_impl(u32 pos, void* data, u32 size) { 
	u8 cmd[4];
	u32 cmdSize = 2;
	
	u32 end = pos + size;
	
	u32 read = 0;
	if(pos < 0x100) {
		u32 len = 0x100 - pos;
		cmd[0] = SPI_512B_EEPROM_CMD_RDLO;
		cmd[1] = (u8) pos;
		
		Result res = SPIWriteRead(EEPROM_512B, cmd, cmdSize, data, len, NULL, 0);
		if(res) return res;
		
		read += len;
	}
	
	if(end >= 0x100) {
		u32 len = end - 0x100;

		cmd[0] = SPI_512B_EEPROM_CMD_RDHI;
		cmd[1] = (u8)(pos + read);
		
		Result res = SPIWriteRead(EEPROM_512B, cmd, cmdSize, (void*)((u8*)data + read), len, NULL, 0);

		if(res) return res;
	}
	
	return 0;
}

Result SPIReadSaveData(CardType type, u32 offset, void* data, u32 size) {	
	u8 cmd[4] = { SPI_CMD_READ };
	u32 cmdSize = 4;
	if(size == 0) return 0;
	if(type == NO_CHIP) return 0xC8E13404;
	
	Result res = SPIWaitWriteEnd(type);
	if(res) return res;
	
	size = (size <= SPIGetCapacity(type) - offset) ? size : SPIGetCapacity(type) - offset; 
	u32 pos = offset;
	switch(type) {
		case EEPROM_512B:
			return _SPIReadSaveData_512B_impl(offset, data, size);
			break;
		case EEPROM_8KB:
		case EEPROM_64KB:
			cmdSize = 3;
			cmd[1] = (u8)(pos >> 8);
			cmd[2] = (u8) pos;
			break;
		case FLASH_256KB_1:
		case FLASH_256KB_2:
		case FLASH_512KB_1:
		case FLASH_512KB_2:
		case FLASH_1MB:
		case FLASH_8MB:
		case FLASH_512KB_INFRARED:
		case FLASH_256KB_INFRARED:
			cmdSize = 4;
			cmd[1] = (u8)(pos >> 16);
			cmd[2] = (u8)(pos >> 8);
			cmd[3] = (u8) pos;
			break;
		default:
			return 0; // never happens
	}
	
	return SPIWriteRead(type, cmd, cmdSize, data, size, NULL, 0);
}

Result SPIEraseSector(CardType type, u32 offset) {
	u8 cmd[4] = {  SPI_FLASH_CMD_SE, (u8)(offset >> 16), (u8)(offset >> 8), (u8) offset };
	if(type == NO_CHIP || type == FLASH_8MB) return 0xC8E13404;
	
	if(type < FLASH_256KB_1 && fill_buf == NULL) { 
			fill_buf = new u8[0x10000];
			memset(fill_buf, 0xff, 0x10000);
	}
	
	Result res = SPIWaitWriteEnd(type);
	
	if(type >= FLASH_256KB_1) {
		if( (res = SPIEnableWriting(type)) ) return res;
		if( (res = SPIWriteRead(type, cmd, 4, NULL, 0, NULL, 0)) ) return res;
		if( (res = SPIWaitWriteEnd(type)) ) return res;
	}
	// Simulate the same behavior on EEPROM chips.
	// Since a sector = 0x10000 bytes, it will erase it completely (as there is no known EEPROM chip whose capacity is higher than that size).
	else {
		u32 sz = SPIGetCapacity(type);
		Result res = SPIWriteSaveData(type, 0, fill_buf, sz);
		return res;
	}
	return 0;
}


// The following routine use code from savegame-manager:

/*
 * savegame_manager: a tool to backup and restore savegames from Nintendo
 *  DS cartridges. Nintendo DS and all derivative names are trademarks
 *  by Nintendo. EZFlash 3-in-1 is a trademark by EZFlash.
 *
 * auxspi.cpp: A thin reimplementation of the AUXSPI protocol
 *   (high level functions)
 *
 * Copyright (C) Pokedoc (2010)
 */
/* 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

Result SPIGetCardType(CardType* type, int infrared) {
	u8 sr = 0;
	u32 jedec = 0;
	u32 tries = 0;
	CardType t = (infrared == 1) ? FLASH_512KB_INFRARED : FLASH_512KB_1;
	Result res; 
	u32 jedecOrderedList[] = { 0x204012, 0x621600, 0x204013, 0x621100, 0x204014, 0x202017};
	
	u32 maxTries = (infrared == -1) ? 2 : 1; // note: infrared = -1 fails 1/3 of the time
	while(tries < maxTries){ 
		res = SPIReadJEDECIDAndStatusReg(t, &jedec, &sr); // dummy
		if(res) return res;
		
		if ((sr & 0xfd) == 0x00 && (jedec != 0x00ffffff)) { break; }		
		if ((sr & 0xfd) == 0xF0 && (jedec == 0x00ffffff)) { t = EEPROM_512B; break; }
		if ((sr & 0xfd) == 0x00 && (jedec == 0x00ffffff)) { t = EEPROM_64KB; break; }
		
		++tries;
		t = FLASH_512KB_INFRARED;
	}
	
	if(t == EEPROM_512B) { *type = t; return 0; }
	else if(t == EEPROM_64KB) {
		static const u32 offset0 = (8*1024-1);        //      8KB
		static const u32 offset1 = (2*8*1024-1);      //      16KB
		u8 buf1;     //      +0k data        read -> write
		u8 buf2;     //      +8k data        read -> read
		u8 buf3;     //      +0k ~data          write
		u8 buf4;     //      +8k data new    comp buf2
		
		res = SPIReadSaveData(t, offset0, &buf1, 1);
		if(res) return res;
		res = SPIReadSaveData(t, offset1, &buf2, 1);
		if(res) return res;	
		buf3=~buf1;
		res = SPIWriteSaveData(t, offset0, &buf3, 1);
		if(res) return res;
		res = SPIReadSaveData(t, offset1, &buf4, 1);
		if(res) return res;
		res = SPIWriteSaveData(t, offset0, &buf1, 1);
		if(res) return res;
		
		if(buf4!=buf2)      //        +8k
			t = EEPROM_8KB;  //       8KB(64kbit)
		else
			t = EEPROM_64KB; //      64KB(512kbit)
		
		*type = t;
		return 0;
	}
	
	else if(t == FLASH_512KB_INFRARED) {
		if(infrared == 0) *type = NO_CHIP; // did anything go wrong?
		
		if(jedec == jedecOrderedList[0] || jedec == jedecOrderedList[1]) *type = FLASH_256KB_INFRARED;
		else *type = FLASH_512KB_INFRARED;
		
		return 0;
	}
	
	else {
		if(infrared == 1) *type = NO_CHIP; // did anything go wrong?
		if(jedec == 0x204017) { *type = FLASH_8MB; return 0; } // 8MB. savegame-manager: which one? (more work is required to unlock this save chip!)
		
		int i;
		
		for(i = 0; i < 6; ++i) {
			if(jedec == jedecOrderedList[i]) { *type = (CardType)(3 + i); return 0; }  
		}
		
		*type = NO_CHIP;
		return 0;
	}
}
