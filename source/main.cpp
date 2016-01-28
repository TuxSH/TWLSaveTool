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

#include <cstdio>
#include <cstring>

extern "C"{
#include <sys/stat.h>
#include <unistd.h>
}

#include "TWLCard.h"
#include <3ds/console.h>


// Fix compile error. This should be properly initialized if you fiddle with the title stuff!
u8 sysLang = 0;


// Override the default service init/exit functions
extern "C"
{
	void __appInit()
	{
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

	void __appExit()
	{
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

void updateProgressBar(u32 offset, u32 total){
	const int nbBars = 40;
	std::string bar(nbBars*offset/total, '#');
	
	bar += std::string(nbBars - nbBars*offset/total, '-');
	
	PrintConsole* csl = consoleGetDefault(); 
	std::string centerPadding((49 - (nbBars + 5 + csl->tabSize))/2, ' ');
	
	printf("\r%s[%s]\t%2d%%", centerPadding.c_str(), bar.c_str(), (int)(100*offset/total));
	fflush(stdout);
}

std::string sizeToStr(u32 sz){
	char buf[50];
	if(sz < 1024)
		sprintf(buf, "%lu B", sz);
	else if(sz < (1 << 20))
		sprintf(buf, "%lu KB", sz >> 10);
	else sprintf(buf, "%lu MB", sz >> 20);
	
	return std::string(buf);
}

int main()
{
restart:

	mkdir("sdmc:/TWLSaveTool", 0777);
	chdir("sdmc:/TWLSaveTool");
	
	bool once = false;
	u64 saveSz = 0;
	TWLCard::Header h;
	u8* data = NULL;
	
	consoleClear();
	printf("\x1b[1m\x1b[0;12HTWLSaveTool 1.0 by TuxSH\x1B[0m\n\n\n");
	
	TWLCard* card = NULL;
	try {
		card = new TWLCard;
		if(card->isTWL()) {
			h = card->cardHeader();
			u32 jedec = 0; u8 reg = 0;
			CardType t = NO_CHIP;
			SPIGetCardType(&t,(h.gameCode[0] == 'I') ? 1 : 0);
			SPIReadJEDECIDAndStatusReg(t, &jedec, &reg);
			printf("%lx\n", jedec); 
			SPIReadJEDECIDAndStatusReg(t, &jedec, &reg);
			printf("%lx\n", jedec); 

			
			printf("Game title:\t%s\nGamecode: %s\n", h.gameTitle.c_str(), h.gameCode.c_str());
			printf("Save file size:\t%s\n", sizeToStr(card->saveSize()).c_str());
			printf("(L)\tBackup save file\n(R)\tRestore save file\n(X)\tErase save file\n(B)\tExit\n(SELECT)\tRestart\n(file name used: %s)\n\n", card->generateFileName().c_str());
		}
		
		else{
			once = true;
			delete card;
			card = NULL;
			printf("\x1B[31mPlease insert a valid NDS game card!\x1B[0m\n");
		}
	}
	catch(Error const& e){
		delete card;
		card = NULL;
		printf("\x1B[31mAn error occured: error code %lx (%s, line %d)\x1B[0m\n", e.getErrorCode(), e.getFileName(), e.getLine());
	}
	catch(std::exception const& e){
		delete card;
		card = NULL;
		printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());
	}
	
	
	std::string fileName = card->generateFileName();
	FILE* f = NULL;
	
	while(aptMainLoop()) {
		hidScanInput();
		auto keys = hidKeysDown(); 
		
		if(!once && (keys & (KEY_L | KEY_R | KEY_X))) {
			printf("\n");
			if(keys & KEY_L){
				try{
					f = fopen(fileName.c_str(), "rb");
					if(f != NULL){
						fclose(f);
						printf("\x1B[33mFile %s already exists.\nOverwrite (A = yes, B = no)?\x1B[0m\n", fileName.c_str());
						while(aptMainLoop()){
							hidScanInput();
							if(hidKeysDown() & KEY_A) break;
							else if(hidKeysDown() & KEY_B) goto end;
						}
					}
					
					printf("Reading save file...\n\n");
					card->backupSaveFile(fileName, &updateProgressBar);
					printf("\n");
				}
				catch(Error const& e){
					printf("\x1B[31mAn error occured: error code %lx (%s, line %d)\x1B[0m\n", e.getErrorCode(), e.getFileName(), e.getLine());
					fclose(f);
				}
				catch(std::exception const& e){
					printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());
					fclose(f);
				}
			}
			
			
			else if(keys & KEY_R){
				try{
					f = fopen(fileName.c_str(), "rb");
					if(f == NULL){
						printf("\x1B[31mError: cannot open file %s\x1B[0m\n", fileName.c_str());
						goto end;
					}
					fseek(f, 0, SEEK_END);
					u32 sz = ftell(f);
					fclose(f);
					
					if(sz != card->saveSize()){
						printf("\x1B[31mError: incorrect file size\x1B[0m\n");
					}
					
					printf("Writing save file...\n\n");					
					card->restoreSaveFile(fileName, &updateProgressBar);
					printf("\n");
				}
				catch(Error const& e){
					printf("\x1B[31mAn error occured: error code %lx (%s, line %d)\x1B[0m\n", e.getErrorCode(), e.getFileName(), e.getLine());
					fclose(f);
				}
				catch(std::exception const& e){
					printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());
					fclose(f);
				}
			}
			
			else if(keys & KEY_X){
				try{
					printf("\x1B[33mAre you REALLY sure you want to erase your save data? (A = yes, B = no)?\x1B[0m\n", fileName.c_str());
					while(aptMainLoop()){
						hidScanInput();
						if(hidKeysDown() & KEY_A) break;
						else if(hidKeysDown() & KEY_B) goto end;
					}
					printf("Erasing save data...\n\n");					
					card->eraseSaveData(&updateProgressBar);
					printf("\n");
				}
				catch(Error const& e){
					printf("\x1B[31mAn error occured: error code %lx (%s, line %d)\x1B[0m\n", e.getErrorCode(), e.getFileName(), e.getLine());
					fclose(f);
				}
				catch(std::exception const& e){
					printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());
					fclose(f);
				}
			}
			
			
		end: 
			once = true;
			printf("\nDone.\nPress B to exit, SELECT to restart.\n");
		}
		
		if(keys & KEY_B) break;
		else if(keys & KEY_SELECT){
			gfxFlushBuffers();
			gfxSwapBuffers();
			gspWaitForVBlank();
			delete card; card = NULL;
			goto restart;
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	delete card;
	return 0;
}
