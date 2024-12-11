#pragma once

#include <cagd.h>
#include <stdio.h>
#include <windows.h>
#include "resource.h"
#include <math.h>
#include <time.h>
#include "EnumHell.h"

typedef enum{
	FRN_CLICK = FERNET_CMD_START,
	FRN_AXS,
	FRN_CRV,
	FRN_TORS,
	FRN_SPHE,
	FRN_ANI,
	FRN_OFFSET,
	FRN_OFPAR,
	FRN_EVOL,
	FRN_X,
	FRN_Y,
	FRN_Z,
	FRN_T,
	FRN_TR,
	FRN_R,
	FRN_CNL,
	FRN_FIN,
	FRN_LOAD
} fernetCommands;



void fernetControlCenter(int id, int indx, PVOID userData);
void addFernMenu(HMENU hMenu);
void freeParamCurveInstance();

void drawCircle(CAGD_POINT circle_center, GLdouble radius, CAGD_POINT fern[3], UINT* lines);
void drawCoil(CAGD_POINT coil_center, GLdouble radius, CAGD_POINT fern[3], double torsion, UINT* lines);


void getFernetData(CAGD_POINT dF, CAGD_POINT ddF, CAGD_POINT dddF, CAGD_POINT fern[3], double* curvi, double* tors);
