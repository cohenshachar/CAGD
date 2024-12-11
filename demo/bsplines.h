#include <cagd.h>
#include <stdio.h>
#include <windows.h>
#include "resource.h"
#include "Control_Point.h"
#include <math.h>
#include <time.h>
#include "EnumHell.h"



typedef struct Curve Curve;


void bsplineCommandCenter(int id, int unUsed, PVOID userData);
void addBsplinesMenus(HMENU hMenu);
void freeBsplineInstance();

void get_domain(Kn_List* list, int K, double domain[2]);

void connectCurvesLeftUp(int x, int y, PVOID userData);

Control_Point** bspline_get_cps(Curve* c);
int bspline_get_cps_len(Curve* c);
Kn_List* bspline_get_knots(Curve* c);
int bspline_get_order(Curve* c);

int evaluateJ(Kn_List* kn_list, double t);
Control_Point** evaluateBsplineInvolvedCPs(const Curve* c, double t);
Control_Point** evaluateInvolvedCPs(Control_Point** cp_arr, int J, int K);
double* evaluateBsplineBaseFunctions(Kn_List* kn_list, double t, int J, int K, int layer);
CAGD_POINT evaluateAtT(Control_Point** control_points, Kn_List* knots, int K, double t, BOOL rational, double* optional_norm);
TimeStampedPolyGroup plot_bspline_function(Control_Point** control_points, int cp_len, Kn_List* knots, int K, int fin, BOOL rational);