#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdbool.h>
#include <switch.h>

// ATMOSPHERE CODE -----------------------------------------------------------------

#define IRAM_PAYLOAD_MAX_SIZE 0x2F000
#define IRAM_PAYLOAD_BASE 0x40010000
static alignas(0x1000) u8 g_reboot_payload[IRAM_PAYLOAD_MAX_SIZE];
static alignas(0x1000) u8 g_ff_page[0x1000];
static alignas(0x1000) u8 g_work_page[0x1000];

void do_iram_dram_copy(void *buf, uintptr_t iram_addr, size_t size, int option) {
    memcpy(g_work_page, buf, size);
    
    SecmonArgs args = {0};
    args.X[0] = 0xF0000201;             /* smcAmsIramCopy */
    args.X[1] = (uintptr_t)g_work_page;  /* DRAM Address */
    args.X[2] = iram_addr;              /* IRAM Address */
    args.X[3] = size;                   /* Copy size */
    args.X[4] = option;                 /* 0 = Read, 1 = Write */
    svcCallSecureMonitor(&args);
    
    memcpy(buf, g_work_page, size);
}

void copy_to_iram(uintptr_t iram_addr, void *buf, size_t size) {
    do_iram_dram_copy(buf, iram_addr, size, 1);
}

void copy_from_iram(void *buf, uintptr_t iram_addr, size_t size) {
    do_iram_dram_copy(buf, iram_addr, size, 0);
}

static void clear_iram(void) {
    memset(g_ff_page, 0xFF, sizeof(g_ff_page));
    for (size_t i = 0; i < IRAM_PAYLOAD_MAX_SIZE; i += sizeof(g_ff_page)) {
        copy_to_iram(IRAM_PAYLOAD_BASE + i, g_ff_page, sizeof(g_ff_page));
    }
}

static void reboot_to_payload(void) {
    clear_iram();
    
    for (size_t i = 0; i < IRAM_PAYLOAD_MAX_SIZE; i += 0x1000) {
        copy_to_iram(IRAM_PAYLOAD_BASE + i, &g_reboot_payload[i], 0x1000);
    }
    
    splSetConfig((SplConfigItem)65001, 2);
}
// END ATMOSPHERE CODE -----------------------------------------------------------------

bool majorError = false, cursorchange = false, readytoboot = false, invalidinput = false, nopayloads = true;
char *list[6], *paylist[30], *location = {'\0'};
int cursor = 4, i = 0, j = 0, temp = 1;
Mix_Chunk *button = {NULL}; Mix_Chunk *delete = {NULL};

void refreshscreen() {
    consoleInit(NULL);
    printf("\x1b[40m\x1b[37;1HLoad a payload by pressing (A) or one of the assigned hotkeys.\n\n\
Hotkey a payload by holding (X) and pressing either (L) / (R) / (ZL) / (ZR).\n\n\
Press (Y) for options to add payload as reboot_payload for cfw or modchips.\n\n\
Press (Right-Stick) or (Left-Stick) to clear ALL saved hotkeys.\n\n\
Quit (+) or (-)");
    printf("\x1b[32m\x1b[1;1HPayLoaderNX: v1.0.0 ^v^\n\x1b[33mCurrent dir: %s\x1b[39m\n-----------------------------------\n", location);
	cursorchange = true;
}

void reboottopayload(const char *payloc) {
		readytoboot = true;
        Result rc = splInitialize();
        if (R_FAILED(rc)) {
            readytoboot = false;
            printf("\nFailed to init spl!: 0x%x\n", rc);
        }
           		
        FILE *p = fopen(payloc, "rb");
        if (p == NULL) { 
			readytoboot = false;
            refreshscreen();
			printf("\x1b[41m\x1b[35;1HFailed to open %s\x1b[40m\n", payloc);
        }
        else {
			fread(g_reboot_payload, 1, sizeof(g_reboot_payload), p);
			fclose(p);
        } 

        if (readytoboot == true) reboot_to_payload();
        splExit(); // if the function above fails this will be run
}

char* keyboard(char* message, size_t size) {
	SwkbdConfig	skp; 
	Result rc = swkbdCreate(&skp, 0);
	char* out = NULL;
	out = (char *)calloc(sizeof(char), size + 1);
	if (out == NULL) out = NULL;
	else if (R_SUCCEEDED(rc)) {
		swkbdConfigMakePresetDefault(&skp);
		swkbdConfigSetGuideText(&skp, message);
	    rc = swkbdShow(&skp, out, size);
		swkbdClose(&skp);	
	}
	else {
        free(out); 
        out = NULL;
    }
	return (out);
}

char* addstrings(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1); 
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void copy(const char* src, const char* dst) {
    FILE* in = fopen(src, "rb");
    FILE* out = fopen(dst, "wb");

    if (in == NULL || out == NULL) printf("\nAn error occured...");
    else {
        size_t len = 0;
        char buffer[BUFSIZ];
        while ((len = fread(buffer, 1, BUFSIZ, in)) > 0)
            fwrite(buffer, 1, len, out);
    }
    if (in) fclose(in);
    if (out) fclose(out);
}

void checkforfolder(char* foldloc) {
	DIR *tr = opendir(addstrings(foldloc, "."));
	if (tr != NULL) {list[temp] = foldloc; temp++;}
    closedir(tr);
}

bool checkfolder(char* foldloc) {
	DIR *tr = opendir(addstrings(foldloc, "."));
	bool rtn = false;
	if (tr != NULL) rtn = true;
    closedir(tr);
    return rtn;
}

void intMusic()
{
    Result romfs = romfsInit(); // check if romfs is there (should always work).
    if (!R_FAILED(romfs)) chdir("romfs:/"); // change directory, needed to load the sound files

    Mix_AllocateChannels(2);
    Mix_OpenAudio(48000, AUDIO_S16, 2, 4096);

    button = Mix_LoadWAV("sound/button.wav");
    delete = Mix_LoadWAV("sound/delete.wav");
    Mix_PlayChannel(-1, Mix_LoadWAV("sound/click.wav"), 0);

    chdir("sdmc:/"); // change back to the root sd card.

    /*if (fopen("/switch/payloadernx/music.ogg", "rb"))
        Mix_PlayMusic(Mix_LoadMUS("/switch/payloadernx/music.ogg"), -1); //my little easter egg...*/
}

void listdir()
{
	consoleInit(NULL);
	majorError = false;

    FILE* file = fopen("/switch/payloadernx/config.ini", "rb");
    if (file == NULL) {
    	temp = 1;
    	if (invalidinput == true) {
    		printf("\x1b[41mError: Path does not exist!\x1b[40m\n");
    		//invalidinput = false;
    	}

    	printf("\x1b[32mSelect payload path:\x1b[37m\n--------------------\x1b[43;1HSelect (A)\n\nCancel (B)");
    	checkforfolder("/payloads/");
        checkforfolder("/argon/payloads/");
        checkforfolder("/bootloader/payloads/");
		checkforfolder("/switch/payloadernx/payloads/");
    	checkforfolder("/");
    	list[temp] = "Set custom payload path...";
    	temp++;
    	cursorchange = true;
    	cursor = 5;

		while(1) {
    		hidScanInput();
      		u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
			consoleUpdate(NULL);

			if (kDown & KEY_A) {
                Mix_PlayChannel(-1, button, 0); location = list[cursor - 4];
                break;
            }
        	if (kDown & KEY_B && nopayloads == false && invalidinput== false) {
                Mix_PlayChannel(-1, button, 0); 
                break;
            }

        	if (kDown & KEY_LSTICK_DOWN || kDown & KEY_RSTICK_DOWN ||kDown & KEY_DDOWN) cursor = cursor + 1, cursorchange = true;
        	if (kDown & KEY_LSTICK_UP || kDown & KEY_RSTICK_UP || kDown & KEY_DUP) cursor = cursor - 1, cursorchange = true;
        	if (cursor <= 4) cursor = 5, cursorchange = false;
        	if (cursor >= temp + 4) cursor = temp + 4 - 1, cursorchange = false;  

    		if (cursorchange == true) {
        	    cursorchange = false;
        	    printf("\x1b[4;1H");
        	    for (i = 0; temp - 1 >= i; i++) {
        	        if (list[i] != NULL) {
        	            if (cursor == i + 4) printf("\x1b[42m%s\n", list[i]);
        	            else printf("\x1b[40m%s\n", list[i]);
        	        }
        	    }
        	}
		}
    	if (strcmp(location, "Set custom payload path...") == 0) location = keyboard("Write your custom path here. Don't forget to add the / at the start and the end of the path!", 150);
		
		if (checkfolder(location) == false) { // if folder location does not exist.
    		invalidinput = true;
    		listdir(); // calls listdir again...
    	}

    	file = fopen ("/switch/payloadernx/config.ini", "w");
    	fputs(location, file); // writes the location of the directory in location ini.
	}
	else { //TODO: create payload entries in seperate config ini for each payload. Maybe make a function. Then after, have 1 ini for payloads and seek entries for each path.
		long length = 0;
		fseek (file, 0L, SEEK_END);
		length = ftell (file);
		fseek (file, 0, SEEK_SET);
		location = calloc(1, length+1);
		if (location) fread (location, 1, length, file);
	}

	fclose(file);
	refreshscreen();
	paylist[0] = "";

	struct dirent *de;
	DIR *dr = opendir(addstrings(location, "."));

	i = 0;
	if (majorError == false) {
		while ((de = readdir(dr)) != NULL && i != 30) {
			if (strstr(de->d_name, ".bin") != NULL) {
				size_t size = strlen(de->d_name) + 1;
				paylist[i] = (char*) malloc (size);
				strlcpy(paylist[i], de->d_name, size);
				if (paylist[i] != NULL) printf("%s\x1b[40m\n", paylist[i]);
				i = i + 1;
			}
		}
	}

    if (strcmp(paylist[0], "") == 0) {
		consoleClear();
		majorError = true;
        nopayloads = true;
		printf("\x1b[40m\nError: Folder does not contain payloads!\n\n\nPress (B) to go back and select another payload location\n\nPress (+) or (-) to exit...");
	}
    else {
        nopayloads = false;
        invalidinput = false;
    }
	cursor = 4;
	closedir(dr); //close for cleanup. Very important!!!
}

int main(int argc, char** argv) {

    void *keyheap;
    if (svcSetHeapSize(&keyheap, 0x10000000) == (Result)-1) fatalSimple(0); //Heap for keyboard

    intMusic();
    mkdir("/switch/payloadernx", 0777);
    mkdir("/switch/payloadernx/hotkeys", 0777);
    mkdir("/switch/payloadernx/payloads", 0777);
	listdir();

	while (appletMainLoop()) {

        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
		u64 kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);

        if (kDown & KEY_PLUS || kDown & KEY_MINUS) break;	
        if (kDown & KEY_B) {
            Mix_PlayChannel(-1, button, 0);
            remove("/switch/payloadernx/config.ini"); 
            listdir();
        }

        if (majorError == false) {
        	if (kDown & KEY_LSTICK_DOWN || kDown & KEY_RSTICK_DOWN || kDown & KEY_DDOWN) cursor = cursor + 1, cursorchange = true;
        	if (kDown & KEY_LSTICK_UP || kDown & KEY_RSTICK_UP || kDown & KEY_DUP) cursor = cursor - 1, cursorchange = true;
        	if (cursor <= 3) cursor = 4, cursorchange = false;
        	if (cursor >= i + 4) cursor = i + 4 - 1, cursorchange = false;  

        	if (cursorchange == true) {
        	    cursorchange = false;
        	    printf("\x1b[4;1H");
        	    for (int j = 0; i - 1 >= j; j++) {
        	        if (paylist[j] != NULL) {
        	            if (cursor == j + 4) printf("\x1b[42m%s\x1b[40m\n", paylist[j]); 
        	            else printf("\x1b[40m%s\n", paylist[j]); 
        	        } 
        	    } 
        	}

			// Hotkey button configs
			if (kHeld & KEY_X && kDown & KEY_L) {
                Mix_PlayChannel(-1, button, 0);
				copy(addstrings(location, paylist[cursor - 4]), "sdmc:/switch/payloadernx/hotkeys/L.hotkey");
				refreshscreen();
				printf("\x1b[46m\x1b[35;1H%s is now mapped to button \"L\"!", paylist[cursor - 4]);
			}

			if (kHeld & KEY_X && kDown & KEY_R) {
                Mix_PlayChannel(-1, button, 0);
				copy(addstrings(location, paylist[cursor - 4]), "sdmc:/switch/payloadernx/hotkeys/R.hotkey");
				refreshscreen();
				printf("\x1b[46m\x1b[35;1H%s is now mapped to button \"R\"!", paylist[cursor - 4]);
			}

			if (kHeld & KEY_X && kDown & KEY_ZL) {
                Mix_PlayChannel(-1, button, 0);
				copy(addstrings(location, paylist[cursor - 4]), "sdmc:/switch/payloadernx/hotkeys/ZL.hotkey");
				refreshscreen();
				printf("\x1b[46m\x1b[35;1H%s is now mapped to button \"ZL\"!", paylist[cursor - 4]);
			}

			if (kHeld & KEY_X && kDown & KEY_ZR) {
                Mix_PlayChannel(-1, button, 0);
				copy(addstrings(location, paylist[cursor - 4]), "sdmc:/switch/payloadernx/hotkeys/ZR.hotkey");
				refreshscreen();
				printf("\x1b[46m\x1b[35;1H%s is now mapped to button \"ZR\"!", paylist[cursor - 4]);
			} // END OF HOTKEYS

        	if (kDown & KEY_Y) {
                Mix_PlayChannel(-1, button, 0);
				consoleClear();
        	    printf("\x1b[32m\x1b[4m\x1b[3;1HSelected payload is ->\x1b[24m\x1b[37m %s\x1b[6;1HPress (A) to save payload as reboot_payload.bin for Atmosphere / Kosmos\n\nPress (X) to save payload as payload.bin for use with sxos and modchips\n\nPress (B) to go back...", paylist[cursor - 4]);
				consoleUpdate(NULL);
				while(1) {
					hidScanInput();
					u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
					if (kDown & KEY_A){
                        Mix_PlayChannel(-1, button, 0);
						copy(addstrings(location, paylist[cursor - 4]), "/atmosphere/reboot_payload.bin");
						refreshscreen();
						printf("\x1b[46m\x1b[34;1H%s added as Atmosphere reboot payload!\nNote: will only take effect after a reboot.", paylist[cursor - 4]);
						break;
					}
					/*if (kDown & KEY_Y){ //TODO: Ready for reinx implimentation.
                        Mix_PlayChannel(-1, button, 0);
						copy(addstrings(location, paylist[cursor - 4]), "/ReiNX.bin");
						refreshscreen();
						printf("\x1b[46m\x1b[34;1H%s added as ReiNX reboot payload!\nNote: will only take effect after a reboot.", paylist[cursor - 4]);
						break;
					}*/
					if (kDown & KEY_X) {
                        Mix_PlayChannel(-1, button, 0);
						copy(addstrings(location, paylist[cursor - 4]), "/payload.bin");
						refreshscreen();
						printf("\x1b[46m\x1b[34;1H%s is now saved as \"payload.bin\" on the root of your sd card!\nNote: will only take effect after a reboot.", paylist[cursor - 4]);
						break;
					}
					if (kDown & KEY_B) {
                        refreshscreen(); 
                        Mix_PlayChannel(-1, button, 0); 
                        break;
                    }
				}
			}

			if (kDown & KEY_LSTICK || kDown & KEY_RSTICK || kHeld & KEY_LSTICK || kHeld & KEY_RSTICK) { // Deletes hotkeys
                Mix_PlayChannel(-1, button, 0);
                refreshscreen();
        	    printf("\x1b[41m\x1b[5;1HDelete all hotkeys?\x1b[40m\n\nPress (A) to continue\n\nPress (B) to go back...\n");
				consoleUpdate(NULL);
				while(1) {
					hidScanInput();
					u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
					if (kDown & KEY_A) {
                        Mix_PlayChannel(-1, delete, 0);
						refreshscreen();
						remove("/switch/payloadernx/hotkeys/L.hotkey");
						remove("/switch/payloadernx/hotkeys/R.hotkey");
						remove("/switch/payloadernx/hotkeys/ZL.hotkey");
						remove("/switch/payloadernx/hotkeys/ZR.hotkey");
						printf("\x1b[46m\x1b[35;1HCleared all hotkeys!");
						break;
					}
					if (kDown & KEY_B) {
                        Mix_PlayChannel(-1, button, 0); 
                        break;
                    }
				}
                refreshscreen();
			}

        	if (kDown & KEY_A) { //Launch a payload with (A)
                Mix_PlayChannel(-1, button, 0);
				refreshscreen();
        	    printf("\x1b[5;1HAre you sure you want to launch %s?\n\nPress (A) to launch payload\n\nPress (B) to go back...", paylist[cursor - 4]);
				consoleUpdate(NULL);
				while(1) {
					hidScanInput();
					u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
					if (kDown & KEY_A) reboottopayload(addstrings(location, paylist[cursor - 4]));
					if (kDown & KEY_B) break;
				}
                Mix_PlayChannel(-1, button, 0);
        	    refreshscreen();
			}

			// Hotkey loading
        	if (kDown == KEY_L && kHeld == KEY_L) reboottopayload("/switch/payloadernx/hotkeys/L.hotkey");
			if (kDown == KEY_R && kHeld == KEY_R) reboottopayload("/switch/payloadernx/hotkeys/R.hotkey");
			if (kDown == KEY_ZL && kHeld == KEY_ZL) reboottopayload("/switch/payloadernx/hotkeys/ZL.hotkey");
			if (kDown == KEY_ZR && kHeld == KEY_ZR) reboottopayload("/switch/payloadernx/hotkeys/ZR.hotkey");
     	}
        consoleUpdate(NULL);
    }

    //cleanup then exit.
	Mix_HaltChannel(-1); //<-free
    Mix_FreeChunk(button); Mix_FreeChunk(delete);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
    romfsExit(); //->music

    if (majorError == false) for (i = 0; i < 30; i++) free(paylist[i]); //free payload list
    svcSetHeapSize(&keyheap, ((u8*) envGetHeapOverrideAddr() + envGetHeapOverrideSize()) - (u8*) keyheap); //credit to WerWolv!
    consoleExit(NULL);
    return 0;
}