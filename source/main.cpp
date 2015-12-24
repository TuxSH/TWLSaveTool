/*
 *  TWLSaveTool is a Nintendo DS save backup tool for the Nintendo 3DS.
 *  Copyright (C) 2015 TuxSH
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
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

extern "C"{
#include <sys/stat.h>
#include <unistd.h>
}

#include "TWLCard.h"



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
		gfxInit(GSP_RGB565_OES, GSP_RGB565_OES, false);
		hidInit();
		fsInit();
		sdmcInit();
	}

	void __appExit()
	{
		// Exit services
		sdmcExit();
		fsExit();
		hidExit();
		gfxExit();
		aptExit();
		srvExit();
	}
}


using namespace TWLCard;

int main()
{
	mkdir("sdmc:/TWLSaveTool", 0777);
	chdir("sdmc:/TWLSaveTool");
	bool once = false;
	u64 saveSz = 0;
	Header h;
	
	consoleInit(GFX_TOP, NULL);
	printf("TWLSaveTool 0.1a by TuxSH\n\n\n");
	
	try {
		bool isTWL = isCardTWL();
		if(isTWL) {
			h = getHeader();
			saveSz = getSaveFileSize();
			printf("Game title: %s\nGame code: %s\nMaker code: %s\n", h.gameTitle.c_str(), h.gameCode.c_str(), h.makerCode.c_str());
			
			if(saveSz < 1024) printf("Save file size: %llu B\n", saveSz);
			else printf("Save file size: %llu KB\n", saveSz >> 10);
			
			printf("(A) Read save file\n(Y) Write save file\n(HOME) Exit\n(file name used: %s.sav)\n\n", h.gameTitle.c_str());
		}
		
		else{
			once = true;
			printf("Please insert a valid NDS game card!\n");
		}
	}
	catch(Error const& e){
		printf("An error occured: error code %x\n", e.getErrorCode());
	}
	catch(std::exception const& e){
		printf("An error occured: %s\n", e.what());
	}

	
	std::string fileName = h.gameTitle + ".sav";
	u8* data = new u8[(size_t) saveSz];
	FILE* f = NULL;
	
	while(aptMainLoop()) {
		hidScanInput();

		if(!once) {
			if(hidKeysDown() & KEY_A){
				
				try{
					f = fopen(fileName.c_str(), "rb");
					if(f != NULL){
						fclose(f);
						printf("Error: file %s already exists\n", fileName.c_str());
						goto end;
					}
					
					f = fopen(fileName.c_str(), "wb+");
					if(f == NULL){
						printf("Error: cannot create file %s\n", fileName.c_str());
						goto end;
					} 
					printf("Please wait...\n");
					readSaveFile(data, saveSz);
					fwrite(data, 1, saveSz, f);
					fclose(f);
				}
				catch(Error const& e){
					printf("An error occured: error code %x\n", e.getErrorCode());
					fclose(f);
				}
				catch(std::exception const& e){
					printf("An error occured: %s\n", e.what());
					fclose(f);
				}
			}
			
			
			else if(hidKeysDown() & KEY_Y){

				try{
					f = fopen(fileName.c_str(), "rb");
					if(f == NULL){
						printf("Error: cannot open file %s\n", fileName.c_str());
						goto end;
					}
					fseek(f, 0, SEEK_END);
					u64 sz = ftell(f);
					rewind(f);
					
					if(sz != saveSz){
						printf("Error: incorrect file size\n");
					}
					
					printf("Please wait...\n");					
					fread(data, 1, saveSz, f);

					writeToSaveFile(data, saveSz);
					fclose(f);
				}
				catch(Error const& e){
					printf("An error occured: error code %x\n", e.getErrorCode());
					fclose(f);
				}
				catch(std::exception const& e){
					printf("An error occured: %s\n", e.what());
					fclose(f);
				}
			}
		end: 
			once = true;
			printf("Done.\n");
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	delete[] data;
	return 0;
}
