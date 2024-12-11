#pragma once
#include <cagd.h>
#include <stdio.h>
#include <windows.h>
#include "resource.h"
#include "EnumHell.h"
#include <math.h>
#include <time.h>

#define NA -1

typedef struct Control_Point Control_Point;
typedef struct CP_List_Node CP_List_Node;
typedef struct  CP_List CP_List;
typedef struct Knot Knot;
typedef struct Kn_List Kn_List;
typedef struct CP_Grid CP_Grid;


CAGD_POINT get_location(Control_Point* cp);
double get_weight(Control_Point* cp);
void update_weight(struct Control_Point* cp, double weight);
void update_weight_from_window(struct Control_Point* cp, int x, int y);
Control_Point* cp_create(CAGD_POINT p, double w);
void* cp_delete(Control_Point* cp);
//void draw_weight_circle( Control_Point* cp);
void update_cp( Control_Point* cp, CAGD_POINT where);
Control_Point* cp_add( Control_Point* cp1,  Control_Point* cp2);


CP_List* get_parent(CP_List_Node* node);
Control_Point* get_cp(CP_List_Node* node);
CP_List_Node* get_next(CP_List_Node* node);
CP_List_Node* get_prev(CP_List_Node* node);
void cp_list_node_init( CP_List_Node* cp_node);
void update_control_node( CP_List_Node* cp_node, CAGD_POINT where);
void update_control_node_2( CP_List_Node* cp_node, int x, int y);
BOOL remove_cp_node(struct CP_List_Node* cp_node);
void create_faux_node_at(CP_List_Node** cp_node, CAGD_POINT where);


BOOL is_empty(CP_List* list);
CP_List_Node* get_first(CP_List* list);
CP_List_Node* get_last(CP_List* list);
int cp_list_len(struct CP_List* list);
void cp_list_init( CP_List** list);
void push_back_cp( CP_List* list,CAGD_POINT p, double w);
void push_next_to_cp( CP_List* list,  CP_List_Node* prev,  Control_Point* cp);
Control_Point** get_cp_array( CP_List* list);
CP_List_Node* get_cp_by_id_list( CP_List* list, UINT id);
CP_List_Node* get_cp_by_index( CP_List* list, int index);
BOOL remove_cp(CP_List* list, Control_Point* cp);
void* delete_cp_list(CP_List* list);
void transform_list( CP_List* list, CAGD_POINT delta);
void rotate_list( CP_List* list, CAGD_POINT center, double degree);
void toggle_cp_list_visiblity( CP_List* list, BOOL cp_vis, BOOL cp_poly_vis, BOOL cp_weight_vis);
Control_Point** get_cp_array_range( CP_List* list, int start_i, int range);



double get_value(Knot* knot);
int get_multi(Knot* knot);
void add_knot(Knot* knot);
//void kn_init(struct Knot* kn);


void kn_list_init(Kn_List** list);
int kn_list_len( Kn_List* list);
int kn_list_upper_index( Kn_List* list, double value);
int kn_list_lower_index( Kn_List* list, double value);
Knot* kn_list_upper_node( Kn_List* list, double value);
Knot* kn_list_lower_node( Kn_List* list, double value);
Knot* kn_list_insert( Kn_List* list, double value, int multi);
BOOL kn_list_remove(Knot* kn);
void kn_list_erase(Knot* kn);
Kn_List* copy_knot_list(Kn_List* list);
void* delete_kn_list(Kn_List* list);
Knot* get_knot_at_index( Kn_List* list, int index);
double* get_knot_array( Kn_List* list);
double* get_knot_range( Kn_List* list, int index, int range);
BOOL check_uniform_spacing( Kn_List* list);
void draw_KV( Kn_List* list);
void update_single_KVector(Knot* kn);
void erase_KV(Kn_List* list);
void create_knot_indicators(CAGD_POINT axis, Knot* kn, CAGD_POINT pos);
void set_moving_indicator_moving(Kn_List* list);
void set_moving_indicator_inplace( Kn_List* list);
Knot* get_knot_from_indicator( Kn_List* list, int x, int y);
UINT* get_knot_indicator_arr( Kn_List* list);
double knot_vector_click_to_value( Kn_List* list, int x, int y);
BOOL check_closed_float(Kn_List* list, int K);
void add_to_uniform(Kn_List* list);
void remove_from_uniform(Kn_List* list);
void remake_as_closed_uni(Kn_List** list, int cp_len, int K);
void remake_as_open_uni(Kn_List** list, int cp_len, int K);
void get_domain(Kn_List* list, int K, double domain[2]);
double updateKnotsMove(int x, int y, PVOID userData);
void updateKnotsLeftUp(int x, int y, PVOID userData);
Knot* get_moving_knot(Kn_List* kn_list);
void set_moving_knot(Knot* kn);
void set_moving_knot_color(Kn_List* kn_list, BYTE r, BYTE g, BYTE b);
void remove_from_moving_knot(Kn_List* kn_list);
double bound_to_domain(Kn_List* kn_list, int K, double value);

void init_cp_grid(CP_Grid** cp_grid, int u_l, int v_l);
Control_Point* update_grid_cp_at(CP_Grid* cp_grid, Control_Point* cp, int u_i, int v_i);
Control_Point** get_cp_row(CP_Grid* grid, int row);
Control_Point** get_cp_column(CP_Grid* grid, int column);
void transform_grid(CP_Grid* cp_grid, CAGD_POINT delta);
void toggle_cp_grid_visiblity(CP_Grid* cp_grid, BOOL cp_vis, BOOL cp_poly_vis, BOOL cp_weight_vis);
Control_Point* get_cp_by_id_grid(CP_Grid* cp_grid, UINT id, int* u_i, int* v_i);
Control_Point* get_cp_grid(CP_Grid* cp_grid, int u_i, int v_i);
void transform_grid_cp_at(CP_Grid* cp_grid, CAGD_POINT delta, int u_i, int v_i);
void update_grid_block(CP_Grid* cp_grid, Control_Point*** updated_points, int rows, int cols, int s_row, int s_col, char* dir);
void* delete_cp_grid(CP_Grid* grid);

typedef struct {
    UINT line_id;
    double* time_stamps;
    int len;
}TimeStampedPolyline;


typedef struct {
    TimeStampedPolyline* segments;
    int len;
}TimeStampedPolyGroup;

void initPolyLine(TimeStampedPolyline* pl);
void initPolyLineGroup(TimeStampedPolyGroup* pg);
int containsPoly(TimeStampedPolyGroup polyGroup, UINT id);
void emptyStampedPolyGroup(TimeStampedPolyGroup* polyGroup);
void emptyStampedPoly(TimeStampedPolyline* poly);
double get_time(TimeStampedPolyline poly, UINT vertex);
void set_polyline_color(TimeStampedPolyline* poly, BYTE r, BYTE g, BYTE b);
void set_polyline_group_color(TimeStampedPolyGroup* poly, BYTE r, BYTE g, BYTE b);