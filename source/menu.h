#ifndef __MENU_H__
#define __MENU_H__

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "ws.h"
#include "fat/gba_nds_fat.h"
#include "fat/disc_io.h"

	// Text font (I hate libnds')
#include "font_bin.h"

	// Clear
#define textClear() memset((u16*)BG_MAP_RAM_SUB(31), 0, 32*32*2)
#define strcasestr(x, y) (strcasecmp(&(x)[strlen(x) - strlen(y)], (y)))

typedef struct
{
	char fName[256];
	FILE_TYPE fType;
	bool isRunable;

} FileEntry, *pFileEntry;

void printText(int col, int x, int y, const char* fmt, ...);
void fillList();
void printList(u32 startPos);
void handleFileMenu();
void keyIrq();
void handleInGameMenu();

#endif
