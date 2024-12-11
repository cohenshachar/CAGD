#include "Control_Point.h"

#define KNOT_X_MARGIN 20
#define KNOT_Y_MARGIN 7
#define RADIUS_NORMALIZER 0.01
#define RADIUS_SENSATIVITY 0.45


struct Control_Point{
	UINT id;
	UINT weight_circle;
	double weight;
};

struct  CP_List_Node {
	Control_Point* cp;
	UINT grid_line_ids[2];
	CP_List_Node* next;
	CP_List_Node* prev;
	CP_List* parent;
};

struct  CP_List {
	int length;
	CP_List_Node* head;
	CP_List_Node* tail;
};

typedef struct{
	UINT vector_line;
	CAGD_POINT vec_location[2];
	CAGD_POINT vec_perp;
	CAGD_POINT vec_locaVec;
	double edge_values[2];
	double length;
	Kn_List* parent;
}Knot_Vector_Rep;

struct Knot {
	double value;
	int multi;
	int indi_num;
	UINT* indicators;
	Knot* next;
	Knot* prev;
	Kn_List* parent;
};

struct Kn_List {
	int length;
	Knot_Vector_Rep vector_line;
	Knot* moving_knot;
	Knot* head;
	Knot* tail;
};

static void get_knot_VL_location(CAGD_POINT* p) {
	RECT clientRect;
	HWND window = cagdGetWindow();
	GetWindowRect(window, &clientRect);
	int windowWidth = clientRect.right - clientRect.left;
	CAGD_POINT q[2];
	cagdToObject(KNOT_X_MARGIN, KNOT_Y_MARGIN, p);
	cagdToObject(windowWidth - (4 * KNOT_X_MARGIN), KNOT_Y_MARGIN, q);
	p[1] = q[0];
	cagdToObject(KNOT_X_MARGIN, 2*KNOT_Y_MARGIN, q);
	double dx = p[0].x - q[0].x;
	double dy = p[0].y - q[0].y;
	double dz = p[0].z - q[0].z;
	double length = sqrt(dx * dx + dy * dy + dz * dz);
	p[2] = (CAGD_POINT){ dx / length , dy / length, dz / length };
}

static void _draw_circle(UINT* id, double r, double x, double y, double z) {
	e2t_expr_node* weight_cric_func[3] = { e2t_expr2tree("r*sin(t)*cos(20*t)+x"), e2t_expr2tree("r*sin(t)*sin(20*t)+y"),e2t_expr2tree("r*cos(t) + z") };
	double circle_domain[2] = { 0, 6.3 };
	long long circle_samples;
	e2t_setparamvalue(r, E2T_PARAM_R);
	e2t_setparamvalue(x, E2T_PARAM_X);
	e2t_setparamvalue(y, E2T_PARAM_Y);
	e2t_setparamvalue(z, E2T_PARAM_Z);
	CAGD_POINT* circle = createCurvePointCollection(circle_domain, weight_cric_func, FALSE, NULL, 100, &circle_samples);
	*id = cagdAddPolyline(circle, circle_samples);
	free(circle);
	e2t_freetree(weight_cric_func[0]);
	e2t_freetree(weight_cric_func[1]);
	e2t_freetree(weight_cric_func[2]);
}

static void draw_weight_circle(Control_Point* cp) {
	cagdSetColor(0, 255, 0);
	CAGD_POINT p = get_location(cp);
	cagdFreeSegment(cp->weight_circle);
	_draw_circle(&(cp->weight_circle), cp->weight / 50, p.x, p.y, p.z);
	cagdShowSegment(cp->weight_circle);

}

UINT get_id(Control_Point* cp) {
	return cp->id;
}

CAGD_POINT get_location(Control_Point* cp) {
	CAGD_POINT location;
	cagdGetSegmentLocation(cp->id, &location);
	return location;
}

double get_weight(Control_Point* cp) {
	return cp->weight;
}

void update_weight(struct Control_Point* cp, double weight) {
	cp->weight = weight > 0.01 ? weight : 0.01;
	BOOL visible = cagdIsSegmentVisible(cp->weight_circle);
	draw_weight_circle(cp);
	visible ? cagdShowSegment(cp->weight_circle) : cagdHideSegment(cp->weight_circle);
}
void update_weight_from_window(struct Control_Point* cp, int x, int y) {
	CAGD_POINT q[2], p = get_location(cp);
	cagdToObjectAtDepth(x, y, p, &q);
	double weight = 50 * sqrt((p.x - q[1].x) * (p.x - q[1].x) + (p.y - q[1].y) * (p.y - q[1].y) + (p.z - q[1].z) * (p.z - q[1].z));
	update_weight(cp, weight);
}

Control_Point* cp_create(CAGD_POINT p, double w) {
	Control_Point* cpoint = SAFE_MALLOC(Control_Point, 1);
	cagdSetColor(0, 255, 255);
	cpoint->id = cagdAddPoint(&p);
	cpoint->weight = w;
	cpoint->weight_circle = NA;
	draw_weight_circle(cpoint);
	cagdHideSegment(cpoint->weight_circle);
	return cpoint;
}

void* cp_delete(Control_Point* cp) {
	if (!cp)
		return NULL;
	cagdFreeSegment(cp->weight_circle);
	cagdFreeSegment(cp->id);
	free(cp);
	return NULL;
}

void update_cp(Control_Point* cp, CAGD_POINT where) {
	cagdReusePoint(cp->id, &where);
	BOOL visible = cagdIsSegmentVisible(cp->weight_circle);
	draw_weight_circle(cp);
	visible ? cagdShowSegment(cp->weight_circle) : cagdHideSegment(cp->weight_circle);
}

Control_Point* cp_add(Control_Point* cp1, Control_Point* cp2) {
	return cp_create(cagdPointAdd(get_location(cp1), get_location(cp2)), cp1->weight* cp2->weight);
}

Control_Point* get_cp(CP_List_Node* node) {
	return node->cp;
}

CP_List_Node* get_next(CP_List_Node* node) {
	return node->next;
}

CP_List_Node* get_prev(CP_List_Node* node) {
	return node->prev;
}

CP_List* get_parent(CP_List_Node* node) {
	return node->parent;
}

void cp_list_node_init(CP_List_Node* cp_node) {
	cp_node->cp = NULL;
	cp_node->next = NULL;
	cp_node->prev = NULL;
	cp_node->parent = NULL;
	cp_node->grid_line_ids[0] = NA;
	cp_node->grid_line_ids[1] = NA;
}

void update_control_node(CP_List_Node* cp_node, CAGD_POINT where) {
	update_cp(cp_node->cp, where);
	CAGD_POINT p[2];
	if (cp_node->next) {
		cagdGetSegmentLocation(cp_node->grid_line_ids[1], p);
		p[0] = where;
		cagdReusePolyline(cp_node->grid_line_ids[1], p, 2);
	}
	if (cp_node->prev) {
		cagdGetSegmentLocation(cp_node->grid_line_ids[0], p);
		p[1] = where;
		cagdReusePolyline(cp_node->grid_line_ids[0], p, 2);
	}
}

void update_control_node_2(struct CP_List_Node* cp_node, int x, int y) {
	CAGD_POINT q[2], p = get_location(cp_node->cp);
	cagdToObjectAtDepth(x, y, p, &q);
	update_control_node(cp_node, q[1]);
}

BOOL remove_cp_node(CP_List_Node* cp_node) {
	if (!cp_node) return FALSE;
	cp_node->cp = cp_delete(cp_node->cp);
	if (cp_node->parent == NULL) {
		free(cp_node);
		return TRUE;
	}
	if (!cp_node->prev && !cp_node->next) {
		cp_node->parent->head = NULL;
		cp_node->parent->tail = NULL;
	}
	else if (!cp_node->next) {
		cagdFreeSegment(cp_node->grid_line_ids[0]);
		cp_node->prev->next = NULL;
		cp_node->prev->grid_line_ids[1] = NA;
		cp_node->parent->tail = cp_node->prev;
	}
	else if (!cp_node->prev) {
		cagdFreeSegment(cp_node->grid_line_ids[1]);
		cp_node->next->prev = NULL;
		cp_node->next->grid_line_ids[0] = NA;
		cp_node->parent->head = cp_node->next;
	}
	else {
		CAGD_POINT q[2] = { get_location(cp_node->prev->cp),get_location(cp_node->next->cp) };
		cagdFreeSegment(cp_node->grid_line_ids[0]);
		cagdFreeSegment(cp_node->grid_line_ids[1]);
		cp_node->next->prev = cp_node->prev;
		cp_node->prev->next = cp_node->next;
		cagdSetColor(255, 255, 0);
		cp_node->prev->grid_line_ids[1] = cp_node->next->grid_line_ids[0] = cagdAddPolyline(q, 2);
	}
	cp_node->parent->length--;
	free(cp_node);
	return TRUE;
}

void create_faux_node_at(CP_List_Node** cp_node, CAGD_POINT where) {
	*cp_node = SAFE_MALLOC(CP_List_Node, 1);
	cp_list_node_init(*cp_node);
	(*cp_node)->cp = SAFE_MALLOC(Control_Point, 1);
	(*cp_node)->cp->weight = 0;
	(*cp_node)->cp->weight_circle = NA;
	cagdHideSegment((*cp_node)->cp->id = cagdAddPoint(&where));
}

BOOL is_empty(CP_List* list) {
	return list->length == 0;
}
CP_List_Node* get_first(CP_List* list) {
	return list->head;
}
CP_List_Node* get_last(CP_List* list) {
	return list->tail;
}
int cp_list_len(struct CP_List* list) {
	return list->length;
}

void cp_list_init(CP_List** list) {
	*list = SAFE_MALLOC(CP_List, 1);
	if (*list) {
		(*list)->head = NULL;
		(*list)->tail = NULL;
		(*list)->length = 0;
	}
}

void push_back_cp(struct CP_List* list, CAGD_POINT p, double w) {
	struct CP_List_Node* cp_node = SAFE_MALLOC(CP_List_Node, 1);
	cp_list_node_init(cp_node);
	cp_node->cp = cp_create(p, w);
	cp_node->parent = list;
	if (!list->head)
		list->head = list->tail = cp_node;
	else {
		CAGD_POINT q[2] = { get_location(list->tail->cp) , p };
		cagdSetColor(255, 255, 0);
		list->tail->grid_line_ids[1] = cp_node->grid_line_ids[0] = cagdAddPolyline(q, 2);
		cp_node->prev = list->tail;
		list->tail->next = cp_node;
		list->tail = cp_node;
	}
	list->length++;
}

void push_next_to_cp(struct CP_List* list, struct CP_List_Node* prev, struct Control_Point* cp) {
	struct CP_List_Node* cpoint_node = SAFE_MALLOC(CP_List_Node, 1);
	cp_list_node_init(cpoint_node);
	BYTE r, g, b;
	cpoint_node->cp = cp;
	cpoint_node->parent = list;
	cagdGetSegmentColor(prev->grid_line_ids[0], &r, &g, &b);
	BOOL visible = cagdIsSegmentVisible(prev->grid_line_ids[0]);
	cpoint_node->next = prev->next;
	cpoint_node->prev = prev;
	if (prev->next) {
		cagdFreeSegment(prev->grid_line_ids[1]);
		cagdFreeSegment(prev->next->grid_line_ids[0]);
	}
	CAGD_POINT p[2] = {get_location(prev->cp),get_location(cp)};
	cagdSetColor(r, g, b);
	visible ? cpoint_node->grid_line_ids[0] = prev->grid_line_ids[1] = cagdAddPolyline(p, 2) : cagdHideSegment(cpoint_node->grid_line_ids[0] = prev->grid_line_ids[1] = cagdAddPolyline(p, 2));
	if (prev->next) {
		p[0] = p[1];
		p[1] = get_location(prev->next->cp);
		visible ? cpoint_node->grid_line_ids[1] = prev->next->grid_line_ids[0] = cagdAddPolyline(p, 2) : cagdHideSegment(cpoint_node->grid_line_ids[1] = prev->next->grid_line_ids[0] = cagdAddPolyline(p, 2));
		prev->next->prev = cpoint_node;
	}
	draw_weight_circle(cpoint_node->cp);
	cagdHideSegment(cpoint_node->cp->weight_circle);
	prev->next = cpoint_node;
	list->length++;
}

Control_Point** get_cp_array(CP_List* list) {
	return get_cp_array_range(list, 0, list->length);
}

Control_Point** get_cp_array_range(CP_List* list, int start_i, int range) {
	if (start_i < 0 || range < 1 || range + start_i > list->length)
		return NULL;
	CP_List_Node* curr = get_cp_by_index(list, start_i);
	Control_Point** res = SAFE_MALLOC(Control_Point*, range);
	for (int i = 0; i < range; i++) {
		if (!curr) {
			free(res);
			return NULL;
		}
		res[i] = curr->cp;
		curr = curr->next;
	}
	return res;
}

struct CP_List_Node* get_cp_by_id_list(struct CP_List* list, UINT id) {
	struct CP_List_Node* curr = list->head;
	for (int i = 0; curr; curr = curr->next) if (curr->cp->id == id) return curr;
	return NULL;
}

struct CP_List_Node* get_cp_by_index(struct CP_List* list, int index) {
	struct CP_List_Node* curr = list->head;
	for (int i = 0; curr; curr = curr->next, i++) if (i == index) return curr;
	return NULL;
}

BOOL remove_cp(CP_List* list, Control_Point* cp) {
	CP_List_Node* to_delete = get_cp_by_id_list(list, cp->id);
	return remove_cp_node(to_delete);
}

void* delete_cp_list(CP_List* list) {
	while (list->head)
		remove_cp_node(list->head);
	free(list);
	return NULL;
}

void transform_list(struct CP_List* list, CAGD_POINT delta) {
	CP_List_Node* curr = list->head;
	while (curr) {
		update_control_node(curr, cagdPointAdd(get_location(curr->cp), delta));
		curr = curr->next;
	}
}

void rotate_list(struct CP_List* list, CAGD_POINT center, double degree) {
	CP_List_Node* curr = list->head;
	while (curr) {
		CAGD_POINT p = get_location(curr->cp);
		double x = center.x + (p.x - center.x) * cos(degree) - (p.y - center.y) * sin(degree);
		double y = center.y + (p.x - center.x) * sin(degree) + (p.y - center.y) * cos(degree);
		p.x = x;
		p.y = y;
		update_control_node(curr, p);
		curr = curr->next;
	}
}

void toggle_cp_list_visiblity(CP_List* list, BOOL cp_vis, BOOL cp_poly_vis, BOOL cp_weight_vis) {
	CP_List_Node* curr = list->head;
	while (curr) {
		cp_vis ? cagdShowSegment(curr->cp->id) : cagdHideSegment(curr->cp->id);
		cp_poly_vis ? cagdShowSegment(curr->grid_line_ids[1]) : cagdHideSegment(curr->grid_line_ids[1]);
		cp_weight_vis ? cagdShowSegment(curr->cp->weight_circle) : cagdHideSegment(curr->cp->weight_circle);
		curr = curr->next;
	}
}

double get_value(Knot* knot) {
	return knot->value;
}

void add_knot(Knot* knot) {
	knot->multi++; 
	knot->parent->length++;
}

int get_multi(Knot* knot) {
	return knot->multi;
}
kn_vec_rep_init(Knot_Vector_Rep* kn_vec) {
	kn_vec->edge_values[0] = kn_vec->edge_values[1] = 0;
	kn_vec->vector_line = NA;
	kn_vec->vec_locaVec = kn_vec->vec_perp = kn_vec->vec_location[0] = kn_vec->vec_location[1] = (CAGD_POINT){ 0.0,0.0,0.0 };
	kn_vec->parent = NULL;
	kn_vec->length = 0;
}

void kn_init(Knot* kn) {
	kn->indicators = NULL;
	kn->indi_num = 0;
	kn->multi = 0;
	kn->value = 0;
	kn->next = NULL;
	kn->prev = NULL;
	kn->parent = NULL;
}

void kn_list_init(Kn_List** list) {
	*list = SAFE_MALLOC(Kn_List, 1); 
	if (*list != NULL) {
		(*list)->head = NULL;
		(*list)->tail = NULL;
		(*list)->length = 0;
		(*list)->moving_knot = NULL;
		kn_vec_rep_init(&(*list)->vector_line);
		(*list)->vector_line.parent = (*list);
	}
}

int kn_list_len(Kn_List* list) {
	return list->length;
}

int kn_list_upper_index(Kn_List* list, double value) {
	Knot* curr = list->head;
	int index = 0;
	while (curr && value >= curr->value) {
		index += curr->multi;
		curr = curr->next;
	}
	return index;
}

int kn_list_lower_index(struct Kn_List* list, double value) {
	Knot* curr = list->head;
	int index = 0;
	while (curr && value > curr->value) {
		index += curr->multi;
		curr = curr->next;
	}
	return index;
}

 Knot* kn_list_upper_node(Kn_List* list, double value) {
	Knot* curr = list->head;
	while (curr && value >= curr->value) {
		if (curr->next)
			curr = curr->next;
		else
			break;
	}
	return curr;
}

Knot* kn_list_lower_node(Kn_List* list, double value) {
	Knot* curr = list->head;
	if (!curr || curr->value >= value)
		return NULL;
	while (curr->next && value > curr->next->value) {
		curr = curr->next;
	}
	return curr;
}

Knot* kn_list_insert(Kn_List* list, double value, int multi) {
	Knot* prev = kn_list_lower_node(list, value);
	Knot* res;
	if (!prev || !prev->next || prev->next->value != value) {
		if (list->head && value == list->head->value) {
			list->head->multi+= multi;
			list->length += multi;
			return list->head;
		}
		Knot* knot = SAFE_MALLOC(Knot, 1); 
		kn_init(knot);
		knot->value = value;
		knot->multi = multi;
		knot->parent = list;
		if (!prev) {
			if (list->head) {
				knot->next = list->head;
				list->head->prev = knot;
				list->head = knot;
			}
			else {
				list->head = list->tail = knot;
			}
		}
		else if (!prev->next) {
			list->tail->next = knot;
			knot->prev = list->tail;
			list->tail = knot;
		}
		else if (prev->next->value != value) {
			prev->next->prev = knot;
			knot->next = prev->next;
			knot->prev = prev;
			prev->next = knot;
		}
		res = knot;
	}
	else {
		prev->next->multi+= multi;
		res = prev->next;
	}
	list->length += multi;
	return res;
}

static void kn_reset_indicators(Knot* knot) {
	for (int i = 0; i < knot->indi_num; i++)
		cagdFreeSegment(knot->indicators[i]);
	if(knot->indicators) free(knot->indicators);
	knot->indicators = NULL;
	knot->indi_num = 0;
}

BOOL kn_list_remove(Knot* kn) {
	kn_reset_indicators(kn);
	kn->multi--;
	kn->parent->length--;
	if (kn->multi == 0) {
		if (kn == kn->parent->moving_knot)
			kn->parent->moving_knot = NULL;
		kn_list_erase(kn);
		return TRUE;
	}
	return FALSE;
}

void kn_list_erase(Knot* kn) {
	kn_reset_indicators(kn);
	int mul = kn->multi;
	if (kn == kn->parent->head) {
		kn->parent->head = kn->next;
		if (!kn->next)
			kn->parent->tail = NULL;
	}
	else if (kn == kn->parent->tail) {
		kn->prev->next = NULL;
		kn->parent->tail = kn->prev;
	}
	else {
		kn->next->prev = kn->prev;
		kn->prev->next = kn->next;
	}
	kn->parent->length -= mul;
	free(kn);
}

Kn_List* copy_knot_list(Kn_List* list) {
	Kn_List* res;
	kn_list_init(&res);
	Knot* curr = list->head;
	while (curr) {
		kn_list_insert(res, curr->value, curr->multi);
		curr = curr->next;
	}
	return res;
}
void reset_kn_vec(Knot_Vector_Rep* kn_vec) {
	if (kn_vec->vector_line != NA) cagdFreeSegment(kn_vec->vector_line);
	kn_vec->vector_line = NA;
	kn_vec_rep_init(kn_vec);
}

void* delete_kn_list(Kn_List* list) {
	while (list->head)
		kn_list_erase(list->head);
	reset_kn_vec(&list->vector_line);
	free(list);
	return NULL;
}

Knot* get_knot_at_index(struct Kn_List* list, int index) {
	Knot* curr = list->head;
	if (!curr || index >= list->length)
		return;
	int i = 0;
	while (i < index) {
		if (i + curr->multi > index)
			break;
		i += curr->multi;
		curr = curr->next;
	}
	return curr;
}
double* get_knot_array(Kn_List* list) {
	return get_knot_range(list, 0, list->length);
}

double* get_knot_range(struct Kn_List* list, int start_i, int range) {
	if (start_i < 0 || range < 1 || range + start_i > list->length)
		return NULL;
	Knot* curr = list->head;
	int i = 0, mul_used = 0;
	while (i < start_i) {
		if (i + curr->multi > start_i) {
			mul_used = start_i - i;
			break;
		}
		i += curr->multi;
		curr = curr->next;
	}
	int j = 0;
	double* res = malloc(sizeof(double) * range);
	while (j < range) {
		if (mul_used == curr->multi) {
			curr = curr->next;
			mul_used = 1;
		}
		else {
			mul_used++;
		}
		double debug = curr->value;
		res[j] = curr->value;
		j++;
	}
	return res;
}


static CAGD_POINT get_knot_pos(Knot_Vector_Rep* kn_vec, double value) {
	// Calculate the interpolation factor t
	double t = (value - kn_vec->edge_values[0]) / (kn_vec->edge_values[1] - kn_vec->edge_values[0]);

	// Interpolate the point on the line
	return cagdPointAdd(scaler(cagdPointSub(kn_vec->vec_location[1], kn_vec->vec_location[0]),t), kn_vec->vec_location[0]);
}

void set_moving_indicator_moving(Knot* movingKnot) {
	if (movingKnot)
		cagdSetSegmentColor(movingKnot->indicators[0], 255, 0, 0);
}
void set_moving_indicator_inplace(Knot* movingKnot) {
	if (movingKnot)
		cagdSetSegmentColor(movingKnot->indicators[0], 255, 255, 255);
	movingKnot = NULL;
}
static double calculate_indicator_radius() {
	CAGD_POINT p[3];
	get_knot_VL_location(p);
	return norm(cagdPointSub(p[1], p[0])) * RADIUS_NORMALIZER;
}
BOOL update_kn_vec_rep(Knot_Vector_Rep* kn_vec) {
	CAGD_POINT p[3];
	get_knot_VL_location(p);
	if (p[0].x != kn_vec->vec_location[0].x || p[0].y != kn_vec->vec_location[0].y || p[0].z != kn_vec->vec_location[0].z
		|| p[1].x != kn_vec->vec_location[1].x || p[1].y != kn_vec->vec_location[1].y || p[1].z != kn_vec->vec_location[1].z) {
		if (kn_vec->vector_line != NA)
			cagdReusePolyline(kn_vec->vector_line,p, 2);
		else
			kn_vec->vector_line = cagdAddPolyline(p, 2);
		kn_vec->edge_values[0] = kn_vec->parent->head->value;
		kn_vec->edge_values[1] = kn_vec->parent->tail->value;
		kn_vec->vec_location[0] = p[0];
		kn_vec->vec_location[1] = p[1];
		kn_vec->vec_locaVec = cagdPointSub(p[1], p[0]);
		kn_vec->vec_perp = p[2];
		kn_vec->length = norm(kn_vec->vec_locaVec);
		return TRUE;
	}
	return FALSE;
}

void draw_KV(Kn_List* list) {
	update_kn_vec_rep(&list->vector_line);
	cagdShowSegment(list->vector_line.vector_line);
	Knot* curr = list->head;
	double r = calculate_indicator_radius();
	while (curr) {
		CAGD_POINT pos = get_knot_pos(&list->vector_line, curr->value);
		create_knot_indicators(list->vector_line.vec_perp,curr, pos,r);
		curr = curr->next;
	}
	cagdRedraw();
}

void update_single_KVector(Knot* kn) {
	CAGD_POINT pos = get_knot_pos(&kn->parent->vector_line, kn->value);
	double r = calculate_indicator_radius();
	create_knot_indicators(kn->parent->vector_line.vec_perp,kn,pos,r);
}

void erase_KV(Kn_List* list) {
	if (list->vector_line.vector_line != NA) cagdHideSegment(list->vector_line.vector_line);
	Knot* curr = list->head;
	while (curr) {
		kn_reset_indicators(curr);
		curr = curr->next;
	}
}

void create_knot_indicators(CAGD_POINT axis, Knot* kn, CAGD_POINT pos, double r) {
	kn_reset_indicators(kn);
	kn->indicators = SAFE_MALLOC(UINT, kn->multi);
	kn->indi_num = kn->multi;
	for (int i = 0; i < kn->multi; i++) {
		cagdSetColor(255, 255, 255);
		double axis_dr = (r + (2 * r * i));
		_draw_circle(&(kn->indicators[i]), r, pos.x - axis.x* axis_dr, pos.y -axis.y * axis_dr, pos.z -axis.z * axis_dr);
	}
}
double calculate_radius_epsilon(Knot_Vector_Rep* knot_vec) {
	return (knot_vec->edge_values[1]- knot_vec->edge_values[0]) * RADIUS_NORMALIZER;
}
Knot* get_knot_from_indicator(Kn_List* list, int x, int y) {
	double click_val = knot_vector_click_to_value(list, x, y);
	Knot* prev = kn_list_lower_node(list, click_val);
	double epsilon = calculate_radius_epsilon(&list->vector_line)*RADIUS_SENSATIVITY;
	if (prev) {
		if (fabs(click_val-prev->value) < epsilon)
			return prev;
		else if (prev->next) {
			if (fabs(click_val-prev->next->value) < epsilon)
				return prev->next;
		}
	}
	else {
		if (list->head && fabs(click_val - list->head->value) < epsilon)
			return list->head;
	}
	return NULL;
}

UINT* get_knot_indicator_arr(Kn_List* list) {
	UINT* res = (UINT*)malloc(sizeof(UINT) * list->length);
	Knot* curr = list->head;
	int i = 0;
	int j = 0;
	while (curr) {
		while (j < curr->indi_num) {
			res[i] = curr->indicators[j];
			j++;
			i++;
		}
		j = 0;
		curr = curr->next;
	}
	return res;
}

double calculate_value_if_merge(Kn_List* list, double value) {
	Knot* next, *prev = kn_list_lower_node(list, value);
	if (!prev){
		return value;
	}
	next = prev->next;
	double epsilon = calculate_radius_epsilon(&list->vector_line) * RADIUS_SENSATIVITY;
	if (fabs(prev->value - value) < epsilon)
		return prev->value;
	if (next && fabs(next->value - value) < epsilon)
		return next->value;
	return value;
}

double calculate_point_value_on_kn_vec(Knot_Vector_Rep* vec, CAGD_POINT p) {
	CAGD_POINT vec_p = cagdPointSub(p, vec->vec_location[0]);
	double t = dot_prod(vec_p, vec->vec_locaVec) / (vec->length * vec->length);
	double m = (vec->edge_values[1]- vec->edge_values[0]);
	double res = vec->edge_values[0] + m * t;
	res = res < vec->edge_values[0] ? vec->edge_values[0] : res;
	res = res > vec->edge_values[1] ? vec->edge_values[1] : res;
	return res;
}
double knot_vector_click_to_value(Kn_List* list, int x, int y) {
	if(update_kn_vec_rep(&list->vector_line)){
		erase_KV(list);
		draw_KV(list);
	}
	CAGD_POINT p[2];
	cagdToObject(x, KNOT_Y_MARGIN, p);
	return calculate_value_if_merge(list,calculate_point_value_on_kn_vec(&list->vector_line, p[0]));
}

BOOL check_uniform_spacing(struct Kn_List* list) {
	if (!list->head || !list->head->next)
		return FALSE;
	double delta = list->head->next->value - list->head->value;
	Knot* curr = list->head->next;
	BOOL flag = TRUE;
	while (flag && curr->next) {
		flag = ((curr->next->value - curr->value) == delta);
		curr = curr->next;
	}
	return flag;
}

BOOL check_closed_float(Kn_List* list , int K) {
	return (list->tail->multi == K && list->head->multi == K);
}

void add_to_uniform(Kn_List* list) {
	if(!check_uniform_spacing(list))
		return;
	double value = list->tail->value;
	double delta = value - list->tail->prev->value;
	list->tail->value += delta;
	kn_list_insert(list, value, 1);
}

void remove_from_uniform(Kn_List* list) {
	if (!check_uniform_spacing(list) || !list->tail)
		return;
	if (list->tail == list->head)
		kn_list_remove(list->tail);
	else {
		double pre_val = list->tail->prev->value;
		if (kn_list_remove(list->tail->prev))
			list->tail->value = pre_val;
	}
}

void remake_as_closed_uni(Kn_List** list, int cp_len, int K) {
	if (!check_uniform_spacing(*list))
		remake_as_open_uni(list, cp_len, K);
	for (int i = 0; i < (K-1)*2; i++) {
		if ((*list)->tail->prev == (*list)->head)
			break;
		kn_list_erase((*list)->tail);
	}
	for (int i = 0; i < (K - 1) ; i++) {
		add_knot((*list)->head);
		add_knot((*list)->tail);
	}	
}

void remake_as_open_uni(Kn_List** list,int cp_len, int K) {
	Kn_List* res_list;
	kn_list_init(&res_list);
	for (int i = cp_len + K - 1; i >=0 ; i--) {
		kn_list_insert(res_list, i, 1);
	}
	*list = delete_kn_list(*list);
	*list = res_list;
}

void get_domain(Kn_List* list, int K, double domain[2]) {
	int k = K - 1;
	domain[0] = get_value(get_knot_at_index(list, k));
	domain[1] = get_value(get_knot_at_index(list, list->length - K));
}

struct CP_Grid {
	int u_length;
	int v_length;
	Control_Point*** cp_grid;
	UINT** cp_polygon_grid;
};

void printGrid(CP_Grid* cp_grid) {
	for (int i = 0; i < cp_grid->v_length; i++) {
		for (int j = 0; j < cp_grid->u_length; j++) {
			sprintf(logMsg, "|%d,%d|", (cp_grid->cp_grid[i][j] ? i + 1 : 0), (cp_grid->cp_grid[i][j] ? j + 1 : 0));
			LogToOutput(logMsg);
		}
		sprintf(logMsg, "\n");
		LogToOutput(logMsg);
	}
}

void printCPolygonIDs(CP_Grid* cp_grid) {
	for (int i = 0; i < cp_grid->v_length; i++) {
		for (int j = 0; j < cp_grid->u_length*2; j++) {
			sprintf(logMsg, "|%d|", cp_grid->cp_polygon_grid[i][j]);
			LogToOutput(logMsg);
		}
		sprintf(logMsg, "\n");
		LogToOutput(logMsg);
	}
}

void* delete_cp_grid(CP_Grid* grid) {

	// Allocate memory for cp_grid and initialize to NULL
	for (int i = 0; i < grid->v_length; ++i) {
		int j;
		for (j = 0; j < grid->u_length; ++j) {
			grid->cp_grid[i][j] = cp_delete(grid->cp_grid[i][j]);
			if(grid->cp_polygon_grid[i][j] != NA) cagdFreeSegment(grid->cp_polygon_grid[i][j]);
			grid->cp_polygon_grid[i][j] = NA;
		}
		for (; j < 2*grid->u_length; ++j) {
			if (grid->cp_polygon_grid[i][j] != NA) cagdFreeSegment(grid->cp_polygon_grid[i][j]);
			grid->cp_polygon_grid[i][j] = NA;
		}
		free(grid->cp_grid[i]);
		free(grid->cp_polygon_grid[i]);
	}
	free(grid->cp_grid);
	free(grid->cp_polygon_grid);
}

void init_cp_grid(CP_Grid** cp_grid, int v_l, int u_l) {
	*cp_grid = SAFE_MALLOC(CP_Grid, 1);
	if (!*cp_grid) {
		return;
	}
	(*cp_grid)->u_length = u_l;
	(*cp_grid)->v_length = v_l;

	// Allocate memory for cp_grid and initialize to NULL
	(*cp_grid)->cp_grid = SAFE_MALLOC(Control_Point**, v_l);
	for (int i = 0; i < v_l; ++i) {
		(*cp_grid)->cp_grid[i] = SAFE_MALLOC(Control_Point*, u_l);
		for (int j = 0; j < u_l; ++j) {
			(*cp_grid)->cp_grid[i][j] = NULL;
		}
	}

	// Allocate memory for cp_polygon_grid and initialize to NA
	(*cp_grid)->cp_polygon_grid = SAFE_MALLOC(UINT*, v_l);
	for (int i = 0; i < v_l; ++i) {
		(*cp_grid)->cp_polygon_grid[i] = SAFE_MALLOC(UINT, u_l *2);
		for (int j = 0; j < u_l *2; ++j) {
			(*cp_grid)->cp_polygon_grid[i][j] = NA;
		}
	}
}

static void resize_cp_grid(CP_Grid* cp_grid) {
	int new_u_length = cp_grid->u_length * 2;
	int new_v_length = cp_grid->v_length * 2;

	// Allocate new grid
	Control_Point*** new_cp_grid = SAFE_MALLOC(Control_Point**, new_v_length);
	for (int i = 0; i < new_v_length; ++i) {
		new_cp_grid[i] = SAFE_MALLOC(Control_Point*, new_u_length);
		for (int j = 0; j < new_u_length; ++j) {
			if (j < cp_grid->u_length && i < cp_grid->v_length) {
				new_cp_grid[i][j] = cp_grid->cp_grid[i][j];
			}
			else {
				new_cp_grid[i][j] = NULL;
			}
		}
	}
	// Allocate new polygon grid
	UINT** new_cp_polygon_grid = SAFE_MALLOC(UINT*, new_v_length);
	for (int i = 0; i < new_v_length; ++i) {
		new_cp_polygon_grid[i] = SAFE_MALLOC(UINT, new_u_length*2);
		for (int j = 0; j < new_u_length *2; ++j) {
			if (i < cp_grid->v_length && j < cp_grid->u_length*2) {
				new_cp_polygon_grid[i][j] = cp_grid->cp_polygon_grid[i][j];
			}
			else {
				new_cp_polygon_grid[i][j] = NA;
			}
		}
	}

	// Free old grid
	for (int i = 0; i < cp_grid->v_length; ++i) {
		free(cp_grid->cp_grid[i]);
		free(cp_grid->cp_polygon_grid[i]);

	}
	free(cp_grid->cp_grid);
	free(cp_grid->cp_polygon_grid);

	// Assign new grid
	cp_grid->cp_grid = new_cp_grid;
	cp_grid->cp_polygon_grid = new_cp_polygon_grid;
	cp_grid->u_length = new_u_length;
	cp_grid->v_length = new_v_length;
}

void update_grid_line(CP_Grid* cp_grid, Control_Point* cp, int v_i, int u_i) {
	CAGD_POINT q[2];
	const int left_neigbor = u_i - 1;
	const int right_neigbor = u_i + 1;
	const int top_neigbor = v_i - 1;
	const int bot_neigbor = v_i + 1;
	const int right_line = 2 * u_i + 1;
	const int left_line = 2 * (u_i - 1) + 1;
	const int bot_line = 2 * u_i;
	BOOL directions[4]; 
	directions[0] = left_neigbor >= 0 && cp_grid->cp_grid[v_i][left_neigbor];
	directions[1] = right_neigbor < cp_grid->u_length&& cp_grid->cp_grid[v_i][right_neigbor];
	directions[2] = top_neigbor >= 0 && cp_grid->cp_grid[top_neigbor][u_i];
	directions[3] = bot_neigbor < cp_grid->v_length&& cp_grid->cp_grid[bot_neigbor][u_i];
	if(!cp)
	 {
		if (directions[0] && cp_grid->cp_polygon_grid[v_i][left_line] != NA) { cagdFreeSegment(cp_grid->cp_polygon_grid[v_i][left_line]); cp_grid->cp_polygon_grid[v_i][left_line] = NA; };
		if (directions[1] && cp_grid->cp_polygon_grid[v_i][right_line] != NA) { cagdFreeSegment(cp_grid->cp_polygon_grid[v_i][right_line]); cp_grid->cp_polygon_grid[v_i][right_line] = NA; };
		if (directions[2] && cp_grid->cp_polygon_grid[top_neigbor][bot_line] != NA) { cagdFreeSegment(cp_grid->cp_polygon_grid[top_neigbor][bot_line]); cp_grid->cp_polygon_grid[top_neigbor][bot_line] = NA; };
		if (directions[3] && cp_grid->cp_polygon_grid[v_i][bot_line] != NA) { cagdFreeSegment(cp_grid->cp_polygon_grid[v_i][bot_line]); cp_grid->cp_polygon_grid[v_i][bot_line] = NA; };
	}
	else{
		q[1] = get_location(cp);
		if (directions[1]) {
			q[0] = get_location(cp_grid->cp_grid[v_i][right_neigbor]);
			if (cp_grid->cp_polygon_grid[v_i][right_line] != NA) cagdReusePolyline(cp_grid->cp_polygon_grid[v_i][right_line], q, 2);
			else cp_grid->cp_polygon_grid[v_i][right_line] = cagdAddPolyline(q, 2);
		}
		if (directions[0]) {
			q[0] = get_location(cp_grid->cp_grid[v_i][left_neigbor]);
			if (cp_grid->cp_polygon_grid[v_i][left_line] != NA) cagdReusePolyline(cp_grid->cp_polygon_grid[v_i][left_line], q, 2);
			else cp_grid->cp_polygon_grid[v_i][left_line] = cagdAddPolyline(q, 2);
		}
		if (directions[3]) {
			q[0] = get_location(cp_grid->cp_grid[bot_neigbor][u_i]);
			if (cp_grid->cp_polygon_grid[v_i][bot_line] != NA) cagdReusePolyline(cp_grid->cp_polygon_grid[v_i][bot_line], q, 2);
			else cp_grid->cp_polygon_grid[v_i][bot_line] = cagdAddPolyline(q, 2);
		}
		if (directions[2]) {
			q[0] = get_location(cp_grid->cp_grid[top_neigbor][u_i]);
			if (cp_grid->cp_polygon_grid[top_neigbor][bot_line] != NA) cagdReusePolyline(cp_grid->cp_polygon_grid[top_neigbor][bot_line], q, 2);
			else cp_grid->cp_polygon_grid[top_neigbor][bot_line] = cagdAddPolyline(q, 2);
		}

	}
}
Control_Point* update_grid_cp_at(CP_Grid* cp_grid, Control_Point* cp, int v_i, int u_i) {
	if (u_i >= cp_grid->u_length || v_i >= cp_grid->v_length) {
		if (!cp)
			return;
		resize_cp_grid(cp_grid);
	}
	Control_Point* res = NULL;
	BOOL replace_old_location = (BOOL)cp_grid->cp_grid[v_i][u_i];
	if (replace_old_location) {
		if (cp != cp_grid->cp_grid[v_i][u_i]) {
			res = cp_grid->cp_grid[v_i][u_i];
			cp_grid->cp_grid[v_i][u_i] = NULL;
		}
	}
	cp_grid->cp_grid[v_i][u_i] = cp;
	update_grid_line(cp_grid, cp, v_i, u_i);
	return res;
}


void transform_grid_cp_at(CP_Grid* cp_grid, CAGD_POINT delta, int v_i, int u_i) {
	if (u_i < 0 || u_i >= cp_grid->u_length || v_i < 0 || v_i >= cp_grid->v_length)
		return;
	update_cp(cp_grid->cp_grid[v_i][u_i], cagdPointAdd(get_location(cp_grid->cp_grid[v_i][u_i]), delta));
	update_grid_line(cp_grid, cp_grid->cp_grid[v_i][u_i], v_i, u_i);
}


void shift_and_fill_row(CP_Grid* cp_grid, Control_Point* cp, int row, int col) {
	// Ensure the row index is within bounds
	if (col < 0 || col > cp_grid->u_length || row < 0 || row > cp_grid->v_length)
		return;

	// Shift CPs to the right
	for (int u_i = cp_grid->u_length; u_i > col; --u_i) {
		update_grid_cp_at(cp_grid, cp_grid->cp_grid[row][u_i-1], row, u_i);
	}
	update_grid_cp_at(cp_grid, cp, row, col);
}

void shift_and_fill_col(CP_Grid* cp_grid, Control_Point* cp, int row, int col) {
	// Ensure the column index is within bounds
	if (col < 0 || col > cp_grid->u_length || row < 0 || row > cp_grid->v_length)
		return;

	// Shift CPs down
	for (int v_i = cp_grid->v_length; v_i > row; --v_i) {
		update_grid_cp_at(cp_grid, cp_grid->cp_grid[v_i - 1][col], v_i, col);
	}

	// Set the specified index to NULL
	update_grid_cp_at(cp_grid, cp, row, col);
}

void update_grid_block(CP_Grid* cp_grid, Control_Point*** updated_points, int rows, int cols, int s_row, int s_col, char* dir) {
	printGrid(cp_grid);
	BOOL dir_bool = strcmp(dir, "u") == 0;
	int u_i, v_i;
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			u_i = !dir_bool ? s_row + i : s_col + j;
			v_i = dir_bool ? s_row + i : s_col + j;
			if (j == cols - 1)
				dir_bool ? shift_and_fill_row(cp_grid, updated_points[i][j], v_i, u_i) : shift_and_fill_col(cp_grid, updated_points[i][j], v_i, u_i);
			else
				cp_delete(update_grid_cp_at(cp_grid, updated_points[i][j], v_i, u_i, TRUE));
		}
	}
	printGrid(cp_grid);
}

Control_Point* get_cp_grid(CP_Grid* cp_grid, int v_i, int u_i) {
	return cp_grid->cp_grid[v_i][u_i];
}
void transform_grid(CP_Grid* cp_grid, CAGD_POINT delta) {
	for (int i = 0; i < cp_grid->u_length; i++) {
		for (int j = 0; j < cp_grid->v_length; j++) {
			Control_Point* cp = cp_grid->cp_grid[j][i];
			if (cp) {
				update_cp(cp, cagdPointAdd(get_location(cp), delta));
				update_grid_cp_at(cp_grid, cp, j, i);
			}
		}
	}
}

// Function to get a row of Control_Point
Control_Point** get_cp_row(CP_Grid* grid, int row) {
	if (row < 0 || row >= grid->v_length) {
		return NULL; // Invalid row index
	}

	// Allocate memory for the row copy
	Control_Point** row_copy = SAFE_MALLOC(Control_Point*, grid->u_length);

	// Copy the pointers from the specified row
	for (int i = 0; i < grid->u_length; ++i) {
		row_copy[i] = grid->cp_grid[row][i];
	}

	return row_copy;
}

// Function to get a column of Control_Point
Control_Point** get_cp_column(CP_Grid* grid, int column) {
	if (column < 0 || column >= grid->u_length) {
		return NULL; // Invalid column index
	}

	Control_Point** column_array = SAFE_MALLOC(Control_Point*, grid->v_length);
	for (int i = 0; i < grid->v_length; ++i) {
		column_array[i] = grid->cp_grid[i][column];
	}

	return column_array;
}

void toggle_cp_grid_visiblity(CP_Grid* cp_grid, BOOL cp_vis, BOOL cp_poly_vis, BOOL cp_weight_vis) {
	if (!cp_grid)
		return;
	for (int i = 0; i < cp_grid->v_length; i++) {
		for (int j = 0; j < cp_grid->u_length; j++) {
			Control_Point* cp = cp_grid->cp_grid[i][j];
			if (cp) {
				cp_vis ? cagdShowSegment(cp->id) : cagdHideSegment(cp->id);
				cp_weight_vis ? cagdShowSegment(cp->weight_circle) : cagdHideSegment(cp->weight_circle);
			}
		}
		for (int j = 0; j < cp_grid->u_length * 2; ++j) {
			UINT grid_line_id = cp_grid->cp_polygon_grid[i][j];
			if (grid_line_id != NA) {
				cp_poly_vis ? cagdShowSegment(grid_line_id) : cagdHideSegment(grid_line_id);
			}
		}
	}
}

Control_Point* get_cp_by_id_grid(CP_Grid* cp_grid, UINT id, int* v_i, int* u_i) {
	for (int i = 0; i < cp_grid->v_length; i++) {
		for (int j = 0; j < cp_grid->u_length; j++) {
			Control_Point* cp = cp_grid->cp_grid[i][j];
			if (cp) {
				if (cp->id == id) {
					*u_i = j;
					*v_i = i;
					return cp;
				}
			}
		}
	}
	*v_i = -1;
	*u_i = -1;
	return NULL;
}

Knot* get_moving_knot(Kn_List* kn_list) {
	return kn_list->moving_knot;
}
void set_moving_knot(Knot* kn) {
	kn->parent->moving_knot = kn;
}
void set_moving_knot_color(Kn_List* kn_list , BYTE r, BYTE g, BYTE b) {
	cagdSetSegmentColor(kn_list->moving_knot->indicators[0], r, g, b);
}
void remove_from_moving_knot(Kn_List* kn_list) {
	if (!kn_list_remove(kn_list->moving_knot))
		update_single_KVector(kn_list->moving_knot);
}

double bound_to_domain(Kn_List* kn_list, int K, double value) {
	double domain[2];
	get_domain(kn_list, K, domain);
	value = value < domain[0] ? domain[0] : value;
	value = value >= domain[1] ? domain[1] - 0.000001 : value;
	return value;
}

double updateKnotsMove(int x, int y, PVOID userData) {
	Kn_List* kn_list = (Kn_List*)userData;
	remove_from_moving_knot(kn_list);
	double value = knot_vector_click_to_value(kn_list, x, y);
	kn_list->moving_knot = kn_list_insert(kn_list, value, 1);
	update_single_KVector(kn_list->moving_knot);
	set_moving_indicator_moving(kn_list->moving_knot);
	cagdRedraw();
	return value;
}


void updateKnotsLeftUp(int x, int y, PVOID userData) {
	Kn_List* kn_list = (Kn_List*)userData;
	set_moving_indicator_inplace(kn_list->moving_knot);
	cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
	cagdRedraw();
}


int containsPoly(TimeStampedPolyGroup polyGroup,  UINT id) {
	if (!polyGroup.segments)
		return -1;
	for (int i = 0; i < polyGroup.len; i++) if(polyGroup.segments[i].line_id == id) return i;
	return -1;
}
void emptyStampedPoly(TimeStampedPolyline* poly) {
	if (!poly)
		return;
	cagdFreeSegment(poly->line_id);
	if(poly->time_stamps) free(poly->time_stamps);
	poly->time_stamps = NULL;
	poly->len = 0;
}

void emptyStampedPolyGroup(TimeStampedPolyGroup* polyGroup)
{
	if (!polyGroup)
		return;
	for (int i = 0; i < polyGroup->len; i++) emptyStampedPoly(&polyGroup->segments[i]);
	if(polyGroup->segments) free(polyGroup->segments);
	polyGroup->segments = NULL;
	polyGroup->len = 0;
}

double get_time(TimeStampedPolyline poly, UINT vertex) {
	return poly.time_stamps[vertex];
}

void initPolyLine(TimeStampedPolyline* pl) {
	if (!pl)
		return;
	pl->line_id = NA;
	pl->time_stamps = NULL;
	pl->len = 0;
}
void initPolyLineGroup(TimeStampedPolyGroup* pg) {
	pg->segments = NULL;
	pg->len = 0;
}

void set_polyline_color(TimeStampedPolyline* poly, BYTE r, BYTE g, BYTE b) {
	if (!poly)
		return;
	cagdSetSegmentColor(poly->line_id, r, g, b);
}
void set_polyline_group_color(TimeStampedPolyGroup* poly, BYTE r, BYTE g, BYTE b) {
	if (!poly->segments)
		return;
	for (int i = 0; i < poly->len; i++) {
		set_polyline_color(&poly->segments[i], r, g, b);
	}
}
