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

#include <cstdio>
#include <cstring>

extern "C"{
#include <sys/stat.h>
#include <unistd.h>
#include <3ds/console.h>
}

#include "TWLCard.h"


// Fix compile error. This should be properly initialized if you fiddle with the title stuff!
u8 sysLang = 0;


// Override the default service init/exit functions
extern "C" {
	void __appInit() {
		// Initialize services
		srvInit();
		aptInit();
		gfxInitDefault();
		hidInit();
		fsInit();
		sdmcInit();
		pxiDevInit();
		consoleInit(GFX_TOP, NULL);
	}

	void __appExit() {
		// Exit services
		pxiDevExit();
		sdmcExit();
		fsExit();
		hidExit();
		gfxExit();
		aptExit();
		srvExit();
	}
}

void updateProgressBar(u32 offset, u32 total) {
	const int nbBars = 40;
	std::string bar(nbBars*offset/total, '#');
	
	bar += std::string(nbBars - nbBars*offset/total, '-');
	
	printf("\r[%s]\t%d%%", bar.c_str(), (int)(100*offset/total));
	
	gfxFlushBuffers();
	gfxSwapBuffers();
}
	
std::string sizeToStr(u32 sz) {
	char buf[50];
	if(sz < 1024)
		sprintf(buf, "%lu B", sz);
	else if(sz < (1 << 20))
		sprintf(buf, "%lu KB", sz >> 10);
	else sprintf(buf, "%lu MB", sz >> 20);
	
	return std::string(buf);
}

#define HANDLE_ERRORS()\
catch(Error const& e){\
	e.describe();\
	once = true;\
	goto end_error;\
}\
catch(std::exception const& e){\
	printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());\
	once = true;\
	goto end_error;\
}


int main(void) {
	mkdir("sdmc:/TWLSaveTool", 0777);
	chdir("sdmc:/TWLSaveTool");
restart:

	u8 fileNumber = 0;

	bool once = false, error_occured = false;
	TWLCard::Header h;
	TWLCard* card = NULL;
	consoleClear();

	printf("\x1b[1m\x1b[0;12HTWLSaveTool 1.1 by TuxSH\x1B[0m\n\n\n");

	try {
		card = new TWLCard;

		if(card->isTWL()) {
			h = card->cardHeader();
			
			if(card->cardType() == NO_CHIP) {
				delete card;
				card = NULL;
				printf("\x1B[31mUnsupported save file type. It is possible that this game doesn't store any save data.\x1B[0m\n");
				error_occured = true;
			}
			
			printf("Game title:\t\t\t%s\nGamecode:\t\t\t%s\n", h.gameTitle.c_str(), h.gameCode.c_str());
			printf("Save file size:\t%s\n", sizeToStr(card->saveSize()).c_str());
			if(card->cardType() >= FLASH_256KB_1)
				printf("JEDEC ID:\t\t\t0x%lx\n", card->JEDECID());
			printf("\n(B)\t\t\t\tBackup save file\n(A)\t\t\t\tRestore save file\n(X)\t\t\t\tErase save file\n");
			printf("(START)\t\t\tExit\n(Y)\t\t\t\tRestart\n");
			printf("(LEFT/RIGHT)\tChange file name\n");
			printf("Current save file name: %s", card->generateFileName(fileNumber).c_str());
		}
		
		else{
			delete card;
			card = NULL;
			printf("\x1B[31mPlease insert a valid NDS game card!\x1B[0m\n");
			error_occured = true;
		}
	}
	catch(Error const& e){
		delete card;
		card = NULL;
		e.describe();
		error_occured = true;
	}
	catch(std::exception const& e){
		delete card;
		card = NULL;
		printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());
		error_occured = true;
	}
	
		
	while(aptMainLoop()) {
		hidScanInput();
		auto keys = hidKeysDown(); 
		
		if(keys & KEY_START) break;
		else if(keys & KEY_Y){	
		//	delete card; card = NULL;
			gfxFlushBuffers();
			gfxSwapBuffers();
			gspWaitForVBlank();
			goto restart;
		}
		else if(keys & (KEY_LEFT | KEY_RIGHT)){
			if(keys & KEY_LEFT) --fileNumber;
			else if(keys & KEY_RIGHT) ++fileNumber;
			printf("\rCurrent save file name: %s   ", card->generateFileName(fileNumber).c_str());
			goto flush_buffers;
		}
		
		if(!once) { 
			if(error_occured) {
					once = true;
					goto end_error;
			}
			if (keys & (KEY_B | KEY_A | KEY_X)) {
					std::string fileName = card->generateFileName(fileNumber);
					FILE* f = NULL;
					printf("\n\n\n");
					
					if(keys & KEY_B){
						try{
							f = fopen(fileName.c_str(), "rb");
							if(f != NULL){
								fclose(f);
								printf("\x1B[33mFile %s already exists. Overwrite?\n\n(UP) Yes\t\t(DOWN) No\x1B[0m\n", fileName.c_str());
								while(aptMainLoop()){
									hidScanInput();
									if(hidKeysDown() & KEY_UP) break;
									else if(hidKeysDown() & KEY_DOWN) goto end_error;
								}
							}
							
							printf("\nReading save file...\n\n");
							card->backupSaveFile(fileName, &updateProgressBar);
							printf("\n");
						}
						HANDLE_ERRORS();
					}
					
					
					
					else if(keys & KEY_A) {
						try{
							f = fopen(fileName.c_str(), "rb");
							if(f == NULL){
								printf("\x1B[31mError: cannot open file %s\x1B[0m\n", fileName.c_str());
								goto end_error;
							}
							fseek(f, 0, SEEK_END);
							u32 sz = ftell(f);
							fclose(f);
							
							if(sz != card->saveSize()){
								printf("\x1B[31mError: incorrect file size\x1B[0m\n");
								goto end_error;
							}
							
							printf("\nWriting save file...\n\n");
							card->restoreSaveFile(fileName, &updateProgressBar);
							printf("\n");
						}
						HANDLE_ERRORS();
					}
					
					else if(keys & KEY_X) {
						try{
							printf("\x1B[33mAre you REALLY sure you want to erase your save data?\n\n(UP) Yes\t\t(DOWN) No\x1B[0m\n");
							while(aptMainLoop()){
								hidScanInput();
								if(hidKeysDown() & KEY_UP) break;
								else if(hidKeysDown() & KEY_DOWN) goto end;
							}
							printf("\nErasing save data...\n\n");
							card->eraseSaveData(&updateProgressBar);
							printf("\n");
						}
						HANDLE_ERRORS();
					}
					
				end:
					once = true;
					printf("\nDone.\n");
			}
			else goto flush_buffers;	
		end_error:
			once = true;
			printf("Press START to exit, Y to restart.\n");
		}
			
		
flush_buffers:
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	if(fill_buf != NULL) delete[] fill_buf;
	delete card;
	return 0;
}
