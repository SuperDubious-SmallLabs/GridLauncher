#ifndef SORTING_H
#define SORTING_H

#include <3ds.h>
#include "menu.h"

#ifdef __cplusplus
extern "C" {
#endif

    void addMenuEntries(char paths[1024][1024], int totalEntries, int pathLength, menu_s* m, bool sortAlpha);

#ifdef __cplusplus
}
#endif

#endif
