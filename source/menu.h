#ifndef __MENU_H__
#define __MENU_H__

#include <stdarg.h>
#include <stdio.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "types.h"
#include "ws.h"

// Text font (I hate libnds')
#include "font_bin.h"

// Clear
#define textClear() memset((u16*)BG_MAP_RAM_SUB(31), 0, 32*32*2)
#define strcasestr(x, y) (strcasecmp(&(x)[strlen(x) - strlen(y)], (y)))

typedef struct {
	char fName[MAXPATHLEN];
	bool isDirectory;
	bool isRunable;
} FileEntry, *pFileEntry;

void printText(int col, int x, int y, const char* fmt, ...);
void fillList();
void printList(u32 startPos);
void handleFileMenu();
void keyIrq();
void handleInGameMenu();

#endif
