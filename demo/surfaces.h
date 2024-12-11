#include <cagd.h>
#include <stdio.h>
#include <windows.h>
#include "resource.h"
#include "bsplines.h"
#include "fernet.h"
#include <math.h>
#include <time.h>
#include "EnumHell.h"

typedef struct Surface Surface;


//void loadSurface(int path, int unUsed, PVOID userData);
Surface* createSurface(struct Curve* c1, struct Curve* c2);
void sufaceControlCenter(int id, int indx, PVOID userData);
void addSurfaceMenu(HMENU hMenu);
void freeSurfaceManager();