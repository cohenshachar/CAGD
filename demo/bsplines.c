#include "bsplines.h"

char myBuffer[BUFSIZ];

typedef struct Bspline_Node Bspline_Node;
typedef struct Bspline_Manager Bspline_Manager;

static void inti_bspline_manager(Bspline_Manager* list);
void* delete_curve_list(Bspline_Manager* list);
void myClickRightUp(int x, int y, PVOID userData);
void updateKnotsRightUp(int x, int y, PVOID userData);
void insertKnotsLeftDown(int x, int y, PVOID userData);
void updateKnotsLeftDown(int x, int y, PVOID userData);
void editWeightsLeftDown(int x, int y, PVOID userData);
void addCPLeftDown(int x, int y, PVOID userData);
static void editCPLeftDown(int x, int y, PVOID userData);
void moveCurveLeftDown(int x, int y, PVOID userData);
void knotInsertion(Curve* c, double value);
CP_List_Node* getClosestControlPoint(Curve* c, int x, int y);
static void setup(Curve* c);

typedef enum {
	BS_ADDKN = BSPLINE_CMD_START,
	BS_REMOVEKN,
	BS_BEZIER,
	BS_BSPLINE,
	BS_LOAD,
	BS_CONC0,
	BS_CONC1,
	BS_CONG1,
	BS_Move,
	BS_ADD,
	BS_REMOVECP,
	BS_WEIGHT,
	BS_REMOVEC,
	BS_KNOTS,
	BS_INKNOT,
	BS_DEG,
	BS_OEND,
	BS_CEND,
	BS_FIN,
} bsplineCommands;


enum popUpFlags {
	FLAG_ADD = 0,
	FLAG_WIEGHT,
	FLAG_EDKNOT,
	FLAG_INSKNOT,
	FLAG_MOVE,
	FLAG_REMOVE,
};
enum bezFlags {
	FLAG_BEZ = 0,
	FLAG_BSP,
	FLAG_C0,
	FLAG_C1,
	FLAG_G1,
	FLAG_NA,
};

struct Curve {
	int order;
	CP_List* cp_list;
	Kn_List* kn_list;
	BOOL is_bspline;
	BOOL zombie;
	BOOL closedfloat;
	BOOL uniform;
	BOOL showWeights;
	e2t_expr_node** bern_funcs;
	double* combinations;
	TimeStampedPolyGroup spline;
	TimeStampedPolyGroup dead_spline;
	UINT knotInsertCurveIndi;
	CP_List_Node* movingPoint;
	Knot* movingKnot;
};

struct Bspline_Node {
	Curve* c;
	Bspline_Node* next;
	Bspline_Node* prev;
};

struct Bspline_Manager {
	int length;
	Bspline_Node* head;
	Bspline_Node* tail;
	unsigned int popUpBitMap;
	unsigned int bezBitMap;
	Curve* connecti1;
	Curve* connecti2;
	Bspline_Node* last_clicked;
	CAGD_POINT moving_ref;
	HMENU myPopup, myKnotPopup;
};

static Bspline_Manager* Bspline_list = NULL;

Bspline_Manager* getBsplineInstance() {
	if (Bspline_list == NULL) {
		Bspline_list = SAFE_MALLOC(Bspline_Manager, 1);
		if (Bspline_list != NULL) {
			inti_bspline_manager(Bspline_list); // Initialize the value
		}
	}
	return Bspline_list;
}

void freeBsplineInstance() {
	if (Bspline_list != NULL) {
		delete_curve_list(Bspline_list);
		free(Bspline_list);
		Bspline_list = NULL;
	}
}

void init_curve(Curve* c) {
	c->order = 0;
	c->cp_list = NULL;
	cp_list_init(&c->cp_list);
	c->kn_list = NULL;
	kn_list_init(&c->kn_list);
	c->is_bspline = FALSE;
	c->zombie = FALSE;
	c->closedfloat = FALSE;
	c->uniform = FALSE;
	c->bern_funcs = NULL;
	c->combinations = NULL;
	c->movingPoint = NULL;
	c->showWeights = FALSE;
	initPolyLineGroup(&c->spline);
	initPolyLineGroup(&c->dead_spline);

}

static void inti_bspline_manager(struct Bspline_Manager* list) {
	list->head = NULL;
	list->tail = NULL;
	list->popUpBitMap = 0;
	list->bezBitMap = 0;
	list->length = 0;
	list->connecti1 = NULL;
	list->connecti2 = NULL;
	list->last_clicked = NULL;
}

void init_bspline_node(Bspline_Node* c_node) {
	c_node->next = NULL;
	c_node->prev = NULL;
	c_node->c = NULL;
}

Bspline_Node* push_back_curve(Curve* c) {
	Bspline_Manager* list = getBsplineInstance();
	Bspline_Node* c_node = SAFE_MALLOC(Bspline_Node, 1);
	init_bspline_node(c_node);
	c_node->c = c;
	if (!list->head)
		list->head = list->tail = c_node;
	else {
		c_node->prev = list->tail;
		list->tail->next = c_node;
		list->tail = c_node;
	}
	list->length++;
	return c_node;
}

void* delete_curve(Curve* c) {
	c->cp_list = delete_cp_list(c->cp_list);
	c->kn_list = delete_kn_list(c->kn_list);
	if (c->bern_funcs) free(c->bern_funcs);
	if (c->combinations) free(c->combinations);
	if (c->dead_spline.segments != c->spline.segments) emptyStampedPolyGroup(&c->dead_spline);
	emptyStampedPolyGroup(&c->spline);
	return NULL;
}

BOOL remove_curve_node(Bspline_Node* c_node) {
	Bspline_Manager* list = getBsplineInstance();
	if (!c_node) return FALSE;
	c_node->c = delete_curve(c_node->c);
	if (!c_node->prev && !c_node->next) {
		list->head = NULL;
		list->tail = NULL;
	}
	else if (!c_node->next) {
		c_node->prev->next = NULL;
		list->tail = c_node->prev;
	}
	else if (!c_node->prev) {
		c_node->next->prev = NULL;
		list->head = c_node->next;
	}
	else {
		c_node->next->prev = c_node->prev;
		c_node->prev->next = c_node->next;
	}
	free(c_node);
	list->length--;
	return TRUE;
}

void* delete_curve_list(Bspline_Manager* list) {
	while (list->head)
		remove_curve_node(list->head);
	return NULL;
}

void draw(Curve* c) {
	if (drawSpline(c, finesse) == TRUE) {
		for (int i = 0; i < c->dead_spline.len; i++) {
			cagdFreeSegment(c->dead_spline.segments[i].line_id);
		}
		c->dead_spline = c->spline;
		c->zombie = FALSE;
	}
	else {
		c->spline.segments = NULL;
		c->spline.len = 0;
		for (int i = 0; i < c->dead_spline.len; i++) {
			cagdSetSegmentColor(c->dead_spline.segments[i].line_id, 100, 100, 100);
		}
		c->zombie = TRUE;
	}
	cagdRedraw();
}

void move_curve(Curve* c, CAGD_POINT delta) {
	transform_list(c->cp_list, delta);
	draw(c);
}

void rotate_curve(Curve* c, CAGD_POINT center, double degree) {
	rotate_list(c->cp_list, center, degree);
	draw(c);
}

BOOL isFull(Curve* c) {
	BOOL res = (c->is_bspline) ? (cp_list_len(c->cp_list) == (kn_list_len(c->kn_list) - c->order)) : (cp_list_len(c->cp_list) == c->order);
	return res;
}

static double* createCombinationsArray(int n) {
	double* res = malloc(sizeof(double) * (n + 1));
	res[0] = 1;
	res[n] = 1;
	for (int i = 1; i < n / 2 + 1; i++) {
		res[n - i] = res[i] = (res[i - 1] * (n - i + 1)) / i;
	}
	return res;
}

void loadCurve(int path, int unUsed, PVOID userData) {
	FILE* file = fopen(path, "r");
	if (!file) {
		perror("Error opening file");
		return 1;
	}
	HMENU* hMenu = (HMENU*)userData;
	char line[256];
	int i = 0, p_i = 0;
	Curve* new_c = (Curve*)malloc(sizeof(Curve));
	init_curve(new_c);
	cagdSetColor(0, 255, 255);
	while (fgets(line, sizeof(line), file) != NULL) {
		int j = 0;
		while (line[j] == ' ') {
			j++;
		}
		if (line[j] == '#' || line[j] == '\n')
			continue;
		if (i == 0) {
			if (sscanf(line, " %d", &(new_c->order)) != 1 || new_c->order < 0) {
				myMessage("Change Order", "Unacceptable Order!", MB_ICONERROR);
				return;
			}
			i++;
		}
		else if (i == 1) {
			int kn_len, index;
			if (sscanf(line, " knots[%d] =%n", &kn_len, &index) == 1) {
				if (kn_len < 1 || kn_len <= new_c->order) {
					myMessage("Change Number Of Knots", "Should be a natural number (not including 0)!", MB_ICONERROR);
					return;
				}
				new_c->is_bspline = TRUE;
				int offset = index, j = 0;
				double knot;
				while (j < kn_len) {
					if (sscanf(line + offset, " %lf%n", &knot, &index) != 1) {
						if (fgets(line, sizeof(line), file) == NULL)
							return;
						offset = 0;
					}
					else {
						kn_list_insert(new_c->kn_list, knot, 1);
						//	sprintf(logMsg, "j: %d , knot = %lf \n", j, knot);
						//	LogToOutput(logMsg);
						offset += index;
						j++;
					}
				}
			}
			else {
				double x, y, w;
				int res = sscanf(line, "%lf %lf %lf", &x, &y, &w);
				if (res < 2) {
					myMessage("hello", "Should be a natural number (not including 0)!", MB_ICONERROR);
					return;
				}
				else if (res == 2)
					w = 1;
				push_back_cp(new_c->cp_list, (CAGD_POINT) { x / w, y / w, 0 }, w);
				p_i++;
			}
			i++;
		}
		else {
			double x, y, w;
			int res = sscanf(line, "%lf %lf %lf", &x, &y, &w);
			if (res < 2) {
				myMessage("hello", "Should be a natural number (not including 0)!", MB_ICONERROR);
				return;
			}
			else if (res == 2)
				w = 1;
			push_back_cp(new_c->cp_list, (CAGD_POINT) { x / w, y / w, 0 }, w);
			p_i++;
			if (isFull(new_c)) {
				setup(new_c);
				new_c = (Curve*)malloc(sizeof(Curve));
				init_curve(new_c);
				i = 0;
				p_i = 0;
			}
		}
	}
	fclose(file);
}

static void resetBezier(Curve* c) {
	int cp_len = cp_list_len(c->cp_list);
	if (c->bern_funcs) {
		for (int i = 0; i < cp_len - 1; i++)
			e2t_freetree(c->bern_funcs[i]);
		free(c->bern_funcs);
	}
	c->bern_funcs = (e2t_expr_node**)malloc(cp_len * sizeof(e2t_expr_node*));
	char buf[256];
	for (int i = 0; i < cp_len; i++) {
		sprintf(buf, "((1-t)^(%d - %d))*(t^%d)", cp_len - 1, i, i);
		c->bern_funcs[i] = e2t_expr2tree(buf);
	}
	if (c->combinations) free(c->combinations);
	c->combinations = createCombinationsArray(cp_len - 1);
	draw(c);
}

static void setup(Curve* c) {
	toggle_cp_list_visiblity(c->cp_list, FALSE, FALSE, FALSE);
	if (!c->is_bspline)
		resetBezier(c);
	else {
		c->closedfloat = check_closed_float(c->kn_list, c->order);
		c->uniform = check_uniform_spacing(c->kn_list);
		draw(c);
	}
	push_back_curve(c);
}

int evaluateJ(Kn_List* kn_list, double t) {
	return kn_list_upper_index(kn_list, t) - 1;
}

Control_Point** evaluateBsplineInvolvedCPs(const Curve* c, double t) {
	int J = evaluateJ(c->kn_list, t);
	return get_cp_array_range(c->cp_list, J - c->order + 1 , c->order);
}

Control_Point** evaluateInvolvedCPs(Control_Point** cp_arr,int J, int K) {
	return cp_arr + J - K + 1;
}

double* evaluateBsplineBaseFunctions(Kn_List* kn_list, double t, int J, int K, int layer) {
	int k = K - 1;
	if (J - k < 0)
		return NULL;
	int K_layers = K - layer;
	double* knots = get_knot_range(kn_list, J-k, 2*k+1);
	double** B = SAFE_MALLOC(double*, K_layers);
	for (int i = 0; i < K_layers; i++) {
		B[i] = SAFE_MALLOC(double, K);
		for (int j = 0; j < k - i; j++)
			B[i][j] = 0;
		for (int j = k - i; j < K; j++) {
			if (i == 0) {
				B[i][j] = 1;
				continue;
			}
			double left_leg = (knots[j + i] - knots[j]) == 0 ? 0 : (t - knots[j]) * B[i - 1][j] / (knots[j + i] - knots[j]);
			if (j == k) {
				B[i][j] = left_leg;
			}
			else {
				double right_leg = (knots[j + i + 1] - knots[j + 1]) == 0 ? 0 : (knots[j + i + 1] - t) * B[i - 1][j + 1] / (knots[j + i + 1] - knots[j + 1]);
				B[i][j] = left_leg + right_leg;
			}
		}
	}
	double* res = B[K_layers-1];
	for (int i = 0; i < K_layers-1; i++)
		free(B[i]);
	free(B);
	free(knots);
	return res;
}

CAGD_POINT evaluateBspline(Control_Point** controls, double* basis, int K, BOOL rational,double* optional_norm) {
	CAGD_POINT res = { 0,0,0 };
	GLdouble sum_mechane = 0;

	for (int b = 0; b < K; b++) {
		double weight = get_weight(controls[b]);
		CAGD_POINT q = get_location(controls[b]);
		double mult = rational ?  basis[b] * weight : basis[b];
		res = cagdPointAdd(res, scaler(q, mult));
		sum_mechane += basis[b] * weight;
	}
	if (optional_norm)
		*optional_norm = sum_mechane;
	return rational ? scaler(res, 1/sum_mechane) : res;
}

CAGD_POINT evaluateBsplineAtT(const Curve* c, double t, BOOL rational) {
	double domain[2];
	get_domain(c->kn_list,c->order, domain);
	if (t < domain[0])
		t = domain[0];
	else if (t >= domain[1]) {
		t =  domain[1] - 0.000001;
	}
	double* basis = evaluateBsplineBaseFunctions(c->kn_list, t, evaluateJ(c->kn_list,t), c->order, 0);
	Control_Point** involvedCps = evaluateBsplineInvolvedCPs(c, t);
	CAGD_POINT res = evaluateBspline(involvedCps, basis, c->order, rational,NULL);
	free(basis);
	free(involvedCps);
	return res;
}

CAGD_POINT evaluateAtT(Control_Point** control_points, Kn_List* knots, int K, double t, BOOL rational, double* optional_norm) {
	int k = K - 1;
	int J = evaluateJ(knots, t);
	if (J < k)
		t = get_value(get_knot_at_index(knots, k));
	else if (J >= kn_list_len(knots) - K) {
		t = get_value(get_knot_at_index(knots, kn_list_len(knots) - K)) - 0.000001;
		J= kn_list_len(knots) - K-1;
	}
	double* basis = evaluateBsplineBaseFunctions(knots, t, J, K, 0);		
	CAGD_POINT res = evaluateBspline(control_points + (J - k), basis, K,rational, optional_norm);
	free(basis);
	return res;
}

TimeStampedPolyGroup plot_bspline_function(Control_Point** control_points, int cp_len, Kn_List* knots, int K, int fin, BOOL rational) {
	int kn_len = kn_list_len(knots);
	if (!((kn_len - cp_len - K) == 0 && cp_len >= K))
		return (TimeStampedPolyGroup) { NULL, 0 };
	int k = K - 1, curve_len = 0, J = k, curves = 0;
	double domain[2];
	domain[0] = get_value(get_knot_at_index(knots, k));
	domain[1] = get_value(get_knot_at_index(knots, kn_len - (k + 1)));
	double delta = (domain[1] - domain[0]) / ((double)fin * cp_len);
	CAGD_POINT* p = SAFE_MALLOC(CAGD_POINT, (cp_len + 1) * fin);
	double* time_stamps = SAFE_MALLOC(double, (cp_len + 1) * fin);
	TimeStampedPolyline* tmp = SAFE_MALLOC(TimeStampedPolyline, (cp_len + 1) * fin / 2);
	BOOL singular_point = TRUE;
	for (double t = domain[0]; t < domain[1]; t += delta, curve_len++) {
		int last_J = J;
		J = evaluateJ(knots, t);
		double* basis = evaluateBsplineBaseFunctions(knots, t, J, K, 0);
		p[curve_len] = evaluateBspline(control_points + (J - k), basis, K, rational,NULL);
		if (curve_len > 0)
			if (p[curve_len].x != p[curve_len - 1].x || p[curve_len].y != p[curve_len - 1].y || p[curve_len].z != p[curve_len - 1].z)
				singular_point = FALSE;
		time_stamps[curve_len] = t;
		free(basis);
		if (J - last_J > k) {
			if(!singular_point){
				initPolyLine(&tmp[curves]);
				t = get_value(get_knot_at_index(knots, J));
				basis = evaluateBsplineBaseFunctions(knots, t-0.000001, last_J, K, 0);
				p[curve_len] = evaluateBspline(control_points + (last_J - k), basis, K, rational, NULL);
				time_stamps[curve_len] = t- 0.000001;
				tmp[curves].line_id = cagdAddPolyline(p, curve_len+1);
				tmp[curves].len = curve_len;
				tmp[curves].time_stamps = SAFE_MALLOC(double, curve_len+1);
				memmove(tmp[curves].time_stamps, time_stamps, (curve_len+1) * sizeof(double));
				curves++;
				free(basis);
				singular_point = TRUE;
			}
			curve_len = -1;
		}
	}
	if (!singular_point) {
		initPolyLine(&tmp[curves]);
		double* basis = evaluateBsplineBaseFunctions(knots, domain[1]- 0.000001, kn_len - (k + 1) - 1, K, 0);
		p[curve_len] = evaluateBspline(control_points + (kn_len - 2*(k + 1)), basis, K, rational, NULL);
		time_stamps[curve_len] = domain[1] - 0.000001;
		tmp[curves].line_id = cagdAddPolyline(p, curve_len+1);
		tmp[curves].len = curve_len;
		tmp[curves].time_stamps = SAFE_MALLOC(double, curve_len+1);
		memmove(tmp[curves].time_stamps, time_stamps, (curve_len+1) * sizeof(double));
		free(basis);
		curves++;
	}
	TimeStampedPolyGroup res;
	initPolyLineGroup(&res);
	if(curves != 0){
		res.segments = SAFE_MALLOC(TimeStampedPolyline, curves);
		res.len = curves;
		memmove(res.segments, tmp, curves * sizeof(TimeStampedPolyline));
	}
	free(p);
	free(tmp);
	free(time_stamps);
	return res;
}

BOOL drawBspline(Curve* c, int fin) {
	int cp_len = cp_list_len(c->cp_list);
	if (!((kn_list_len(c->kn_list) - cp_len - c->order) == 0 && cp_len >= c->order))
		return FALSE;
	cagdSetColor(0, 255, 255);
	Control_Point** cp_arr = get_cp_array(c->cp_list);
	c->spline = plot_bspline_function(cp_arr, cp_len, c->kn_list, c->order, fin, TRUE);
	free(cp_arr);
	return c->spline.segments != NULL;
}

BOOL drawBezier(Curve* c, int fin) {
	int cp_len = cp_list_len(c->cp_list);
	if (!(!c->is_bspline && (cp_len == c->order) && cp_len > 1))
		return FALSE;
	cagdSetColor(0, 255, 255);
	double domain[2] = { 0.0,1.0 };
	int curve_len = 0;
	double d = domain[1] - domain[0];
	CAGD_POINT* p = SAFE_MALLOC(CAGD_POINT, (cp_len + 1) * fin);
	TimeStampedPolyline* bez_poly = SAFE_MALLOC(TimeStampedPolyline, 1);
	bez_poly->time_stamps = SAFE_MALLOC(double, (cp_len + 1) * fin);
	for (double t = domain[0]; t < domain[1]; t += (d / ((double)fin * cp_len)), curve_len++) {
		GLdouble sum_z = 0, sum_y = 0, sum_x = 0, mechane = 0, sum_mechane = 0;
		CP_List_Node* cp_node = get_cp_by_index(c->cp_list, 0);
		e2t_setparamvalue(t, E2T_PARAM_T);
		for (int b = 0; b < cp_len; b++) {
			Control_Point* cp = get_cp(cp_node);
			CAGD_POINT q = get_location(cp);
			mechane = get_weight(cp) * e2t_evaltree(c->bern_funcs[b]) * c->combinations[b];
			sum_x += mechane * q.x;
			sum_y += mechane * q.y;
			sum_z += mechane * q.z;
			sum_mechane += mechane;
			cp_node = get_next(cp_node);
			}
		p[curve_len].x = sum_x / sum_mechane;
		p[curve_len].y = sum_y / sum_mechane;
		p[curve_len].z = sum_z / sum_mechane;
		bez_poly->time_stamps[curve_len] = t;
	}
	bez_poly->line_id = cagdAddPolyline(p, curve_len);
	bez_poly->len = curve_len;
	c->spline.segments = bez_poly;
	c->spline.len = 1;
	return TRUE;
}

BOOL drawSpline(Curve* c, int fin) {
	if (c->is_bspline)
		return drawBspline(c, fin);
	return drawBezier(c, fin);
}

Bspline_Node* findCurve(UINT id) {
	Bspline_Node* curr = getBsplineInstance()->head;
	while (curr) {
		if (containsPoly(curr->c->spline, id) != -1)
			return curr;
		curr = curr->next;
	}
	return NULL;
}

void prevCurve(int x, int y, PVOID userData) {
	Bspline_Node* c_node = (Bspline_Node*)userData;
	if (c_node->prev) toggleShowOrHideControl(c_node->prev);
}

void nextCurve(int x, int y, PVOID userData) {
	Bspline_Node* c_node = (Bspline_Node*)userData;
	if (c_node->next) toggleShowOrHideControl(c_node->next);
}

static BOOL toggleShowOrHideControl(Bspline_Node* c_node) {
	if (!c_node)
		return FALSE;
	BOOL isLastClicked = (getBsplineInstance()->last_clicked != NULL && c_node == getBsplineInstance()->last_clicked);
	if (!isLastClicked)
		toggleShowOrHideControl(getBsplineInstance()->last_clicked);

	if (isLastClicked) // hide
	{
		toggle_cp_list_visiblity(c_node->c->cp_list, FALSE, FALSE, FALSE);
		getBsplineInstance()->last_clicked = NULL;
		cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
		cagdRegisterCallback(CAGD_RIGHT, NULL, NULL);
		cagdRegisterCallback(CAGD_LEFT, NULL, NULL);
		cagdRedraw();
		return FALSE;
	}
	else { // show
		getBsplineInstance()->last_clicked = c_node;
		toggle_cp_list_visiblity(c_node->c->cp_list, TRUE, TRUE, c_node->c->showWeights);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, editCPLeftDown, c_node->c);
		cagdRegisterCallback(CAGD_RIGHT, nextCurve, (PVOID)c_node);
		cagdRegisterCallback(CAGD_LEFT, prevCurve, (PVOID)c_node);
		cagdRedraw();
		return TRUE;
	}

}

static Bspline_Node* getClosestCurve(int x, int y) {
	UINT id, min_id =-1;
	CAGD_POINT p;
	GLdouble min_dist = 0x7ff0000000000000;
	for (cagdPick(x, y); id = cagdPickNext();)
		if (cagdGetSegmentType(id) == CAGD_SEGMENT_POLYLINE) {
			UINT v;
			GLdouble temp_dist;
			if (v = cagdGetNearestVertex(id, x, y)) {
				cagdGetVertex(id, --v, &p);
				int X, Y; 
				cagdToWindow(&p, &X, &Y);
				temp_dist = (X - x) * (X - x) + (Y - y) * (Y - y);
				if (temp_dist < min_dist)
					min_id = id;
			}
		}
	return findCurve(min_id);
}

static void defaultBsplineLeftClick(int x, int y, PVOID userData)
{
	Bspline_Manager* list = getBsplineInstance();
	Bspline_Node* c_node = list->last_clicked ? list->last_clicked : getClosestCurve(x, y);
	if (c_node) {
		if (toggleShowOrHideControl(c_node)) {
			cagdRegisterCallback(CAGD_RBUTTONUP, myClickRightUp, userData);
		}
	}
}

Curve* getBsplineFromClick(int x, int y)
{
	Bspline_Node* c_node = getClosestCurve(x, y);
	return c_node ? c_node->c : NULL;
}

static void weightsHandler(Curve* c, BOOL flag) {
	if (flag) {
		c->showWeights = TRUE;
		cagdRegisterCallback(CAGD_LBUTTONDOWN, editWeightsLeftDown, (PVOID)c);
		cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
		toggle_cp_list_visiblity(c->cp_list, TRUE, TRUE, c->showWeights);
	}
	else {
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, editCPLeftDown, (PVOID)c);
		c->showWeights = FALSE;
		toggle_cp_list_visiblity(c->cp_list, TRUE, TRUE, c->showWeights);
	}
}

static void knotsHandler(Curve* c, BOOL flag) {
	if (flag) {
		draw_KV(c->kn_list);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, updateKnotsLeftDown, (PVOID)c);
		cagdRegisterCallback(CAGD_RBUTTONUP, updateKnotsRightUp, (PVOID)NULL);
		cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	}
	else {
		erase_KV(c->kn_list);
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, editCPLeftDown, (PVOID)c);
		cagdRegisterCallback(CAGD_RBUTTONUP, myClickRightUp, (PVOID)NULL);
		c->closedfloat = check_closed_float(c->kn_list, c->order);
		c->uniform = check_uniform_spacing(c->kn_list);
	}
}

static void inknotsHandler(Curve* c, BOOL flag) {
	if (flag) {
		draw_KV(c->kn_list);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, insertKnotsLeftDown, (PVOID)c);
		cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	}
	else {
		erase_KV(c->kn_list);
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, editCPLeftDown, (PVOID)c);
		c->closedfloat = check_closed_float(c->kn_list, c->order);
		c->uniform = check_uniform_spacing(c->kn_list);
	}

}

static void addHandler(Curve* c, BOOL flag) {
	if (flag) {
		cagdRegisterCallback(CAGD_LBUTTONDOWN, addCPLeftDown, (PVOID)c);
	}
	else {
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, editCPLeftDown, (PVOID)c);
	}

}

static void moveHandler(Curve* c, BOOL flag) {
	if (flag) {
		cagdRegisterCallback(CAGD_LBUTTONDOWN, moveCurveLeftDown, (PVOID)c);
	}
	else {
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, editCPLeftDown, (PVOID)c);
	}

}

static void closeLastHandlerUsed(Curve* c, enum popUpflag flag) {
	unsigned int* bitmap = &getBsplineInstance()->popUpBitMap;
	BOOL toActivate = !((*bitmap & (1 << flag)));
	if (*bitmap & (1 << FLAG_ADD)) {
		addHandler(c, FALSE);
		*bitmap ^= (1 << FLAG_ADD);
	}
	if (*bitmap & (1 << FLAG_WIEGHT)) {
		weightsHandler(c, FALSE);
		*bitmap ^= (1 << FLAG_WIEGHT);
	}
	if (*bitmap & (1 << FLAG_EDKNOT)) {
		knotsHandler(c, FALSE);
		*bitmap ^= (1 << FLAG_EDKNOT);
	}
	if (*bitmap & (1 << FLAG_INSKNOT)) {
		inknotsHandler(c, FALSE);
		*bitmap ^= (1 << FLAG_INSKNOT);
	}
	if (*bitmap & (1 << FLAG_MOVE)) {
		moveHandler(c, FALSE);
		*bitmap ^= (1 << FLAG_MOVE);
	}
	if (toActivate) {
		switch (flag) {
		case FLAG_MOVE:
			moveHandler(c, TRUE);
			break;
		case FLAG_ADD:
			addHandler(c, TRUE);
			break;
		case FLAG_WIEGHT:
			weightsHandler(c, TRUE);
			break;
		case FLAG_EDKNOT:
			knotsHandler(c, TRUE);
			break;
		case FLAG_INSKNOT:
			inknotsHandler(c, TRUE);
			break;
		}
		*bitmap |= (1 << flag);
	}
}

static void myClickRightUp(int x, int y, PVOID userData)
{
	Bspline_Manager* list = getBsplineInstance();
	Bspline_Node* c_node = list->last_clicked;
	if (!c_node)
		return;
	HMENU hMenu = list->myPopup;

	EnableMenuItem(hMenu, BS_Move, !c_node->c->zombie ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, BS_KNOTS, c_node->c->is_bspline ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, BS_INKNOT, c_node->c->is_bspline ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, BS_DEG, c_node->c->is_bspline ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, BS_CEND, c_node->c->is_bspline ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem(hMenu, BS_OEND, c_node->c->is_bspline ? MF_ENABLED : MF_GRAYED);

	CheckMenuItem(hMenu, BS_Move, (list->popUpBitMap & (1 << FLAG_MOVE)) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_ADD, list->popUpBitMap & (1 << FLAG_ADD) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_WEIGHT, (list->popUpBitMap & (1 << FLAG_WIEGHT)) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_REMOVEC, FALSE ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_KNOTS, list->popUpBitMap & (1 << FLAG_EDKNOT) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_INKNOT, list->popUpBitMap & (1 << FLAG_INSKNOT) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_DEG, FALSE ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_CEND, c_node->c->closedfloat && c_node->c->uniform ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_OEND, !c_node->c->closedfloat && c_node->c->uniform ? MF_CHECKED : MF_UNCHECKED);

	CP_List_Node* cp_node;
	switch (cagdPostMenu(hMenu, x, y)) {
	case BS_Move:
		closeLastHandlerUsed(c_node->c, FLAG_MOVE);
		break;
	case BS_ADD:
		closeLastHandlerUsed(c_node->c, FLAG_ADD);
		break;
	case BS_REMOVECP:
		if ((cp_node = getClosestControlPoint(c_node->c, x, y))) {
			remove_cp(c_node->c->cp_list, get_cp(cp_node));
			if (!c_node->c->is_bspline) {
				c_node->c->order = cp_list_len(c_node->c->cp_list);
				resetBezier(c_node->c);
			}
			else if (c_node->c->uniform) {
				remove_from_uniform(c_node->c->kn_list);
			}
			draw(c_node->c);
		}
		break;
	case BS_WEIGHT:
		closeLastHandlerUsed(c_node->c, FLAG_WIEGHT);
		break;
	case BS_REMOVEC:
		closeLastHandlerUsed(c_node->c, FLAG_REMOVE);
		remove_curve_node(c_node);
		list->last_clicked = NULL;
		break;
	case BS_KNOTS:
		closeLastHandlerUsed(c_node->c, FLAG_EDKNOT);
		break;
	case BS_INKNOT:
		closeLastHandlerUsed(c_node->c, FLAG_INSKNOT);
		break;
	case BS_DEG:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_T2),
			cagdGetWindow(),
			(DLGPROC)myDialogProc))
			if (sscanf(myBuffer, "%d", &c_node->c->order) != 1 || c_node->c->order <= 1) {
				myMessage("Change degree", "Unacceptable degree (should by deg >= 2) !", MB_ICONERROR);
				return;
			}
		draw(c_node->c);
		break;
	case BS_CEND:
		remake_as_closed_uni(&c_node->c->kn_list,cp_list_len(c_node->c->cp_list), c_node->c->order);
		draw(c_node->c);
		break;
	case BS_OEND:
		remake_as_open_uni(&c_node->c->kn_list, cp_list_len(c_node->c->cp_list), c_node->c->order);
		draw(c_node->c);
		break;
	}
	cagdRedraw();
}

static void editCPMove(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	update_control_node_2(c->movingPoint, x, y);
	draw(c);
}

static void editWeightsMove(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	update_weight_from_window(get_cp(c->movingPoint), x, y);
	draw(c);
}

static void moveCurveMove(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	CAGD_POINT q[2], p = get_location(get_cp(c->movingPoint));
	cagdToObjectAtDepth(x, y, p, q);
	q[0] = q[1];
	q[1] = p;
	if (q[0].x - q[1].x != 0 || q[0].y - q[1].y != 0 || q[0].z - q[1].z != 0) {
		move_curve(c, (CAGD_POINT){ q[0].x - q[1].x, q[0].y - q[1].y, q[0].z - q[1].z });
		update_control_node(c->movingPoint, q[0]);
		draw(c);
	}
}

static void editWeightsLeftUp(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	c->movingPoint = NULL;
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	cagdRedraw();
}

static void moveCurveLeftUp(int x, int y, PVOID userData) {
	Curve* c = (Curve*)userData;
	remove_cp_node(c->movingPoint);
	c->movingPoint = NULL;
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	cagdRedraw();
}

static void editCPLeftUp(int x, int y, PVOID userData)
{	
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, userData);
	cagdRedraw();
}

Control_Point** bspline_get_cps(Curve* c){
	return get_cp_array(c->cp_list);
}
int bspline_get_cps_len(Curve* c) {
	return cp_list_len(c->cp_list);
}
Kn_List* bspline_get_knots(Curve* c) {
	return copy_knot_list(c->kn_list);
}
int bspline_get_order(Curve* c) {
	return c->order;
}

CP_List_Node* getClosestControlPoint(Curve* c, int x, int y) {
	UINT id;
	CP_List_Node* tmp, * res = NULL;
	CAGD_POINT p;
	GLdouble min_dist = 100000000000;
	for (cagdPick(x, y); id = cagdPickNext();) {
		if (cagdGetSegmentType(id) == CAGD_SEGMENT_POINT) {
			if (tmp = get_cp_by_id_list(c->cp_list, id)) {
				GLdouble temp_dist;
				cagdGetSegmentLocation(id, &p);
				int X, Y;
				cagdToWindow(&p, &X, &Y);
				temp_dist = (X - x) * (X - x) + (Y - y) * (Y - y);
				if (temp_dist < min_dist) {
					res = tmp;
				}
			}
		}
	}
	return res;
}

void editWeightsLeftDown(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	CP_List_Node* cp = getClosestControlPoint(c, x, y);
	if (cp) {
		c->movingPoint = cp;
		cagdRegisterCallback(CAGD_MOUSEMOVE, editWeightsMove, (PVOID)c);
		cagdRegisterCallback(CAGD_LBUTTONUP, editWeightsLeftUp, (PVOID)c);
		cagdRedraw();
	}
}

static void editCPLeftDown(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	CP_List_Node* cp = getClosestControlPoint(c, x, y);
	if (cp) {
		c->movingPoint = cp;
		cagdRegisterCallback(CAGD_MOUSEMOVE, editCPMove, (PVOID)c);
		cagdRegisterCallback(CAGD_LBUTTONUP, editCPLeftUp, (PVOID)NULL);
		cagdRedraw();
	}
}

void addCPLeftDown(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	CAGD_POINT q[2] , p;
	if (!get_last(c->cp_list)) {
		cagdToObject(x, y, &q);
		q[1] = q[0];
	}
	else {
		cagdToObjectAtDepth(x, y, get_location(get_cp(get_last(c->cp_list))), &q);
	}
	push_back_cp(c->cp_list,q[1], 1);
	if (!c->is_bspline) {
		c->order = cp_list_len(c->cp_list);
		resetBezier(c);
	}
	else if (c->uniform) {
		if (c->order + cp_list_len(c->cp_list) == kn_list_len(c->kn_list) + 1)
			add_to_uniform(c->kn_list);
	}
	c->movingPoint = get_last(c->cp_list);
	draw(c);
	cagdRegisterCallback(CAGD_MOUSEMOVE, editCPMove, (PVOID)c);
	cagdRegisterCallback(CAGD_LBUTTONUP, editCPLeftUp, (PVOID)NULL);
	cagdRedraw();
}

void moveCurveLeftDown(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	CAGD_POINT q[2], p;
	if (!get_last(c->cp_list)) {
		cagdToObject(x, y, &q);
		q[1] = q[0];
	}
	else {
		cagdToObjectAtDepth(x, y, get_location(get_cp(get_last(c->cp_list))), &q);
	}
	create_faux_node_at(&c->movingPoint, q[1]); 
	cagdRegisterCallback(CAGD_MOUSEMOVE, moveCurveMove, (PVOID)c);
	cagdRegisterCallback(CAGD_LBUTTONUP, moveCurveLeftUp, (PVOID)c);
	cagdRedraw();
}

void fit_derivatives(const Curve* c1, Curve* c2, double deriv_diff) {
	/*
	int J = c2->order - 1;
	double t =  get_kn_value(get_knot_at_index(c2->kn_list, J));
	while (get_kn_value(get_knot_at_index(c2->kn_list, J + 1)) <= t) {
		J++;
	}
	Knot* c2_kn = get_knot_at_index(c2->kn_list, J);
	while (c2_kn->multi < c2->order - 1) {
		knotInsertion(c2, c2_kn->value);
	}
	CP_List_Node* c2_p1 = get_cp_by_index(c2->cp_list, J - (c2->order - 1));
	CAGD_POINT p1, p2, p_res;
	cagdGetSegmentLocation(c2_p1->cp->id, &p1);
	cagdGetSegmentLocation(c2_p1->next->cp->id, &p2);
	if (fabs(deriv_diff) < 0.001)
		return;
	p_res.x = p1.x * deriv_diff + (1 - deriv_diff) * p2.x;
	p_res.y = p1.y * deriv_diff + (1 - deriv_diff) * p2.y;
	p_res.z = 0;
	cagdReusePoint(c2_p1->cp->id, &p_res);
	draw(c1);
	draw(c2);
	*/
}

double rotate_to_end(const Curve* c1, Curve* c2) {
	int c1_points_len = c1->spline.segments[c1->spline.len - 1].len;
	int c2_points_len = c2->spline.segments[0].len;
	CAGD_POINT* c1_points = malloc(sizeof(CAGD_POINT) * c1_points_len);
	CAGD_POINT* c2_points = malloc(sizeof(CAGD_POINT) * c2_points_len);
	cagdGetSegmentLocation(c1->spline.segments[c1->spline.len - 1].line_id, c1_points);
	cagdGetSegmentLocation(c2->spline.segments[0].line_id, c2_points);
	double c1_dx = c1_points[c1_points_len - 1].x - c1_points[c1_points_len - 2].x, c2_dx = c2_points[1].x - c2_points[0].x;
	double c1_dy = c1_points[c1_points_len - 1].y - c1_points[c1_points_len - 2].y, c2_dy = c2_points[1].y - c2_points[0].y;
	double dot_prod = c1_dx * c2_dx + c1_dy * c2_dy;
	double dir = (c1_dx * c2_dy - c1_dy * c2_dx) <= 0 ? 1 : -1;
	double normaliser = sqrt(c1_dx * c1_dx + c1_dy * c1_dy) * sqrt(c2_dx * c2_dx + c2_dy * c2_dy);
	double c1_derivsize = sqrt(c1_dx * c1_dx + c1_dy * c1_dy);
	double c2_derivsize = sqrt(c2_dx * c2_dx + c2_dy * c2_dy);
	dot_prod /= normaliser;
	rotate_curve(c2, c2_points[0], dir * acos(dot_prod));
	free(c1_points);
	free(c2_points);
	//sprintf(logMsg, "derative_1: %lf , derative_1: %lf , diff_prop: %lf\n", c1_derivsize, c2_derivsize, c1_derivsize/ c2_derivsize );
	//LogToOutput(logMsg);
	return c1_derivsize / c2_derivsize;
}

void connect_ends(const Curve* c1, Curve* c2) {
	int c1_points_len = c1->spline.segments[c1->spline.len - 1].len;
	int c2_points_len = c2->spline.segments[0].len;
	CAGD_POINT* c1_points = malloc(sizeof(CAGD_POINT) * c1_points_len);
	CAGD_POINT* c2_points = malloc(sizeof(CAGD_POINT) * c2_points_len);
	cagdGetSegmentLocation(c1->spline.segments[c1->spline.len - 1].line_id, c1_points);
	cagdGetSegmentLocation(c2->spline.segments[0].line_id, c2_points);
	move_curve(c2, (CAGD_POINT){ c1_points[c1_points_len - 1].x - c2_points[0].x, c1_points[c1_points_len - 1].y - c2_points[0].y, c1_points[c1_points_len - 1].z - c2_points[0].z });
	free(c1_points);
	free(c2_points);
}

void convToBspline(Curve* c) {
	if (c->is_bspline == TRUE)
		return;
	kn_list_insert(c->kn_list, 0, c->order);
	kn_list_insert(c->kn_list, 1, c->order);
	if (c->combinations) free(c->combinations);
	c->combinations = NULL;
	if (c->bern_funcs) {
		for (int i = 0; i < cp_list_len(c->cp_list) - 1; i++)
			e2t_freetree(c->bern_funcs[i]);
		free(c->bern_funcs);
		c->bern_funcs = NULL;
	}
	c->is_bspline = TRUE;
	c->uniform = TRUE;
	c->closedfloat = TRUE;
	toggle_cp_list_visiblity(c->cp_list, FALSE, FALSE, FALSE);
}

void connectCurvesLeftUp(int x, int y, PVOID userData)
{
	Bspline_Manager* list = getBsplineInstance();
	void (*connection_function)(Curve*, Curve*) = (void (*)(Curve*, Curve*))userData;
	Curve* c = getBsplineFromClick(x, y);
	if (!c || c->zombie)
		return;
	if (list->connecti1 == NULL) {
		list->connecti1 = c;
		set_polyline_group_color(&list->connecti1->spline, 255, 255, 255);
	}
	else {
		if (list->connecti1 == c) {
			set_polyline_group_color(&list->connecti1, 0, 255, 255);
			list->connecti1 = NULL;
			return;
		}
		list->connecti2 = c;
		convToBspline(list->connecti2);
		connection_function(list->connecti1, list->connecti2);
		set_polyline_group_color(&list->connecti1->spline, 0, 255, 255);
		list->connecti1 = NULL;
		list->connecti2 = NULL;
	}
	cagdRedraw();
}	

static void insertKnotsMove(int x, int y, PVOID userData) {
	Curve* c = (Curve*)userData;
	double value = updateKnotsMove(x, y, (PVOID)c->kn_list);
	remove_from_moving_knot(c->kn_list);
	CAGD_POINT p = evaluateBsplineAtT(c, value,TRUE);
	cagdReusePoint(c->knotInsertCurveIndi, &p);
	cagdSetColor(255, 0, 0);
	Knot* mkn = kn_list_insert(c->kn_list, value, 1);
	set_moving_knot(mkn);
	update_single_KVector(mkn);
	set_moving_knot_color(c->kn_list,255,0,0);
	cagdRedraw();
}


static void insertKnotsLeftUp(int x, int y, PVOID userData) {
	Curve* c = (Curve*)userData;
	double value = get_value(get_moving_knot(c->kn_list));
	remove_from_moving_knot(c->kn_list);
	knotInsertion(c, value);
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	c->uniform = check_uniform_spacing(c->kn_list);
	cagdFreeSegment(c->knotInsertCurveIndi);
	draw(c);
	toggle_cp_list_visiblity(c->cp_list, TRUE, TRUE, c->showWeights);
	cagdRedraw();
}


static void insertKnotsLeftDown(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	double value = knot_vector_click_to_value(c->kn_list, x, y);
	CAGD_POINT p = evaluateBsplineAtT(c, value,TRUE);
	cagdSetColor(255, 0, 0);
	c->knotInsertCurveIndi = cagdAddPoint(&p);
	set_moving_knot(kn_list_insert(c->kn_list,value, 1));
	cagdRegisterCallback(CAGD_MOUSEMOVE, insertKnotsMove, userData);
	cagdRegisterCallback(CAGD_LBUTTONUP, insertKnotsLeftUp, userData);
}

static void updateKnotsMoveBspline(int x, int y, PVOID userData) {
	Curve* c = (Curve*)userData;
	updateKnotsMove(x, y, (PVOID)c->kn_list);
	draw(c);
	cagdRedraw();
}

static void updateKnotsRightUp(int x, int y, PVOID userData) {
	Curve* c = getBsplineInstance()->last_clicked->c;
	if (!c)
		return;
	HMENU hMenu = getBsplineInstance()->myKnotPopup;
	Knot* knot = get_knot_from_indicator(c->kn_list, x, y);
	if (knot) {
		switch (cagdPostMenu(hMenu, x, y)) {
		case BS_ADDKN:
			add_knot(knot);
			update_single_KVector(knot);
			break;
		case BS_REMOVEKN:
			if(!kn_list_remove(knot))
				update_single_KVector(knot);
			break;
		}
		draw(c);
	}
	else {
		myClickRightUp(x, y, (PVOID)NULL);
	}
}

static void updateKnotsLeftDown(int x, int y, PVOID userData)
{
	Curve* c = (Curve*)userData;
	Knot* knot = get_knot_from_indicator(c->kn_list, x, y);
	if (knot) {
		set_moving_knot(knot);
		cagdRegisterCallback(CAGD_MOUSEMOVE, updateKnotsMoveBspline, (PVOID)c);
		cagdRegisterCallback(CAGD_LBUTTONUP, updateKnotsLeftUp, (PVOID)c->kn_list);
	}
	else {
		cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
		cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
	}
}


void knotInsertion(Curve* c, double value) {
	int l_lower, l_upper, l;
	l_lower = kn_list_lower_index(c->kn_list, value);
	l_upper = kn_list_upper_index(c->kn_list, value);
	l = l_lower;
	if (l - c->order < 0 || l > kn_list_len(c->kn_list) - c->order)
	{
		l = l_upper;
		if (l - c->order < 0 || l > kn_list_len(c->kn_list) - c->order)
			return;
	}
	update_single_KVector(kn_list_insert(c->kn_list, value, 1));
	CAGD_POINT i_0, i_1;
	CP_List_Node* curr_cp = get_cp_by_index(c->cp_list, l - c->order);
	CP_List_Node* prev_cp = get_prev(curr_cp);
	double w_0, w_1 = get_weight(get_cp(curr_cp));
	if (prev_cp) {
		i_0 = get_location(get_cp(prev_cp));// i-1
		w_0 = get_weight(get_cp(prev_cp));
	}
	else {
		i_0 = i_1;
		w_0 = w_1;
	}
	for (int i = l - c->order; i < l; i++) {
		double t_i = get_value(get_knot_at_index(c->kn_list, i));
		double t_k = get_value(get_knot_at_index(c->kn_list, i + c->order));
		i_1 = get_location(get_cp(curr_cp)); // i
		w_1 = get_weight(get_cp(curr_cp));
		double alpha = (value - t_i) / (t_k - t_i); 
		double new_weight = alpha * w_1 + (1 - alpha) * w_0;
		CAGD_POINT upd = scaler(cagdPointAdd(scaler(i_1, w_1*alpha), scaler(i_0, w_0 * (1 - alpha))),1/new_weight);
		//CAGD_POINT upd = { ((((value - t_i) * i_1.x) + ((t_k - value) * i_0.x)) /(t_k - t_i)),
		//	((((value - t_i) * i_1.y) + ((t_k - value)  * i_0.y))/ (t_k - t_i)),
		//	((((value - t_i) * i_1.z) + ((t_k - value)  * i_0.z)) /(t_k - t_i)) };
		//upate_weight(get_cp(curr_cp), new_weight);
	//CAGD_POINT upd = { (((value - t_i) *w1* i_1.x) + ((t_k - value) * w2* i_0.x)) / ((w1*w2)*(t_k - t_i)),
	//						(((value - t_i) * w1 * i_1.y) + ((t_k - value)* w2 * i_0.y)) / ((w1 * w2) * (t_k - t_i)),
	//						((((value - t_i) * w1 * i_1.z) + ((t_k - value) * w2 * i_0.z)) / ((w1 * w2) * (t_k - t_i))) };
		update_control_node(curr_cp, upd);
		update_weight(get_cp(curr_cp), 1);
		i_0 = i_1;
		w_0 = w_1;
		prev_cp = curr_cp;
		curr_cp = get_next(curr_cp);
	}
	push_next_to_cp(c->cp_list, prev_cp, cp_create(i_1, 1));
}

Curve* create_empty_bez() {
	Curve* new_c = SAFE_MALLOC(Curve, 1);
	init_curve(new_c);
	new_c->is_bspline = FALSE;
	return new_c;
}

void newBezierHandler(BOOL activate) {
	Bspline_Manager* list = getBsplineInstance();
	toggleShowOrHideControl(list->last_clicked);
	if (activate) {
		Curve* new_c = create_empty_bez();
		cagdRegisterCallback(CAGD_LBUTTONDOWN, addCPLeftDown, (PVOID)new_c);
		list->last_clicked = push_back_curve(new_c);
	}
	else {
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
		if (!list->tail->c->spline.segments) {
			remove_curve_node(list->tail);
			cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
		}
	}
}
Curve* create_empty_bspline() {
	Curve* new_c = SAFE_MALLOC(Curve, 1);
	init_curve(new_c);
	new_c->is_bspline = TRUE;
	new_c->order = 3;
	remake_as_closed_uni(&new_c->kn_list, cp_list_len(new_c->cp_list), new_c->order);
	new_c->uniform = TRUE;
	new_c->closedfloat = TRUE;
	return new_c;
}

void newBsplineHandler(BOOL activate) {
	Bspline_Manager* list = getBsplineInstance();
	toggleShowOrHideControl(list->last_clicked);
	if (activate) {
		Curve* new_c = create_empty_bspline();
		list->last_clicked  = push_back_curve(new_c);
		cagdRegisterCallback(CAGD_LBUTTONDOWN, addCPLeftDown, (PVOID)new_c);
	}
	else {
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
		if (!list->tail->c->spline.segments) {
			remove_curve_node(list->tail);
			cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
		}
	}
}

void connectHandler(enum bezFlags flag, BOOL activate) {
	Bspline_Manager* list = getBsplineInstance();
	if (activate) {
		cagdRegisterCallback(CAGD_LBUTTONUP, connectCurvesLeftUp, (PVOID)flag);
	}
	else {
		if (list->connecti1) for (int i = 0; i < list->connecti1->spline.len; i++) {
			cagdSetSegmentColor(list->connecti1->spline.segments[i].line_id, 0, 255, 255);
		}
		list->connecti1 = NULL;
		list->connecti2 = NULL;
		cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
	}
}

void bezHandler(enum bezFlags flag) {
	unsigned int* bezBitMap = &getBsplineInstance()->bezBitMap;
	BOOL toActivate = !((*bezBitMap & (1 << flag)));
	if (*bezBitMap & (1 << FLAG_BEZ)) {
		newBezierHandler(FALSE);
		*bezBitMap ^= (1 << FLAG_BEZ);
	}
	if (*bezBitMap & (1 << FLAG_BSP)) {
		newBsplineHandler(FALSE);
		*bezBitMap ^= (1 << FLAG_BSP);
	}
	if (*bezBitMap & (1 << FLAG_C0)) {
		connectHandler(FLAG_C0, FALSE);
		*bezBitMap ^= (1 << FLAG_C0);
	}
	if (*bezBitMap & (1 << FLAG_C1)) {
		connectHandler(FLAG_C1, FALSE);
		*bezBitMap ^= (1 << FLAG_C1);
	}
	if (*bezBitMap & (1 << FLAG_G1)) {
		connectHandler(FLAG_G1, FALSE);
		*bezBitMap ^= (1 << FLAG_G1);
	}
	if (toActivate) {
		switch (flag) {
		case FLAG_BEZ:
			newBezierHandler(TRUE);
			break;
		case FLAG_BSP:
			newBsplineHandler(TRUE);
			break;
		case FLAG_C0:
			connectHandler(FLAG_C0, TRUE);
			break;
		case FLAG_C1:
			connectHandler(FLAG_C1, TRUE);
			break;
		case FLAG_G1:
			connectHandler(FLAG_G1, TRUE);
			break;
		}
		*bezBitMap |= (1 << flag);
	}
}



void bsplineCommandCenter(int id, int unUsed, PVOID userData)
{
	Bspline_Manager* manager = getBsplineInstance();
	cagdRegisterCallback(CAGD_LBUTTONUP, defaultBsplineLeftClick, (PVOID)NULL);
	HMENU hMenu = (HMENU)userData;
	switch (id) {
	case BS_BEZIER:
		bezHandler(FLAG_BEZ);
		break;
	case BS_BSPLINE:
		bezHandler(FLAG_BSP);
		break;
	case BS_LOAD:
		bezHandler(FLAG_NA);
		openFileName.hwndOwner = auxGetHWND();
		openFileName.lpstrTitle = "Load File";
		if (GetOpenFileName(&openFileName)) {
			loadCurve((int)openFileName.lpstrFile, 0, NULL);
		}
		break;
	case BS_CONC0:
		bezHandler(FLAG_C0);
		break;

	case BS_CONC1:
		bezHandler(FLAG_C1);
		break;

	case BS_CONG1:
		bezHandler(FLAG_G1);
		break;
	case BS_FIN:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_F),
			cagdGetWindow(),
			(DLGPROC)myDialogProc))
			if (sscanf(myBuffer, "%d", &finesse) != 1 || finesse <= 2) {
				myMessage("Change finesse", "Bad finesse!", MB_ICONERROR);
				return;
			}
		break;
	}

	static state = CAGD_USER;
	CheckMenuItem(hMenu, state, MF_UNCHECKED);
	CheckMenuItem(hMenu, state = id, MF_CHECKED);
	CheckMenuItem(hMenu, BS_BEZIER, (manager->bezBitMap & (1 << FLAG_BEZ)) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BS_BSPLINE, (manager->bezBitMap & (1 << FLAG_BSP)) ? MF_CHECKED : MF_UNCHECKED);
	cagdRedraw();
}

void addBsplinesMenus(HMENU hMenu) {
	Bspline_Manager* manager = getBsplineInstance();
	manager->myPopup = CreatePopupMenu();
	AppendMenu(manager->myPopup, MF_STRING, BS_Move, "Move");
	AppendMenu(manager->myPopup, MF_STRING, BS_ADD, "Add");
	AppendMenu(manager->myPopup, MF_STRING, BS_REMOVECP, "Remove Control Point");
	AppendMenu(manager->myPopup, MF_STRING, BS_WEIGHT, "Edit Weights");
	AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(manager->myPopup, MF_STRING, BS_REMOVEC, "Remove Curve");
	AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
	AppendMenu(manager->myPopup, MF_STRING, BS_KNOTS, "Edit Knots");
	AppendMenu(manager->myPopup, MF_STRING, BS_INKNOT, "Insert Knot");
	AppendMenu(manager->myPopup, MF_STRING, BS_DEG, "Edit Degree");
	AppendMenu(manager->myPopup, MF_STRING, BS_OEND, "Uniform Open");
	AppendMenu(manager->myPopup, MF_STRING, BS_CEND, "Uniform Float");

	manager->myKnotPopup = CreatePopupMenu();
	AppendMenu(manager->myKnotPopup, MF_STRING, BS_ADDKN, "Add Knot");
	AppendMenu(manager->myKnotPopup, MF_STRING, BS_REMOVEKN, "Remove Knot");

	AppendMenu(hMenu, MF_STRING, BS_BEZIER, "Add Bezier");
	AppendMenu(hMenu, MF_STRING, BS_BSPLINE, "Add Bspline");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, BS_LOAD, "Load Control Points");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, BS_CONC0, "Connect C0");
	AppendMenu(hMenu, MF_STRING, BS_CONC1, "Connect C1");
	AppendMenu(hMenu, MF_STRING, BS_CONG1, "Connect G1");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, BS_FIN, "Finesse");
	cagdAppendMenu(hMenu, "Splines");
}
