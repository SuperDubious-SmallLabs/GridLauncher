#include "addmenuentry.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

extern "C" {
    #include "filesystem.h"
    #include "shortcut.h"
}

bool comparisonFunc(const char *c1, const char *c2) {
    char * d1 = new char[strlen(c1)+1];
    char * d2 = new char[strlen(c2)+1];

    strcpy(d1, c1);
    strcpy(d2, c2);

    char *p;

    for (p = d1; *p != '\0'; ++p) {
        *p = tolower(*p);
    }

    for (p = d2; *p != '\0'; ++p) {
        *p = tolower(*p);
    }

    return strcmp(d1, d2) < 0;
}

void addMenuEntries(char paths[1024][1024], int totalEntries, int pathLength, menu_s* m, bool sortAlpha) {
    std::vector<char*>vPaths;

    int i;
    for (i=0; i<totalEntries; i++) {
        char *path = paths[i];
        vPaths.push_back(path);
    }

    if (sortAlpha) {
        std::sort(vPaths.begin(), vPaths.end(), comparisonFunc);
    }

    unsigned int j;

    for (j=0; j < vPaths.size(); j++) {
        char * path = vPaths.at(j);

        int n = strlen(path);

        //File is a .3dsx
        if(n>5 && !strcmp(".3dsx", &path[n-5])){
            addExecutableToMenu(m, path);
        }

        //File is a shortcut
        else if(n>4 && !strcmp(".xml", &path[n-4])){
            addShortcutToMenu(m, path);
        }

        //File is a directory
        else {
            addDirectoryToMenu(m, path);
        }
    }
}
