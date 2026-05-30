#ifndef ALERT_H
#define ALERT_H

#include <3ds.h>
#include "gfx.h"

#define alertButtonNone -1
#define alertButtonKeyB -2

extern int alertSelectedButton;

int drawAlert(char * title, char * body, u8 * image, int numButtons, char buttonTitles[3][32]);

#endif
