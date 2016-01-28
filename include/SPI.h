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

#pragma once
#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_CMD_RDSR 5
#define SPI_CMD_WREN 6

#define SPI_512B_EEPROM_CMD_WRLO 2
#define SPI_512B_EEPROM_CMD_WRHI 10
#define SPI_512B_EEPROM_CMD_RDLO 3
#define SPI_512B_EEPROM_CMD_RDHI 11

#define SPI_CMD_PP 2
#define SPI_CMD_READ 3

#define SPI_FLASH_CMD_PW 10
#define SPI_FLASH_CMD_RDID 0x9f
#define SPI_FLASH_CMD_SE 0xd8


#define SPI_FLG_WIP 1
#define SPI_FLG_WEL 2

Result pxiDevInit(void);
void pxiDevExit(void);

typedef enum {
	NO_CHIP = -1,
	EEPROM_512B = 0, // <- sure
	EEPROM_8KB = 1, // <- sure
	EEPROM_64KB = 2, // <- sure
	FLASH_256KB_1 = 3,
	FLASH_256KB_2 = 4,
	FLASH_512KB_1 = 5,
	FLASH_512KB_2 = 6,
	FLASH_1MB = 7,
	FLASH_8MB = 8, // <- sure, can't restore savegames atm
	FLASH_512KB_INFRARED = 9, // <- sure
	FLASH_256KB_INFRARED = 10 // AFAIK only "Active Health with Carol Vorderman" has such a save memory
} CardType;

typedef enum {
	WAIT_NONE = 0, WAIT_SLEEP = 1, WAIT_IREQ_RETURN = 2, WAIT_IREQ_CONTINUE = 3
} WaitType;

typedef enum {
	DEASSERT_NONE = 0, DEASSERT_BEFORE_WAIT = 1, DEASSERT_AFTER_WAIT = 2
} DeassertType;
 
typedef struct __attribute__((packed)){
	u8 baudRate:6; // 512kHz << baudRate
	u8 busMode:2; // 0 = 1-bit, 1 = 4-bit 
} TransferOption;

typedef struct  __attribute__((packed)) {
	WaitType wait:4;
	DeassertType deassert:4;
	u64 duration: 60; // ns
} WaitOperation;

// Longest signature ever!

Result PXIDEV_SPIMultiWriteRead(
	u64 header, u32 headerSize, TransferOption headerTransferOption, WaitOperation headerWaitOperation,
	void* writeBuffer1, u32 wb1Size, TransferOption wb1TransferOption, WaitOperation wb1WaitOperation,
	void* readBuffer1, u32 rb1Size, TransferOption rb1TransferOption, WaitOperation rb1WaitOperation,
	void* writeBuffer2, u32 wb2Size, TransferOption wb2TransferOption, WaitOperation wb2WaitOperation,
	void* readBuffer2, u32 rb2Size, TransferOption rb2TransferOption, WaitOperation rb2WaitOperation,
	u64 footer, u32 footerSize, TransferOption footerTransferOption
);

Result SPIWriteRead(CardType type, void* cmd, u32 cmdSize, void* answer, u32 answerSize, void* data, u32 dataSize);
Result SPIWaitWriteEnd(CardType type);
Result SPIEnableWriting(CardType type);
Result SPIReadJEDECIDAndStatusReg(CardType type, u32* id, u8* statusReg);
Result SPIGetCardType(CardType* type, int infrared);
u32 SPIGetPageSize(CardType type);
u32 SPIGetCapacity(CardType type);

Result SPIWriteSaveData(CardType type, u32 offset, void* data, u32 size);
Result SPIReadSaveData(CardType type, u32 offset, void* data, u32 size);

Result SPIEraseSector(CardType type, u32 offset);
//Result SPIErase(CardType type); 

#ifdef __cplusplus
}
#endif