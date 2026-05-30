#pragma once
#include <3ds.h>
//#include "button.h"
#include "menu.h"

extern menu_s foldersMenu;
extern bool show3DSFolder;

//buttonList folderButtons;

//void folders3DSToggled();

char * currentFolder();
char * currentFolderName();

void buildFoldersList();
void setFolder(char * folderName);