#include <cstdio>
#include <cstring>

extern "C"{
#include <sys/stat.h>
#include <unistd.h>
}

#include "TWLCard.h"
#include "games.h"
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
		consoleInit(GFX_TOP, NULL);
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

void updateProgressBar(u64 offset, u64 total){
	const int nbBars = 40;
	std::string bar(nbBars*offset/total, '#');
	
	bar += std::string(nbBars - nbBars*offset/total, '-');
	
	PrintConsole* csl = consoleGetDefault(); 
	std::string centerPadding((49 - (nbBars + 5 + csl->tabSize))/2, ' ');
	
	printf("\r%s[%s]\t%2d%%", centerPadding.c_str(), bar.c_str(), (int)(100*offset/total));
	fflush(stdout);
}

std::string sizeToStr(u64 sz){
	char buf[50];
	if(sz < 1024)
		sprintf(buf, "%llu B", sz);
	else if(sz < (1 << 20))
		sprintf(buf, "%llu KB", sz >> 10);
	else sprintf(buf, "%llu MB", sz >> 20);
	
	return std::string(buf);
}

u64 selectSaveFileSize(u64 initialValue){
	int pos = std::distance(validSizes, std::find(validSizes, validSizes + 7, initialValue));
	std::string filler(49, ' ');
	
	while(aptMainLoop()){
		printf("\rSave file size (L/R = -/+, A = confirm): %s", sizeToStr(validSizes[pos]).c_str());
		fflush(stdout);
		hidScanInput();
		
		if(hidKeysDown() & (KEY_L | KEY_R)){
			printf("\r%s", filler.c_str()); // clears the current line
			fflush(stdout);
			
			if(hidKeysDown() & KEY_L) pos = (7 + pos - 1)%7; // because of how modulus works in C ...
			else if(hidKeysDown() & KEY_R) pos = (pos + 1)%7;
		}

		else if(hidKeysDown() & KEY_A) break;
	}
	printf("\n");
	return validSizes[pos];
}

int main()
{
restart:

	mkdir("sdmc:/TWLSaveTool", 0777);
	chdir("sdmc:/TWLSaveTool");
	
	bool once = false;
	u64 saveSz = 0;
	Header h;
	u8* data = NULL;
	
	consoleClear();
	printf("\x1b[1m\x1b[0;12HTWLSaveTool v0.1a by TuxSH\x1B[0m\n\n\n");
	
	try {
		bool isTWL = isCardTWL();
		if(isTWL) {
			h = getHeader();
			saveSz = getSaveFileSize();
			printf("Game title:\t%s\nCode name: %s\n", h.gameTitle.c_str(), h.generateCodeName().c_str());
			printf("Possible save file size:\t%s\n", sizeToStr(saveSz).c_str());
			//printf("SPI size:\t%s\n", sizeToStr(getSPISize()).c_str());
			printf("(A)\tRead save file\n(Y)\tWrite save file\n(B)\tExit\n(SELECT)\tRestart\n(file name used: %s.sav)\n\n", h.gameTitle.c_str());
		}
		
		else{
			once = true;
			printf("\x1B[31mPlease insert a valid NDS game card!\x1B[0m\n");
		}
	}
	catch(Error const& e){
		printf("\x1B[31mAn error occured: error code %x (%s, line %d)\x1B[0m\n", e.getErrorCode(), e.getFileName(), e.getLine());
	}
	catch(std::exception const& e){
		printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());
	}
	
	
	std::string fileName = h.gameTitle + ".sav";
	FILE* f = NULL;
	
	while(aptMainLoop()) {
		hidScanInput();
		auto keys = hidKeysDown(); 
		
		if(!once && (keys & (KEY_A | KEY_Y))) {
			saveSz = selectSaveFileSize(saveSz);

			data = new u8[(size_t) saveSz];
			printf("\n");
			if(keys & KEY_A){
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
					
					f = fopen(fileName.c_str(), "wb+");
					if(f == NULL){
						printf("\x1B[31mError: cannot create file %s\x1B[0m\n", fileName.c_str());
						goto end;
					} 
					printf("Reading save file...\n\n");
					readSaveFile(data, saveSz, updateProgressBar);
					printf("\n");
					fwrite(data, 1, saveSz, f);
					fclose(f);
				}
				catch(Error const& e){
					printf("\x1B[31mAn error occured: error code %x (%s, line %d)\x1B[0m\n", e.getErrorCode(), e.getFileName(), e.getLine());
					fclose(f);
				}
				catch(std::exception const& e){
					printf("\x1B[31mAn error occured: %s\x1B[0m\n", e.what());
					fclose(f);
				}
			}
			
			
			else if(keys & KEY_Y){
				try{
					f = fopen(fileName.c_str(), "rb");
					if(f == NULL){
						printf("\x1B[31mError: cannot open file %s\x1B[0m\n", fileName.c_str());
						goto end;
					}
					fseek(f, 0, SEEK_END);
					u64 sz = ftell(f);
					rewind(f);
					
					if(sz != saveSz){
						printf("\x1B[31mError: incorrect file size\x1B[0m\n");
					}
					
					printf("Writing save file...\n\n");					
					fread(data, 1, saveSz, f);

					writeToSaveFile(data, saveSz, updateProgressBar);
					printf("\n");
					fclose(f);
				}
				catch(Error const& e){
					printf("\x1B[31mAn error occured: error code %x (%s, line %d)\x1B[0m\n", e.getErrorCode(), e.getFileName(), e.getLine());
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
			delete[] data;
			goto restart;
		}

		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}
	
	delete[] data;
	return 0;
}
