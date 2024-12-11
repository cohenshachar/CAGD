#include <cagd.h>
#include <stdio.h>
#include <windows.h>
#include "resource.h"
#include "bsplines.h"
#include "surfaces.h"
#include "fernet.h"
#include <math.h>
#include <time.h>
#include "EnumHell.h"

void freeAllSingletons() {
	freeBsplineInstance();
	freeParamCurveInstance();
	freeSurfaceManager();
}
void myCommandAux(int id, int unUsed, PVOID userData) {
	HMENU* hMenuArray = (HMENU*)userData;
	if (id >= FERNET_CMD_START && id < FERNET_CMD_END){
		fernetControlCenter(id, unUsed, (PVOID)hMenuArray[1]);
	}
	else if (id >= BSPLINE_CMD_START && id < BSPLINE_CMD_END){
		bsplineCommandCenter(id, unUsed, (PVOID)hMenuArray[0]);
	}
	else if (id >= SURFACES_CMD_START && id < SURFACES_CMD_END){
		sufaceControlCenter(id, unUsed, (PVOID)hMenuArray[2]);
	}
}

int main(int argc, char *argv[])
{
	cagdFreeAllSegments();
	HMENU hMenu[4];
	cagdBegin("CAGD", 512, 512);
	hMenu[0] = CreatePopupMenu();
	addBsplinesMenus(hMenu[0]);
	hMenu[1] = CreatePopupMenu();
	addFernMenu(hMenu[1]);
	hMenu[2] = CreatePopupMenu();
	addSurfaceMenu(hMenu[2]);
	cagdRegisterCallback(CAGD_MENU, myCommandAux, (PVOID)hMenu);
	cagdMainLoop();
	freeAllSingletons();
	cagdFreeAllSegments();
	return 0;
}