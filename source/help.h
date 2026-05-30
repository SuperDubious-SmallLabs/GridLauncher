#pragma once
#include <3ds.h>

extern char * foldersHelpTitle;
extern char * foldersHelpBody;

extern char * themesHelpTitle;
extern char * themesHelpBody;

extern bool showingHelpDetails;

void showHelp();
void drawHelp();
void updateHelp();
void handleHelpMenuSelection();
void handleHelpBackButton();
void showHelpWithForcedText(char * title, char * body);
