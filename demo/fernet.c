#include "fernet.h"

static void setup(struct Parametric_Curve* p_curve);
void drawFernetShapes(struct Parametric_Curve* p_curve, double t, BOOL for_anime);
void drawCurve(struct Parametric_Curve* p_curve, char* which);
void delete_param_curve();
void init_param_curve(struct Parametric_Curve* p_curve);
void getFernetData(CAGD_POINT dF, CAGD_POINT ddF, CAGD_POINT dddF, CAGD_POINT fern[3], double* curvi, double* tors);

enum Parametric_Control {
	CLICK = 0,
	ANIM,
	OFSE,
	EVOL,
	TRDOM,
	AXIS,
	CURV,
	SPHE,
	COIL,
	LOADED
};

struct Parametric_Curve {
	e2t_expr_node* func[3];
	e2t_expr_node* dfunc[3];
	e2t_expr_node* ddfunc[3];
	e2t_expr_node* dddfunc[3];
	e2t_expr_node* func_tr;

	UINT mainCurve;
	UINT evolCurve;
	UINT offsetCurve;

	UINT frenet_axis[3];
	UINT frenet_shapes[3];

	UINT anime_frenet_axis[3];
	UINT anime_frenet_shapes[3];

	double T[2];
	double R[2];
	double* D;

	double* time_stamps;
	long long samples;

	BOOL use_Tr;
	BOOL draw_geo[4];
	BOOL animation;
	BOOL click;
	BOOL draw_offset; 
	BOOL draw_evol;
	long long anime_fernet;
	double offset;

	unsigned int bitmap;
};

static struct Parametric_Curve* Param_Curve = NULL;

struct Parametric_Curve* getParamCurveInstance() {
	if (Param_Curve == NULL) {
		Param_Curve = (struct Parametric_Curve*)malloc(sizeof(struct Parametric_Curve));
		if (Param_Curve != NULL) {
			init_param_curve(Param_Curve); // Initialize the value
		}
	}
	return Param_Curve;
}

void freeParamCurveInstance() {
	if (Param_Curve != NULL) {
		delete_param_curve(Param_Curve);
		Param_Curve = NULL;
	}
}

void delete_param_curve() {
	for (int i = 0; i < 3; i++) {
		if (Param_Curve->func[i]) e2t_freetree(Param_Curve->func[i]);
		if (Param_Curve->dfunc[i]) e2t_freetree(Param_Curve->dfunc[i]);
		if (Param_Curve->ddfunc[i]) e2t_freetree(Param_Curve->ddfunc[i]);
		if (Param_Curve->dddfunc[i]) e2t_freetree(Param_Curve->dddfunc[i]);
		cagdFreeSegment(Param_Curve->frenet_axis[i]);
		cagdFreeSegment(Param_Curve->frenet_shapes[i]);
		cagdFreeSegment(Param_Curve->anime_frenet_axis[i]);
		cagdFreeSegment(Param_Curve->anime_frenet_shapes[i]);
	}
	cagdFreeSegment(Param_Curve->evolCurve);
	cagdFreeSegment(Param_Curve->mainCurve);
	cagdFreeSegment(Param_Curve->offsetCurve);
	if (Param_Curve->func_tr) e2t_freetree(Param_Curve->func_tr);
}

void init_param_curve(struct Parametric_Curve* p_curve) {
	CAGD_POINT dummy[] = { {1, 1, 1}, {1, -1, 1} };
	for (int i = 0; i < 3; i++) {
		if (i == 0) cagdSetColor(0, 255, 125);
		if (i == 1) cagdSetColor(125, 0, 255);
		if (i == 2) cagdSetColor(255, 125, 0);
		cagdHideSegment(p_curve->frenet_axis[i] = cagdAddPolyline(dummy, 2));
		cagdHideSegment(p_curve->anime_frenet_axis[i] = cagdAddPolyline(dummy, 2));
		cagdHideSegment(p_curve->frenet_shapes[i] = cagdAddPolyline(dummy, 2));
		cagdHideSegment(p_curve->anime_frenet_shapes[i] = cagdAddPolyline(dummy, 2));
	}

	for (int i = 0; i < 3; i++) {
		p_curve->func[i] = NULL;
		p_curve->dfunc[i] = NULL;
		p_curve->ddfunc[i] = NULL;
		p_curve->dddfunc[i] = NULL;
	}
	p_curve->func_tr = NULL;
	p_curve->T[0] = p_curve->R[0] = 0;
	p_curve->T[1] = p_curve->R[1] = 1;
	p_curve->D = p_curve->T;
	cagdSetColor(0, 255, 255);
	cagdHideSegment(p_curve->mainCurve = cagdAddPolyline(dummy, 2));
	cagdSetColor(0, 255, 164);
	cagdHideSegment(p_curve->offsetCurve = cagdAddPolyline(dummy, 2));
	cagdSetColor(0, 164, 255);
	cagdHideSegment(p_curve->evolCurve = cagdAddPolyline(dummy, 2));

	p_curve->time_stamps = NULL;
	p_curve->samples = 0;

	p_curve->use_Tr = FALSE;
	p_curve->draw_geo[0] = p_curve->draw_geo[1] = p_curve->draw_geo[2] = p_curve->draw_geo[3] = FALSE;
	p_curve->animation = FALSE;
	p_curve->click = FALSE;
	p_curve->draw_offset = FALSE;
	p_curve->draw_evol = FALSE;
	p_curve->anime_fernet = 0;
	p_curve->offset = 1;
	p_curve->bitmap = 0;
}

void showSegments(struct Parametric_Curve* p_curve) {
	if (p_curve->draw_geo[0]) for (int i = 0; i < 3; i++){
		if (p_curve->click) cagdShowSegment(p_curve->frenet_axis[i]);
		if (p_curve->animation) cagdShowSegment(p_curve->anime_frenet_axis[i]);
	}
	for (int i = 0; i < 3; i++) if (p_curve->draw_geo[i+1]) {
		if (p_curve->click) cagdShowSegment(p_curve->frenet_shapes[i]);
		if (p_curve->animation) cagdShowSegment(p_curve->anime_frenet_shapes[i]);
	}
	if (p_curve->draw_offset) cagdShowSegment(p_curve->offsetCurve);
	if (p_curve->draw_evol) cagdShowSegment(p_curve->evolCurve);
}

void hideSegments(struct Parametric_Curve* p_curve) {
	if (p_curve->draw_geo[0]) for (int i = 0; i < 3; i++) {
		if (!p_curve->click) cagdHideSegment(p_curve->frenet_axis[i]);
		if (!p_curve->animation) cagdHideSegment(p_curve->anime_frenet_axis[i]);
	}
	for (int i = 0; i < 3; i++) if (p_curve->draw_geo[i+1]) {
		if (!p_curve->click) cagdHideSegment(p_curve->frenet_shapes[i]);
		if (!p_curve->animation) cagdHideSegment(p_curve->anime_frenet_shapes[i]);
	}
	if (!p_curve->draw_offset) cagdHideSegment(p_curve->offsetCurve);
	if (!p_curve->draw_evol) cagdHideSegment(p_curve->evolCurve);
}
 //this is the load and draw of a parametric curve!
 
void loadAndDrawFunc(int path, int unUsed, PVOID userData) {
	FILE* file = fopen(path, "r"); //
	if (!file) {
		perror("Error opening file");
		return 1;
	}
	struct Parametric_Curve* p_curve = (struct Parametric_Curve*)userData;
	char line[4][256];
	int i = 0;
	while (i < 4 && fgets(line[i], sizeof(line[i]), file) != NULL ) {
		if (line[i][0] == '#' || line[i][0] == '\n')
			continue;
		if(i<3)
			line[i][strlen(line[i]) - 1] = '\0';
		else {
			if (sscanf(line[i], "%lf %lf", &(p_curve->T[0]), &(p_curve->T[1])) != 2 || p_curve->T[1] < p_curve->T[0]) {
				myMessage("Change domain", "Bad domain!", MB_ICONERROR);
				return;
			}
			break;
		}
		i++;
	}
	fclose(file);
	p_curve->func[0] = e2t_expr2tree(line[0]);
	p_curve->func[1] = e2t_expr2tree(line[1]);
	p_curve->func[2] = e2t_expr2tree(line[2]);
	setup(p_curve);
}


static void setup(struct Parametric_Curve* p_curve) {
	char tr_funcStr[3][2048];
	char funcStr[2048];
	char dfuncStr[3][2048];
	char ddfuncStr[3][2048];
	char dddfuncStr[3][2048];
	if (p_curve->use_Tr) {
		e2t_expr_node* dtr = e2t_derivtree(p_curve->func_tr, E2T_PARAM_R);
		e2t_printtree(dtr, tr_funcStr[0]);
		e2t_expr_node* ddtr = e2t_derivtree(dtr, E2T_PARAM_R);
		e2t_printtree(ddtr, tr_funcStr[1]);
		e2t_expr_node* dddtr = e2t_derivtree(dtr, E2T_PARAM_R);
		e2t_printtree(dddtr, tr_funcStr[2]);
		e2t_freetree(dtr);
		e2t_freetree(ddtr);
		e2t_freetree(dddtr);
	}
	for (int i = 0; i < 3; i++) {
		if(p_curve->dfunc[i])
			e2t_freetree(p_curve->dfunc[i]);
		p_curve->dfunc[i] = e2t_derivtree(p_curve->func[i], E2T_PARAM_T);
		e2t_printtree(p_curve->dfunc[i], dfuncStr[i]);
		if (p_curve->ddfunc[i])
			e2t_freetree(p_curve->ddfunc[i]);
		p_curve->ddfunc[i] = e2t_derivtree(p_curve->dfunc[i], E2T_PARAM_T);
		e2t_printtree(p_curve->ddfunc[i], ddfuncStr[i]);
		if (p_curve->dddfunc[i])
			e2t_freetree(p_curve->dddfunc[i]);
		p_curve->dddfunc[i] = e2t_derivtree(p_curve->ddfunc[i], E2T_PARAM_T);
		e2t_printtree(p_curve->dddfunc[i], dddfuncStr[i]);
		if (p_curve->use_Tr) {
			sprintf(funcStr, "(%s)*(%s)", dfuncStr[i], tr_funcStr[0]);
			e2t_freetree(p_curve->dfunc[i]);
			p_curve->dfunc[i] = e2t_expr2tree(funcStr);
			sprintf(funcStr, "(%s)*(%s)^2 + (%s)*(%s)", ddfuncStr[i], tr_funcStr[0], dfuncStr[i], tr_funcStr[1]);
			e2t_freetree(p_curve->ddfunc[i]);
			p_curve->ddfunc[i] = e2t_expr2tree(funcStr);
			sprintf(funcStr, "(%s)*(%s)^3 + 3*(%s)*(%s)*(%s) + (%s)*(%s)", dddfuncStr[i], tr_funcStr[0], ddfuncStr[i], tr_funcStr[0],tr_funcStr[1], dfuncStr[i], tr_funcStr[2]);
			e2t_freetree(p_curve->dddfunc[i]);
			p_curve->dddfunc[i] = e2t_expr2tree(funcStr);
		}
	}
	drawCurve(p_curve, "main");
	cagdRedraw();
}


void fernLeftUp(int x, int y, PVOID userData)
{
	struct Parametric_Curve* p_curve = (struct Parametric_Curve*)userData;
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	p_curve->click = FALSE;
	hideSegments(p_curve);
	cagdRedraw();
}

void fernLeftMove(int x, int y, PVOID userData) {
	struct Parametric_Curve* p_curve = (struct Parametric_Curve*)userData;
	CAGD_POINT p;
	int v;
	if (v = cagdGetNearestVertex(p_curve->mainCurve, x, y)) {
		cagdGetVertex(p_curve->mainCurve, --v, &p);
		double time = p_curve->time_stamps[v];
		drawFernetShapes(p_curve, time, FALSE);
	}
	p_curve->click = TRUE;
	showSegments(p_curve);
	cagdRedraw();
}
void fernLeftDown(int x, int y, PVOID userData)
{
	cagdRegisterCallback(CAGD_LBUTTONUP, fernLeftUp, userData);
	cagdRegisterCallback(CAGD_MOUSEMOVE, fernLeftMove, userData);
}


void drawCircle(CAGD_POINT circle_center, GLdouble radius, CAGD_POINT fern[3], UINT* lines) {
	double domain[2] = { 0, 6.4 };
	e2t_expr_node* circle_func[3];
	char circle_func_strings[3][256];
	sprintf(circle_func_strings[0], "%lf + %lf*cos(t)*%lf + %lf*sin(t)*%lf", circle_center.x, radius, fern[0].x, radius, fern[1].x);
	sprintf(circle_func_strings[1], "%lf + %lf*cos(t)*%lf + %lf*sin(t)*%lf", circle_center.y, radius, fern[0].y, radius, fern[1].y);
	sprintf(circle_func_strings[2], "%lf + %lf*cos(t)*%lf + %lf*sin(t)*%lf", circle_center.z, radius, fern[0].z, radius, fern[1].z);
	circle_func[0] = e2t_expr2tree(circle_func_strings[0]);
	circle_func[1] = e2t_expr2tree(circle_func_strings[1]);
	circle_func[2] = e2t_expr2tree(circle_func_strings[2]);
	long long size;
	CAGD_POINT* p = createCurvePointCollection(domain, circle_func, FALSE, NULL, 15, &size);
	cagdReusePolyline(*lines, p, size);
	free(p);
	e2t_freetree(circle_func[0]);
	e2t_freetree(circle_func[1]);
	e2t_freetree(circle_func[2]);
}
void drawSphere(CAGD_POINT circle_center, GLdouble radius, CAGD_POINT fern[3], UINT* lines) {
	double domain[2] = { 0, 6.29 };
	e2t_expr_node *circle_func_x[3], *circle_func_y[3];
	char circle_func_strings[3][512];
	sprintf(circle_func_strings[0], "%lf + %lf*%lf*cos(t) + %lf*%lf*sin(t)", circle_center.x, radius, fern[0].x, radius, fern[1].x);
	sprintf(circle_func_strings[1], "%lf + cos(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t)) - sin(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t))", circle_center.y, radius, fern[0].y, radius, fern[1].y, radius, fern[0].z, radius, fern[1].z);
	sprintf(circle_func_strings[2], "%lf + sin(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t)) + cos(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t))", circle_center.z, radius, fern[0].y, radius, fern[1].y, radius, fern[0].z, radius, fern[1].z);
	circle_func_x[0] = e2t_expr2tree(circle_func_strings[0]);
	circle_func_x[1] = e2t_expr2tree(circle_func_strings[1]);
	circle_func_x[2] = e2t_expr2tree(circle_func_strings[2]);
	sprintf(circle_func_strings[0], "%lf + cos(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t)) + sin(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t))", circle_center.x, radius, fern[0].x, radius, fern[1].x, radius, fern[0].z, radius, fern[1].z);
	sprintf(circle_func_strings[1], "%lf + %lf*%lf*cos(t) + %lf*%lf*sin(t)", circle_center.y, radius, fern[0].y, radius, fern[1].y);
	sprintf(circle_func_strings[2], "%lf - sin(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t)) + cos(w)*(%lf*%lf*cos(t) + %lf*%lf*sin(t))", circle_center.z, radius, fern[0].x, radius, fern[1].x, radius, fern[0].z, radius, fern[1].z);
	circle_func_y[0] = e2t_expr2tree(circle_func_strings[0]);
	circle_func_y[1] = e2t_expr2tree(circle_func_strings[1]);
	circle_func_y[2] = e2t_expr2tree(circle_func_strings[2]);
	int j = 0;
	for (double i = 0; i < 3.14; i += 3.14/12) {
		e2t_setparamvalue(i, E2T_PARAM_W);
		long long sizex, sizey;
		CAGD_POINT* px = createCurvePointCollection(domain, circle_func_x, FALSE, NULL, 15, &sizex);
		CAGD_POINT* py = createCurvePointCollection(domain, circle_func_y, FALSE, NULL, 15, &sizey);
		cagdReusePolyline(lines[j], px, sizex);
		cagdReusePolyline(lines[j+1], py, sizey);
		free(px);
		free(py);
		j += 2;
	}
	e2t_freetree(circle_func_x[0]);
	e2t_freetree(circle_func_x[1]);
	e2t_freetree(circle_func_x[2]);
	e2t_freetree(circle_func_y[0]);
	e2t_freetree(circle_func_y[1]);
	e2t_freetree(circle_func_y[2]);
}
void drawCoil(CAGD_POINT coil_center, GLdouble radius, CAGD_POINT fern[3], double torsion, UINT* lines) {
	double domain[2];
	if (torsion == 0) {
		drawCircle(coil_center, 0.1, fern, lines);
		return;
	}
	domain[0] = 0.00;
	domain[1] = torsion* 3.14;
	e2t_expr_node* coil_func[3];
	char coil_func_strings[3][512];
	double factor = 0.15 / domain[1];
	sprintf(coil_func_strings[0], "%lf-t*cos(14*t)*%lf + t*sin(14*t)*%lf + t*%lf", coil_center.x, fern[0].x * factor, fern[1].x * factor, 2 * factor * fern[2].x * torsion);
	sprintf(coil_func_strings[1], "%lf-t*cos(14*t)*%lf + t*sin(14*t)*%lf + t*%lf", coil_center.y, fern[0].y * factor, fern[1].y * factor, 2 * factor * fern[2].y * torsion);
	sprintf(coil_func_strings[2], "%lf-t*cos(14*t)*%lf + t*sin(14*t)*%lf + t*%lf", coil_center.z, fern[0].z * factor, fern[1].z * factor, 2 * factor * fern[2].z * torsion);
	coil_func[0] = e2t_expr2tree(coil_func_strings[0]);
	coil_func[1] = e2t_expr2tree(coil_func_strings[1]);
	coil_func[2] = e2t_expr2tree(coil_func_strings[2]);
	long long size;
	CAGD_POINT* p = createCurvePointCollection(domain, coil_func, FALSE, NULL, 60, &size);
	if (p)
	{
		cagdReusePolyline(*lines, p, size);
		free(p);
	}
	e2t_freetree(coil_func_strings[0]);
	e2t_freetree(coil_func_strings[1]);
	e2t_freetree(coil_func_strings[2]);
}

void getFernetDataFromParametric(struct Parametric_Curve* p_curve, CAGD_POINT fern[3], double *curvi, double* tors) {
	CAGD_POINT dF = { (GLdouble)e2t_evaltree(p_curve->dfunc[0]), (GLdouble)e2t_evaltree(p_curve->dfunc[1]), (GLdouble)e2t_evaltree(p_curve->dfunc[2]) };
	CAGD_POINT ddF = { (GLdouble)e2t_evaltree(p_curve->ddfunc[0]), (GLdouble)e2t_evaltree(p_curve->ddfunc[1]), (GLdouble)e2t_evaltree(p_curve->ddfunc[2]) };
	CAGD_POINT dddF = { (GLdouble)e2t_evaltree(p_curve->dddfunc[0]), (GLdouble)e2t_evaltree(p_curve->dddfunc[1]), (GLdouble)e2t_evaltree(p_curve->dddfunc[2]) };
	getFernetData(dF, ddF, dddF, fern, curvi, tors);
}

void getFernetData(CAGD_POINT dF, CAGD_POINT ddF, CAGD_POINT dddF, CAGD_POINT fern[3], double* curvi, double* tors) {
	double sizeOfdF = norm(dF);

	CAGD_POINT dFxddF = cross_prod(dF, ddF);
	double sizeOfdFxddF = norm(dFxddF);
	
	*curvi = sizeOfdFxddF / (sizeOfdF * sizeOfdF * sizeOfdF);
	CAGD_POINT unit_binormal = normalize(dFxddF);
	CAGD_POINT unit_tangent = normalize(dF);
	CAGD_POINT unit_normal = cross_prod(unit_binormal, unit_tangent);
	fern[0] = unit_normal; fern[1] = unit_tangent; fern[2] = unit_binormal;
	if(tors)
		*tors = dot_prod(dFxddF, dddF) / (sizeOfdFxddF * sizeOfdFxddF);
}

void drawFernetShapes(struct Parametric_Curve* p_curve, double t, BOOL for_anime) {
	if (p_curve->use_Tr) {
		e2t_setparamvalue(t, E2T_PARAM_R);
		e2t_setparamvalue((GLdouble)e2t_evaltree(p_curve->func_tr), E2T_PARAM_T);
	}
	else
		e2t_setparamvalue(t, E2T_PARAM_T);
	CAGD_POINT p = { (GLdouble)e2t_evaltree(p_curve->func[0]) , (GLdouble)e2t_evaltree(p_curve->func[1]), (GLdouble)e2t_evaltree(p_curve->func[2]) };
	CAGD_POINT fren_axis[3];
	double curviture, torsion;
	getFernetDataFromParametric(p_curve,fren_axis,&curviture, &torsion);
	if (p_curve->draw_geo[0]) {
		CAGD_POINT norm[2] = { p, cagdPointAdd(p , fren_axis[0])};
		CAGD_POINT tang[2] = { p, cagdPointAdd(p , fren_axis[1]) };
		CAGD_POINT bino[2] = { p, cagdPointAdd(p , fren_axis[2]) };

		cagdReusePolyline(for_anime ? p_curve->anime_frenet_axis[0] : p_curve->frenet_axis[0], norm, sizeof(norm) / sizeof(CAGD_POINT));
		cagdReusePolyline(for_anime ? p_curve->anime_frenet_axis[1] : p_curve->frenet_axis[1], tang, sizeof(tang) / sizeof(CAGD_POINT));
		cagdReusePolyline(for_anime ? p_curve->anime_frenet_axis[2] : p_curve->frenet_axis[2], bino, sizeof(bino) / sizeof(CAGD_POINT));
	}
	if (curviture == 0)
		return;
	CAGD_POINT circle_center = cagdPointAdd(p , scaler(fren_axis[0],1/curviture));
	if (p_curve->draw_geo[1]) {
		drawCircle(circle_center, 1 / curviture, fren_axis, for_anime ? &p_curve->anime_frenet_shapes[0] : &p_curve->frenet_shapes[0]);
	}
	if(p_curve->draw_geo[3]){
		drawCoil(p, 1 / curviture, fren_axis, torsion, for_anime ? &p_curve->anime_frenet_shapes[2] : &p_curve->frenet_shapes[2]);
	}
	if (p_curve->draw_geo[2]) {
		//drawSphere(circle_center[1], 1 / curviture, fren_axis, &p_curve->frenet_shapes[1]);
	}
}

void drawCurve(struct Parametric_Curve* p_curve, char* which) {
	int size_of_p = (p_curve->D[1] - p_curve->D[0] + 1) * finesse * finesse * sizeof(CAGD_POINT), i = 0;
	UINT target = p_curve->mainCurve;
	if (strcmp(which, "offset") == 0)
		target = p_curve->offsetCurve;
	else if (strcmp(which, "evol") == 0)
		target = p_curve->evolCurve;
	else if (strcmp(which, "main") == 0) {
		target = p_curve->mainCurve;
		if (p_curve->time_stamps)
			free(p_curve->time_stamps);
		p_curve->time_stamps = (double*)malloc(size_of_p);
	}
	else
		return;
	CAGD_POINT* p = (CAGD_POINT*)malloc(size_of_p);
	CAGD_POINT fren_axis[3];
	double curviture, torsion;
	getFernetDataFromParametric(p_curve, fren_axis, &curviture, &torsion);
	for (double j = p_curve->D[0]; j <= p_curve->D[1]; j += (1 / ((double)finesse * finesse)), i++) {
		if (p_curve->use_Tr) {
			e2t_setparamvalue(j, E2T_PARAM_R);
			e2t_setparamvalue((GLdouble)e2t_evaltree(p_curve->func_tr), E2T_PARAM_T);
		}
		else
			e2t_setparamvalue(j, E2T_PARAM_T);
		p[i].x = (GLdouble)e2t_evaltree(p_curve->func[0]);
		p[i].y = (GLdouble)e2t_evaltree(p_curve->func[1]);
		p[i].z = (GLdouble)e2t_evaltree(p_curve->func[2]);
		if (target == p_curve->offsetCurve) {
			getFernetDataFromParametric(p_curve, fren_axis, &curviture, &torsion);
			p[i].x = p[i].x + p_curve->offset * fren_axis[0].x;
			p[i].y = p[i].y + p_curve->offset * fren_axis[0].y;
			p[i].z = p[i].z + p_curve->offset * fren_axis[0].z;
		}
		else if (target == p_curve->evolCurve) {
			getFernetDataFromParametric(p_curve, fren_axis, &curviture, &torsion);
			if (curviture == 0)
				continue;
			p[i].x += fren_axis[0].x / curviture;
			p[i].x += fren_axis[0].y / curviture;
			p[i].x += fren_axis[0].z / curviture;
		}
		else {
			p_curve->time_stamps[i] = j;
			p_curve->samples = i;
		}
	}
	cagdReusePolyline(target,p,i);
	cagdShowSegment(target);
	cagdRedraw();
	free(p);
}

void myAnimeTimer(int x, int y, PVOID userData)
{
	struct Parametric_Curve* p_curve = getParamCurveInstance();
	hideSegments(p_curve);
	p_curve->anime_fernet++;
	p_curve->anime_fernet %= p_curve->samples;
	drawFernetShapes( p_curve, p_curve->time_stamps[p_curve->anime_fernet], TRUE);
	showSegments(p_curve);
	cagdRedraw();
}


static void closeLastHandlerUsed(struct Parametric_Curve* p_curve, enum Parametric_Control flag) {
	unsigned int* bitmap = &p_curve->bitmap;
	switch (flag) {
	case CLICK:
		if (!(*bitmap & (1 << CLICK))) {
			cagdRegisterCallback(CAGD_LBUTTONDOWN, fernLeftDown, (PVOID)p_curve);
		}
		else {
			cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
			cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
		}
		break;
	case ANIM:
		p_curve->animation = !p_curve->animation;
		if (!(*bitmap & (1 << ANIM))) {
			update_timer_speed(10);
			cagdRegisterCallback(CAGD_TIMER, myAnimeTimer, NULL);
		}
		else{
			cagdRegisterCallback(CAGD_TIMER, NULL, NULL);
		}
		break;
	case TRDOM:
		if (!(*bitmap & (1 << TRDOM)))
			p_curve->D = p_curve->R;
		else
			p_curve->D = p_curve->T;
		break;
	case OFSE:
		p_curve->draw_offset = !p_curve->draw_offset;
		drawCurve(p_curve, "offset");
		break;
	case EVOL:
		p_curve->draw_evol = !p_curve->draw_evol;
		drawCurve(p_curve, "evol");
		break;
	case AXIS:
		p_curve->draw_geo[0] = !p_curve->draw_geo[0];
		break;
	case CURV:
		p_curve->draw_geo[1] = !p_curve->draw_geo[1];
		break;
	case SPHE:
		p_curve->draw_geo[2] = !p_curve->draw_geo[2];
		break;
	case COIL:
		p_curve->draw_geo[3] = !p_curve->draw_geo[3];
		break;
	default:
		return;
	}
	if (*bitmap & (1 << flag))
		*bitmap &= ~(1 << flag);
	else
		*bitmap |= (1 << flag);
}


void fernetControlCenter(int id, int indx, PVOID userData) {
	struct Parametric_Curve* p_curve = getParamCurveInstance();
	HMENU* hMenu = (HMENU*)userData;
	switch (id) {
	case FRN_OFPAR:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_T1),
			cagdGetWindow(),
			(DLGPROC)myDialogProc))
			if (sscanf(myBuffer, "%lf", &p_curve->offset) != 1) {
				myMessage("Try again", "something went wrong!", MB_ICONERROR);
				return;
			}
		if (p_curve->draw_offset)
			drawCurve(p_curve, "offset");
		break;
	case FRN_EVOL:
		closeLastHandlerUsed(p_curve, EVOL);
		break;
	case FRN_X:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_FUN),
			cagdGetWindow(),
			(DLGPROC)myDialogProc)) {
			if (p_curve->func[0])
				e2t_freetree(p_curve->func[0]);
			p_curve->func[0] = e2t_expr2tree(myBuffer);
			setup(p_curve);
		}
		else {
			return;
		}
		break;
	case FRN_Y:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_FUN),
			cagdGetWindow(),
			(DLGPROC)myDialogProc)) {
			if (p_curve->func[1])
				e2t_freetree(p_curve->func[1]);
			p_curve->func[1] = e2t_expr2tree(myBuffer);
			setup(p_curve);
		}
		else {
			return;
		}
		break;
	case FRN_Z:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_FUN),
			cagdGetWindow(),
			(DLGPROC)myDialogProc)) {
			if (p_curve->func[2])
				e2t_freetree(p_curve->func[2]);
			p_curve->func[2] = e2t_expr2tree(myBuffer);
			setup(p_curve);
		}
		else {
			return;
		}
		break;
	case FRN_T:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_T),
			cagdGetWindow(),
			(DLGPROC)myDialogProc))
			if (sscanf(myBuffer, "%lf %lf", &p_curve->T[0], &p_curve->T[1]) != 2 || p_curve->T[1] < p_curve->T[0]) {
				myMessage("Change domain", "Bad domain!", MB_ICONERROR);
				return;
			}
			else {
				drawCurve(p_curve,"main");
				return;
			}
		break;
	case FRN_TR:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_FUN1),
			cagdGetWindow(),
			(DLGPROC)myDialogProc2)) {
			char newFunction[512], newDomain[512];
			if (sscanf(myBuffer, "%s %lf %lf", &newFunction, &p_curve->R[0], &p_curve->R[1]) != 3 || p_curve->R[1] < p_curve->R[0]) {
				myMessage("Incorrect Arguments", "Something went wrong, please check your arguemnets", MB_ICONERROR);
				return;
			}
			if (p_curve->func_tr)
				e2t_freetree(p_curve->func_tr);
			p_curve->func_tr = e2t_expr2tree(newFunction);
			setup(p_curve);
		}
		else {
			return;
		}
		break;
	case FRN_R:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_T),
			cagdGetWindow(),
			(DLGPROC)myDialogProc)){
				if (sscanf(myBuffer, "%lf %lf", &p_curve->R[0], &p_curve->R[1]) != 2 || p_curve->R[1] < p_curve->R[0]) {
					myMessage("Change domain", "Bad domain!", MB_ICONERROR);
					return;
				}
				drawCurve(p_curve, "main");
		}
		break;
	case FRN_CNL:
		if (p_curve->func_tr) {
			p_curve->use_Tr = !p_curve->use_Tr;
			closeLastHandlerUsed(p_curve, TRDOM);
			drawCurve(p_curve, "main");
		}
		break;
	case FRN_FIN:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_F),
			cagdGetWindow(),
			(DLGPROC)myDialogProc))
			if (sscanf(myBuffer, "%d", &finesse) != 1 || finesse <= 0) {
				myMessage("Change finesse", "Bad finesse!", MB_ICONERROR);
				return;
			}
		finesse += 1;
		drawCurve(p_curve, "main");
		break;
	case FRN_LOAD:
		freeParamCurveInstance();
		p_curve = getParamCurveInstance();
		openFileName.hwndOwner = auxGetHWND();
		openFileName.lpstrTitle = "Load File";
		if (GetOpenFileName(&openFileName)) {
			loadAndDrawFunc((int)openFileName.lpstrFile, 0, (PVOID)p_curve);
		}
		EnableMenuItem(hMenu, FRN_CLICK, MF_ENABLED);
		EnableMenuItem(hMenu, FRN_AXS, MF_ENABLED);
		EnableMenuItem(hMenu, FRN_CRV, MF_ENABLED);
		EnableMenuItem(hMenu, FRN_SPHE, MF_ENABLED);
		EnableMenuItem(hMenu, FRN_TORS, MF_ENABLED);
		EnableMenuItem(hMenu, FRN_ANI, MF_ENABLED);
		EnableMenuItem(hMenu, FRN_OFFSET, MF_ENABLED);
		EnableMenuItem(hMenu, FRN_EVOL, MF_ENABLED);

		break;
	case FRN_CLICK:
		closeLastHandlerUsed(p_curve, CLICK);
		break;
	case FRN_AXS:
		closeLastHandlerUsed(p_curve, AXIS);
		break;
	case FRN_CRV:
		closeLastHandlerUsed(p_curve, CURV);
		break;
	case FRN_SPHE:
		closeLastHandlerUsed(p_curve, SPHE);
		break;
	case FRN_TORS:
		closeLastHandlerUsed(p_curve, COIL);
		break;
	case FRN_ANI:
		closeLastHandlerUsed(p_curve, ANIM);
		break;
	case FRN_OFFSET:
		closeLastHandlerUsed(p_curve, OFSE);
		break;
	}
	CheckMenuItem(hMenu, FRN_CLICK, (p_curve->bitmap& (1 << CLICK)) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_AXS, p_curve->bitmap& (1 << AXIS) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_CRV, p_curve->bitmap& (1 << CURV) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_SPHE, p_curve->bitmap& (1 << SPHE) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_TORS, p_curve->bitmap& (1 << COIL) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_ANI, p_curve->bitmap& (1 << ANIM) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_OFFSET, p_curve->bitmap& (1 << OFSE) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_EVOL, p_curve->bitmap& (1 << EVOL) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, FRN_CNL, p_curve->bitmap& (1 << TRDOM) ? MF_CHECKED : MF_UNCHECKED);
	hideSegments(p_curve);
	showSegments(p_curve);
	cagdRedraw();
}
void addFernMenu(HMENU hMenu){
	AppendMenu(hMenu, MF_STRING, FRN_LOAD, "Load Parmetric Curve");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, FRN_X, "X(t)");
	AppendMenu(hMenu, MF_STRING, FRN_Y, "Y(t)");
	AppendMenu(hMenu, MF_STRING, FRN_Z, "Z(t)");
	AppendMenu(hMenu, MF_STRING, FRN_T, "T");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, FRN_TR, "T(r)");
	AppendMenu(hMenu, MF_STRING, FRN_R, "R");
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_CNL, "Use Transformation");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, FRN_FIN, "Finesse");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_CLICK, "Click");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_AXS, "Add Fernet System");
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_CRV, "Add osilating circle");
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_SPHE, "Add osilating Sphere");
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_TORS, "Add osilating coil");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_ANI, "Animate");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_OFFSET, "Add Offset Curve");
	AppendMenu(hMenu, MF_STRING, FRN_OFPAR, "Offset value");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | MF_GRAYED, FRN_EVOL, "Add Evolution Curve");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	cagdAppendMenu(hMenu, "Parameteric Curves");
}