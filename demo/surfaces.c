#include "surfaces.h"


enum DRAWING_BOOL {
    B_TANGENTS = 0,
    B_TANGENTP,
    B_CURVATURE,
    B_NORMAL,
    B_LAST
};

typedef struct {
    Surface** surfaces;
    size_t count;
    size_t capacity;
    Surface* last_clicked;
    unsigned int popUpBitMap;
    unsigned int menuBitmap;
    CAGD_POINT moving_ref;
    BOOL draw_geo[B_LAST];
    HMENU myPopup, myKnotPopup, myMenu;
    double anime_speed;
} SurfaceManager;

typedef struct {
    TimeStampedPolyGroup* lines;
    double* time;
    int len;
}SurfaceLinesWrapper;


enum DRAWINGS {
    D_CLI_TANGENTU = 0,
    D_CLI_TANGENTV,
    D_CLI_TANGENTP,
    D_CLI_NORMAL,
    D_CLI_OCSCIRC1,
    D_CLI_OCSCIRC2,
    D_CLI_PDIR1,
    D_CLI_PDIR2,
    D_ANI_TANGENTU,
    D_ANI_TANGENTV,
    D_ANI_TANGENTP,
    D_ANI_NORMAL,
    D_ANI_OCSCIRC1,
    D_ANI_OCSCIRC2,
    D_ANI_PDIR1,
    D_ANI_PDIR2,
    D_LAST
};


struct Surface {
    CP_Grid* cp_grid; //grid of control points and polygon
    Kn_List* u_kn_list; // u side knot list
    Kn_List* v_kn_list; // v side knot list
    SurfaceLinesWrapper u_polys,  v_polys; // lines of drawing in each direction
    TimeStampedPolyGroup insertionIndicator;
    int u_cp_len;
    int v_cp_len;
    int u_order;
    int v_order;
    BOOL showWeights;
    BOOL dead;
    Control_Point* movingPoint;
    int mp_ui; 
    int mp_vi;
    char* dir;
    UINT drawings[D_LAST];
    double from_anime[2];
    double to_anime[2];
    double curr_anime;
    double offset;
    int anime_points_selected; 
    int id;
};

enum Surface_Control {
    CLICK = 0,
    SPAN,
    ANIM,
    TNGV,
    NORM,
    TNGP,
    PDIR,
    OFSE,
    KOFF,
    HOFF,
    K1OF,
    K2OF,
};

typedef enum {
    SRF_CLICK = SURFACES_CMD_START,
    SRF_SPAN,
    SRF_ANIM,
    SRF_TNGV,
    SRF_TNGP,
    SRF_NORM,
    SRF_PDIR,
    SRF_OFFSET,
    SRF_OFPAR,
    SRF_KOFF,
    SRF_HOFF,
    SRF_K1OFF,
    SRF_K2OFF,
    SRF_FIN,
    SRF_CLEAR,
    SRF_SLCT,
    SRF_LOAD,
    SRF_CREATE,
    SRF_ADDKN,
    SRF_RMKN,
    SRF_MOVE,
    SRF_WEIGHT,
    SRF_REMOVES,
    SRF_KNOTSU,
    SRF_KNOTSV,
    SRF_INKNOTU,
    SRF_INKNOTV,
    SRF_DEGU,
    SRF_DEGV,
    SRF_UNIOPU,
    SRF_UNIOPV,
    SRF_UNICU,
    SRF_UNICV,
    SRF_SLOWER,
    SRF_FASTER
} SurfaceCommands;

enum popUpFlags {
    FLAG_WIEGHT = 0,
    FLAG_EDKNOTU,
    FLAG_INSKNOTU,
    FLAG_EDKNOTV,
    FLAG_INSKNOTV,
    FLAG_MOVE,
    FLAG_REMOVE,
    FLAG_ANIME,
    FLAG_CLICK
};

static void defaultSurfaceLeftClick(int x, int y, PVOID userData);
static void defaultSurfaceRightUp(int x, int y, PVOID userData);
static Control_Point* getClosestControlPoint(Surface* surf, int x, int y, int* u_i, int* v_i);
static void editSurfaceCPLeftDown(int x, int y, PVOID userData);
void emptySurfaceLinesWrapper(SurfaceLinesWrapper polys);
static void moveSurfaceHandler(Surface* surf, BOOL flag);
void surfaceWeightsHandler(Surface* surf, BOOL flag);
static void knotsHandler(Surface* surf, BOOL flag, char* dir);
void calculateFirstFundamentalForm(CAGD_POINT Su, CAGD_POINT Sv, double* E, double* F, double* G);
void calculateSecondFundamentalForm(Surface* surf, double u, double v, CAGD_POINT normal, double* L, double* M, double* N);
static void evaluateSurfacePartialDerivative(Surface* surf, double u, double v, CAGD_POINT* Su, CAGD_POINT* Sv);
void drawShapesClickHandler(Surface* surf, BOOL flag);
double* evaluateBSplineBasisDerivatives(Kn_List* kn_list, double t, int J, int K, int deriv_deg);
static void hideAllSegments(Surface* surf);
static void showSegments(Surface* surf);
void drawShapesAnimeHandler(Surface* surf, BOOL flag);
void offset_surface(Surface* surf, enum Surface_Control flag);
void knotInsertionSurfaceWrapper(Surface* surf, double t);
void inknotsSurfaceHandler(Surface* surf, BOOL flag, char* dir);
static Control_Point** generalKnotInsertion(Control_Point** cp_array, Kn_List* kn_list, int K, double value, int* change_s, Knot** inserted);
static void colorItDead(Surface* surf);
static TimeStampedPolyGroup draw_line_at_value(Surface* surf, double value, char* dir);
void drawSurface(Surface* surf);
INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GridContainerProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void moveSurfaceLeftDown(int x, int y, PVOID userData);
static void* deleteSurface(Surface* surf);

// Singleton instance
static SurfaceManager* instance = NULL;

// Function prototypes
SurfaceManager* getSurfaceManager();
void initSurfaceManager(size_t initialCapacity);
void pushSurface(struct Surface* surface);
void popSurface();
void freeSurfaceManager();
void cleanupSurfaceManager();


SurfaceManager* getSurfaceManager() {
    if (instance == NULL) {
        initSurfaceManager(10);
    }
    return instance;
}

// Initialize the SurfaceManager
void initSurfaceManager(size_t initialCapacity) {
    if (instance != NULL) {
        fprintf(stderr, "SurfaceManager is already initialized.\n");
        return;
    }
    instance = SAFE_MALLOC(SurfaceManager, 1);
    instance->surfaces = SAFE_MALLOC(Surface*, initialCapacity);
    instance->count = 0;
    instance->capacity = initialCapacity;
    instance->last_clicked = NULL;
    instance->popUpBitMap = 0;
    instance->menuBitmap = 0;
    instance->moving_ref = (CAGD_POINT){ 0,0,0 };
    for (int i = 0; i < B_LAST; i++) {
        instance->draw_geo[i] = FALSE;
    }
    instance->anime_speed = 0.0002;
    finesse = 25;
}

// Cleanup the SurfaceManager
static void cleanupSurfaceManager() {
    SurfaceManager* manager = getSurfaceManager();
    for (size_t i = 0; i < manager->count; ++i) {
        deleteSurface(manager->surfaces[i]);
        free(manager->surfaces[i]);
        manager->surfaces[i] = NULL;
    }
    manager->count = 0;
    manager->last_clicked = NULL;
    manager->popUpBitMap = 0;
    manager->menuBitmap = 0;
    manager->moving_ref.x = manager->moving_ref.y = manager->moving_ref.z = 0;
}
// Add a Surface to the manager
void pushSurface(Surface* surface) {
    SurfaceManager* manager = getSurfaceManager();
    if (manager->count >= manager->capacity) {
        manager->capacity *= 2;
        manager->surfaces = (Surface**)realloc(manager->surfaces, manager->capacity * sizeof(Surface*));
    }
    surface->id = manager->count;
    manager->surfaces[manager->count++] = surface;
}

// Remove the last Surface from the manager
void popSurface() {
    SurfaceManager* manager = getSurfaceManager();
    if (manager->count > 0) {
        if (manager->surfaces[manager->count-1] == manager->last_clicked) {
            manager->last_clicked = NULL;
        }
        manager->surfaces[manager->count-1] = deleteSurface(&manager->surfaces[manager->count-1]);
        manager->count--;
    }
    else {
        fprintf(stderr, "No surfaces to pop.\n");
    }
}
void removeSurface(Surface* surface) {
    SurfaceManager* manager = getSurfaceManager();
    int id = surface->id;
    if (id < 0 || id >= manager->count)
        return;
    if (manager->surfaces[id] == manager->last_clicked) {
        manager->last_clicked = NULL;
    }
    manager->surfaces[id] = deleteSurface(manager->surfaces[id]);
    manager->count--;
    manager->surfaces[id] = manager->surfaces[manager->count];
    if(manager->surfaces[id]) manager->surfaces[id]->id = id;
    manager->surfaces[manager->count] = NULL;
}

// Remove the singleton instance
void freeSurfaceManager() {
    if (instance != NULL) {
        cleanupSurfaceManager();
        free(instance);
        instance = NULL;
    }
}

// Cleanup Surface
static void* deleteSurface(Surface* surf){
    emptySurfaceLinesWrapper(surf->u_polys);
    emptySurfaceLinesWrapper(surf->v_polys);
    delete_cp_grid(surf->cp_grid);
    surf->movingPoint = NULL;
    surf->u_kn_list = delete_kn_list(surf->u_kn_list);
    surf->v_kn_list = delete_kn_list(surf->v_kn_list);
    surf->u_cp_len = surf->v_cp_len = 0;
    surf->u_order = surf->v_order = 0;
    surf->mp_ui = surf->mp_vi = 0;
    surf->dir = NULL;
    emptyStampedPolyGroup(&surf->insertionIndicator);
    for (int i = 0; i < D_LAST; i++) {
        cagdFreeSegment(surf->drawings[i]);
        surf->drawings[i] = NA;
    }
    return NULL;
}

void initSurface(Surface* surf) {
    surf->u_polys = (SurfaceLinesWrapper){ NULL, NULL, 0 };
    surf->v_polys = (SurfaceLinesWrapper){ NULL, NULL, 0 };
    surf->cp_grid = NULL;
    surf->movingPoint = NULL;
    surf->u_kn_list = NULL;
    surf->v_kn_list = NULL;
    surf->u_cp_len = surf->v_cp_len = 0;
    surf->u_order = surf->v_order = 0;
    surf->mp_ui = surf->mp_vi = -1;
    surf->showWeights = FALSE;
    surf->dir = NULL;
    surf->offset = 0;
    surf->dead = FALSE;
    initPolyLineGroup(&surf->insertionIndicator);
    for (int i = 0; i < 2; i++) {
        surf->from_anime[i] = 0;
        surf->to_anime[i] = 0;
    }
    surf->curr_anime = 0;
    surf->anime_points_selected = 0;
    CAGD_POINT dummy[2] = { {0,0,0}, {1,1,1} };
    for (int i = 0; i < D_LAST; i++) {
        cagdHideSegment(surf->drawings[i] = cagdAddPolyline(dummy, 2));
    }
    surf->id = -1;
}


//MALLOC SAFE
void reuse_plane(UINT plane_id, CAGD_POINT corners[4]){
    CAGD_POINT delta_side1, delta_side2;
    int points = finesse;
    delta_side1 = scaler(cagdPointSub(corners[1], corners[0]), 1/(double)points);
    delta_side2 = scaler(cagdPointSub(corners[2], corners[3]), 1/(double)points);
    CAGD_POINT* plane_points = SAFE_MALLOC(CAGD_POINT, (points * 2 + 9));
    plane_points[0] = corners[0];
    int j = 1; 
    for (int i = 0; i < points; i++, j+=2) {
        plane_points[j] = i % 2 == 0 ? cagdPointAdd(corners[3], scaler(delta_side2,i)) : cagdPointAdd(corners[0], scaler(delta_side1, i));
        plane_points[j+1] = i % 2 != 0 ? cagdPointAdd(corners[3], scaler(delta_side2, i)) : cagdPointAdd(corners[0], scaler(delta_side1, i));
    }
    if (j != points * 2 + 1) {
        return;
    }
    for (; j < points * 2 + 9; j++) {
        plane_points[j] = corners[j % 4];
    }
    cagdReusePolyline(plane_id,plane_points, j);
    free(plane_points);
}



void loadSurface(int path, int unUsed, PVOID userData) {
    FILE* file = fopen(path, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }
    HMENU* hMenu = (HMENU*)userData;
    char line[256];
    int i = 0, u_i = 0, v_i = 0;
    Surface* res = SAFE_MALLOC(Surface, 1);
    initSurface(res);
    cagdSetColor(0, 255, 255);
    while (fgets(line, sizeof(line), file) != NULL) {
        int j = strspn(line, " \t");
        if (line[j] == '#' || line[j] == '\n')
            continue;
        double x, y, z, w;
        int scan_res, kn_len, index;
        switch (i) {
        case 0: //scan orders
            if (sscanf(line, " %d %d", &(res->u_order), &(res->v_order)) != 2 || res->u_order < 1 || res->v_order < 1) {
                myMessage("Change Order", "Unacceptable Order!", MB_ICONERROR);
                return;
            }
            i++;
            break;
        case 1:
            if (sscanf(line, " u_knots[%d] =%n", &kn_len, &index) == 1) {
                if (kn_len < 1) {
                    myMessage("Change Number Of Knots", "Should be a natural number (not including 0)!", MB_ICONERROR);
                    return;
                }
                kn_list_init(&res->u_kn_list);
                int offset = index, j = 0;
                double knot;
                while (j < kn_len) {
                    if (sscanf(line + offset, " %lf%n", &knot, &index) != 1) {
                        if (fgets(line, sizeof(line), file) == NULL)
                            return;
                        offset = 0;
                    }
                    else {
                        kn_list_insert(res->u_kn_list, knot, 1);
                        //	sprintf(logMsg, "j: %d , knot = %lf \n", j, knot);
                        //	LogToOutput(logMsg);
                        offset += index;
                        j++;
                    }
                }
            }
            else if (sscanf(line, " v_knots[%d] =%n", &kn_len, &index) == 1) {
                if (kn_len < 1) {
                    myMessage("Change Number Of Knots", "Should be a natural number (not including 0)!", MB_ICONERROR);
                    return;
                }
                kn_list_init(&res->v_kn_list);
                int offset = index, j = 0;
                double knot;
                while (j < kn_len) {
                    if (sscanf(line + offset, " %lf%n", &knot, &index) != 1) {
                        if (fgets(line, sizeof(line), file) == NULL)
                            return;
                        offset = 0;
                    }
                    else {
                        kn_list_insert(res->v_kn_list, knot, 1);
                        //	sprintf(logMsg, "j: %d , knot = %lf \n", j, knot);
                        //	LogToOutput(logMsg);
                        offset += index;
                        j++;
                    }
                }
            }
            else if (sscanf(line, " points[%d][%d]", &res->u_cp_len, &res->v_cp_len) == 2) {
                if (!res->v_kn_list) {
                    kn_list_init(&res->v_kn_list);
                    remake_as_closed_uni(&res->v_kn_list, res->v_cp_len, res->v_order);
                }
                if(!res->u_kn_list){
                    kn_list_init(&res->u_kn_list);
                    remake_as_closed_uni(&res->u_kn_list, res->u_cp_len, res->u_order);
                }
                init_cp_grid(&res->cp_grid, res->v_cp_len, res->u_cp_len);
                i++;

            }
            else {
                myMessage("File incorrect format!", "Unable to scan knots", MB_ICONERROR);
                return;
            }
            break;
        case 2:
            scan_res = sscanf(line, " %lf %lf %lf %lf ", &x, &y, &z, &w);
            if (scan_res == 3) {
                update_grid_cp_at(res->cp_grid, cp_create((CAGD_POINT) { x, y, z }, 1), v_i, u_i);
            }
            else if (scan_res == 4) {
                update_grid_cp_at(res->cp_grid, cp_create((CAGD_POINT) {x/w,y/w,z/w}, w), v_i, u_i);
            }
            else {
                myMessage("File incorrect format!", "Unable to scan points", MB_ICONERROR);
            }
            u_i++;
            if (u_i == res->u_cp_len) {
                u_i = 0;
                v_i++;
            }
            break;
        }
    }
    fclose(file);
    toggle_cp_grid_visiblity(res->cp_grid, FALSE, FALSE, FALSE);
    pushSurface(res);
    drawSurface(res);
}


Surface* createSurface(Curve* c1,  Curve* c2) {
    Control_Point** main_cp = bspline_get_cps(c1);
    Control_Point** sub_cp = bspline_get_cps(c2);
	Surface* res = SAFE_MALLOC(Surface,1);
    initSurface(res);
    res->u_cp_len = bspline_get_cps_len(c1);
    res->v_cp_len = bspline_get_cps_len(c2);
	init_cp_grid(&res->cp_grid, res->v_cp_len, res->u_cp_len);
	for (int i = 0; i < res->v_cp_len; i++) {
		for (int j = 0; j < res->u_cp_len; j++) {
            update_grid_cp_at(res->cp_grid, cp_add(main_cp[j], sub_cp[i]), i, j);
		}
	}
	res->u_kn_list = bspline_get_knots(c1);
	res->v_kn_list = bspline_get_knots(c2);
    res->u_order = bspline_get_order(c1);
    res->v_order = bspline_get_order(c2);
    transform_grid(res->cp_grid, scaler(get_location(sub_cp[0]), -1));
    free(main_cp);
    free(sub_cp);
    toggle_cp_grid_visiblity(res->cp_grid, FALSE, FALSE, FALSE);
    return res;
}

static Control_Point** evaluateQcpArr(Surface* surf, double t, char* dir) {
    if (!(strcmp(dir, "u") == 0 || strcmp(dir, "v") == 0))
        return NULL;
    BOOL dir_flag = strcmp(dir, "u") == 0; // TRUE will be horozontal
    int res_len = dir_flag ? surf->u_cp_len : surf->v_cp_len;
    Kn_List* knots = !dir_flag ? surf->u_kn_list : surf->v_kn_list;  
    int K = !dir_flag ? surf->u_order : surf->v_order;
    Control_Point** cp_tmp;
    Control_Point** cp_res = SAFE_MALLOC(Control_Point*, res_len);
    BYTE r, g, b; 
    cagdGetColor(&r, &g, &b);
    for (int i = 0; i < res_len; i++) {
        cp_tmp = dir_flag ? get_cp_column(surf->cp_grid, i) : get_cp_row(surf->cp_grid, i);
        double weight;
        CAGD_POINT p = evaluateAtT(cp_tmp, knots, K, t, TRUE, &weight);
        cp_res[i] = cp_create(p, weight);
        free(cp_tmp);
    }
    cagdSetColor(r, g, b);
    return cp_res;
}

static void* deleteQcpArr(Control_Point** Q_points, int len) {
    for (int i = 0; i < len; i++) {
        cp_delete(Q_points[i]);
    }
    free(Q_points);
    return NULL;
}

SurfaceLinesWrapper draw_side(Surface* surf, char* dir) {
    BOOL dir_bool = strcmp(dir, "u") == 0;
    Kn_List* kn_list = dir_bool ? surf->u_kn_list : surf->v_kn_list;
    Kn_List* other_kn = !dir_bool ? surf->u_kn_list : surf->v_kn_list;
    int order = dir_bool ? surf->u_order : surf->v_order;
    int other_order = !dir_bool ? surf->u_order : surf->v_order;
    int cp_len = dir_bool ? surf->u_cp_len : surf->v_cp_len;
    int other_cp_len = !dir_bool ? surf->u_cp_len : surf->v_cp_len;
    char* other_side = !dir_bool ? "u" : "v";
    dir_bool ? cagdSetColor(0, 255, 255) : cagdSetColor(255, 255, 0);
    SurfaceLinesWrapper res;
    double domain[2];
    get_domain(other_kn, other_order, domain);
    if (domain[1] == domain[0]) {
        res.lines = NULL;
        res.time = NULL;
        res.len = 0;
        return res;
    }
    int lines = finesse;
    double t = domain[0];
    res.lines = SAFE_MALLOC(TimeStampedPolyGroup, lines+1);
    res.time = SAFE_MALLOC(double, lines+1);
    for (int line = 0; line < lines; line++, t+= (domain[1] - domain[0]) / lines) {
        Control_Point** Q_points = evaluateQcpArr(surf, t, dir);
        res.lines[line] = plot_bspline_function(Q_points, cp_len, kn_list, order, finesse/3,TRUE);
        res.time[line] = t;
        deleteQcpArr(Q_points, cp_len);
    }
    res.time[lines] = domain[1] - 0.0001;
    Control_Point** Q_points = evaluateQcpArr(surf, res.time[lines], dir);
    res.lines[lines] = plot_bspline_function(Q_points, cp_len, kn_list, order, finesse / 3, TRUE);
    deleteQcpArr(Q_points, cp_len);
    res.len = lines+1;
    return res;
}

CAGD_POINT get_point_on_surface(Surface* surf, double u, double v) {
    Control_Point** Q_points = evaluateQcpArr(surf, v, "u");
    CAGD_POINT res = evaluateAtT(Q_points, surf->u_kn_list, surf->u_order, u, TRUE, NULL);
    deleteQcpArr(Q_points, surf->u_cp_len);
    return res;
}

void drawSurface(Surface* surf) {
    int lines_num;
    SurfaceLinesWrapper lines;
    surf->dead = FALSE;
    if ((surf->v_order + surf->v_cp_len == kn_list_len(surf->v_kn_list)) && (surf->u_order + surf->u_cp_len == kn_list_len(surf->u_kn_list))) {
        lines = draw_side(surf, "v");
        emptySurfaceLinesWrapper(surf->v_polys);
        surf->v_polys = lines;
        lines = draw_side(surf, "u");
        emptySurfaceLinesWrapper(surf->u_polys);
        surf->u_polys = lines;
    }
    else {
        surf->dead = TRUE;
        colorItDead(surf);
    }
    cagdRedraw();
}

static void colorItDead(Surface* surf) {
   for (int i = 0; i < surf->u_polys.len; i++) {
       set_polyline_group_color(&surf->u_polys.lines[i], 150, 150, 150);
   }
   for (int i = 0; i < surf->v_polys.len; i++) {
       set_polyline_group_color(&surf->v_polys.lines[i], 150, 150, 150);
   }
}
void surfaceFromBsplines(struct Curve* c1, struct Curve* c2) {
    Surface* new_s = createSurface(c1, c2);
    pushSurface(new_s);
    drawSurface(new_s);
}

static void handleMenuButtons(enum Surface_Control flag) {
    SurfaceManager* manager = getSurfaceManager();
    unsigned int* bitmap = &(manager->menuBitmap);
    switch (flag) {
    case SPAN:
        if (!(*bitmap & (1 << SPAN))) {
            cagdRegisterCallback(CAGD_LBUTTONUP, connectCurvesLeftUp, (PVOID)surfaceFromBsplines);
        }
        else {
            cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
        }
        break;
    case OFSE:
        break;
    case TNGV:
        manager->draw_geo[B_TANGENTS] = !manager->draw_geo[B_TANGENTS];
        break;
    case NORM:
        manager->draw_geo[B_NORMAL] = !manager->draw_geo[B_NORMAL];
        break;
    case TNGP:
        manager->draw_geo[B_TANGENTP] = !manager->draw_geo[B_TANGENTP];
        break;
    case PDIR:
        manager->draw_geo[B_CURVATURE] = !manager->draw_geo[B_CURVATURE];
        break;
    default:
        return;
    }
    if (*bitmap & (1 << flag))
        *bitmap &= ~(1 << flag);
    else
        *bitmap |= (1 << flag);
}

void sufaceControlCenter(int id, int indx, PVOID userData) {
    SurfaceManager* manager = getSurfaceManager();
    HMENU* hMenu = (HMENU*)userData;
    switch (id) {
    case SRF_SPAN:
        handleMenuButtons(SPAN);
		break;
	case SRF_TNGV:
        handleMenuButtons(TNGV);
		break;
	case SRF_TNGP:
        handleMenuButtons(TNGP);
		break;
	case SRF_NORM:
        handleMenuButtons(NORM);
		break;
	case SRF_PDIR:
        handleMenuButtons(PDIR);
		break;
    case SRF_SLOWER:
        manager->anime_speed -= 0.0002;
        manager->anime_speed = manager->anime_speed < 0.0002 ? 0.0002 : manager->anime_speed;
        break;
    case SRF_FASTER:
        manager->anime_speed += 0.0002;
        break;
	case SRF_FIN:
		if (DialogBox(cagdGetModule(),
			MAKEINTRESOURCE(IDD_F),
			cagdGetWindow(),
			(DLGPROC)myDialogProc))
			if (sscanf(myBuffer, "%d", &finesse) != 1 || finesse <= 5) {
				myMessage("Change finesse", "Bad finesse!", MB_ICONERROR);
				return;
			}
		finesse += 1;
        //drawAllSurfaces();
		break;
	case SRF_LOAD:  
		openFileName.hwndOwner = auxGetHWND();
		openFileName.lpstrTitle = "Load File";
		if (GetOpenFileName(&openFileName)) {
            loadSurface((int)openFileName.lpstrFile, 0, (PVOID)manager);
		}
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, userData);
		break;
    case SRF_SLCT:
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, userData);
        break;
    case SRF_CREATE:
        DialogBox(cagdGetModule(), MAKEINTRESOURCE(IDD_DYNAMIC_SURFACE_CREATE), NULL, DialogProc);
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, userData);
        break;
    case SRF_CLEAR:
        cleanupSurfaceManager();
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, userData);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
        cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
        cagdRegisterCallback(CAGD_RBUTTONUP, NULL, NULL);
        break;
    }
	CheckMenuItem(hMenu, SRF_SPAN, manager->menuBitmap & (1 << SPAN) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, SRF_TNGV, manager->menuBitmap & (1 << TNGV) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, SRF_TNGP, manager->menuBitmap & (1 << TNGP) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, SRF_NORM, manager->menuBitmap & (1 << NORM) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, SRF_PDIR, manager->menuBitmap & (1 << PDIR) ? MF_CHECKED : MF_UNCHECKED);
	cagdRedraw();

}

void addSurfaceMenu(HMENU hMenu) {
    SurfaceManager* manager =getSurfaceManager();
    manager->myPopup = CreatePopupMenu();
    AppendMenu(manager->myPopup, MF_STRING, SRF_MOVE, "Move");
    AppendMenu(manager->myPopup, MF_STRING, SRF_WEIGHT, "Edit Weights");
    AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(manager->myPopup, MF_STRING, SRF_KNOTSU, "Edit U Knots");
    AppendMenu(manager->myPopup, MF_STRING, SRF_KNOTSV, "Edit V Knots");
    AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(manager->myPopup, MF_STRING, SRF_INKNOTU, "insert U Knots");
    AppendMenu(manager->myPopup, MF_STRING, SRF_INKNOTV, "insert V Knots");
    AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(manager->myPopup, MF_STRING, SRF_CLICK, "Draw Differential Geo");
    AppendMenu(manager->myPopup, MF_STRING, SRF_ANIM, "Draw Differential Anime");
    AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(manager->myPopup, MF_STRING | MF_DISABLED, SRF_OFFSET, "Offset");
    AppendMenu(manager->myPopup, MF_STRING | MF_DISABLED, SRF_HOFF, "Offset by H");
    AppendMenu(manager->myPopup, MF_STRING | MF_DISABLED, SRF_KOFF, "Offset by K");
    AppendMenu(manager->myPopup, MF_STRING | MF_DISABLED, SRF_K1OFF, "Offset by k1");
    AppendMenu(manager->myPopup, MF_STRING | MF_DISABLED, SRF_K2OFF, "Offset by k2");
    AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(manager->myPopup, MF_STRING, SRF_DEGU, "Edit U Degree");
    AppendMenu(manager->myPopup, MF_STRING, SRF_DEGV, "Edit V Degree");
    AppendMenu(manager->myPopup, MF_STRING, SRF_UNIOPU, "U Uniform Open");
    AppendMenu(manager->myPopup, MF_STRING, SRF_UNIOPV, "V Uniform Open");
    AppendMenu(manager->myPopup, MF_STRING, SRF_UNICU, "U Uniform Float");
    AppendMenu(manager->myPopup, MF_STRING, SRF_UNICV, "V Uniform Float");
    AppendMenu(manager->myPopup, MF_SEPARATOR, 0, NULL);
    AppendMenu(manager->myPopup, MF_STRING, SRF_REMOVES, "Remove Surface");

    manager->myKnotPopup = CreatePopupMenu();
    AppendMenu(manager->myKnotPopup, MF_STRING, SRF_ADDKN, "Add Knot");
    AppendMenu(manager->myKnotPopup, MF_STRING, SRF_RMKN, "Remove Knot");

    AppendMenu(hMenu, MF_STRING, SRF_SLCT, "Surface Select");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, SRF_LOAD, "Load Surface");
    AppendMenu(hMenu, MF_STRING, SRF_CREATE, "Create Surface");
	AppendMenu(hMenu, MF_STRING, SRF_SPAN, "Span Surface from splines");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, SRF_TNGV, "Add Tangent Vectors");
    AppendMenu(hMenu, MF_STRING, SRF_TNGP, "Add Tangent Plane");
    AppendMenu(hMenu, MF_STRING, SRF_NORM, "Add Surface Normal");
    AppendMenu(hMenu, MF_STRING, SRF_PDIR, "Add Principle Curviture");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, SRF_FASTER, "Faster Animation");
    AppendMenu(hMenu, MF_STRING, SRF_SLOWER, "Slower Animation");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, SRF_FIN, "Finesse");
    AppendMenu(hMenu, MF_STRING, SRF_CLEAR, "Clear all Surface");
    manager->myMenu = hMenu;
	cagdAppendMenu(hMenu, "Surfaces");
}

void emptySurfaceLinesWrapper(SurfaceLinesWrapper polys) {
    for (int i = 0; i < polys.len; i++) emptyStampedPolyGroup(&polys.lines[i]);
    if(polys.lines) free(polys.lines);
    if(polys.time) free(polys.time);
    polys.lines = NULL;
    polys.time = NULL;
    polys.len = 0;
}


BOOL where_in_surface(Surface* surf,int poly_id, UINT vertex_id, double* u, double *v) {
    if (poly_id == -1)
        return FALSE;
    int poly_index; 
    for (int i = 0; i < surf->u_polys.len; i++) if ((poly_index = containsPoly(surf->u_polys.lines[i], poly_id)) != -1) {
        *v = surf->u_polys.time[i];
        if ((*u = get_time(surf->u_polys.lines[i].segments[poly_index], vertex_id)) != -1)
            return TRUE;
    }
    for (int i = 0; i < surf->v_polys.len; i++) if ((poly_index = containsPoly(surf->v_polys.lines[i], poly_id)) != -1) {
        *u = surf->v_polys.time[i];
        if ((*v = get_time(surf->v_polys.lines[i].segments[poly_index], vertex_id)) != -1)
            return TRUE;
    }
    return FALSE;
}

BOOL is_part_of_surface(Surface* surf, UINT id) {
    for (int i = 0; i < surf->u_polys.len; i++) if (containsPoly(surf->u_polys.lines[i], id) != -1) return TRUE;
    for (int i = 0; i < surf->v_polys.len; i++) if (containsPoly(surf->v_polys.lines[i], id) != -1) return TRUE;
    return FALSE;
}

Surface* findSurface(int id) {
    if (id == -1) return NULL;
    SurfaceManager* manager = getSurfaceManager();
    for (int i = 0; i < manager->count; i++) {
        if (is_part_of_surface(manager->surfaces[i], id))
            return manager->surfaces[i];
    }
    return NULL;
}


BOOL getSurfaceUVFromClick(Surface* surf, int x, int y, int* u, int* v) {
    UINT id, min_v, min_id = -1;
    CAGD_POINT p;
    GLdouble min_dist = 0x7ff0000000000000;
    for (cagdPick(x, y); id = cagdPickNext();)
        if (cagdGetSegmentType(id) == CAGD_SEGMENT_POLYLINE) {
            if (is_part_of_surface(surf, id)) {
                UINT vertix;
                GLdouble temp_dist;
                if (vertix = cagdGetNearestVertex(id, x, y)) {
                    cagdGetVertex(id, --vertix, &p);
                    int X, Y;
                    cagdToWindow(&p, &X, &Y);
                    temp_dist = (X - x) * (X - x) + (Y - y) * (Y - y);
                    if (temp_dist < min_dist) {
                        min_id = id;
                        min_v = vertix;
                        min_dist = temp_dist;
                    }
                }
            }
        }
    return where_in_surface(surf, min_id, min_v, u, v);
}

Surface* getClosestSurface(int x, int y) {
    UINT id, min_id = -1;
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
                if (temp_dist < min_dist) {
                    min_id = id;
                    min_dist = temp_dist;
                }
            }
        }
    return findSurface(min_id);
}

static BOOL toggleShowOrHideControl(Surface* surf) {
    if (!surf)
        return FALSE;
    SurfaceManager* manager = getSurfaceManager();
    BOOL isLastClicked = (manager->last_clicked != NULL && surf == manager->last_clicked);
    if (!isLastClicked)
        toggleShowOrHideControl(manager->last_clicked);

    if (isLastClicked) // hide
    {
        toggle_cp_grid_visiblity(surf->cp_grid, FALSE, FALSE, FALSE);
        manager->last_clicked = NULL;
        cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
        cagdRedraw();
        return FALSE;
    }
    else { // show
        manager->last_clicked = surf;
        toggle_cp_grid_visiblity(surf->cp_grid, TRUE, TRUE, surf->showWeights);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, editSurfaceCPLeftDown, surf);
        cagdRedraw();
        return TRUE;
    }

}

static void defaultSurfaceLeftClick(int x, int y, PVOID userData)
{
    SurfaceManager* manager = getSurfaceManager();
    Surface* surf = manager->last_clicked ? manager->last_clicked : getClosestSurface(x, y);
    if (surf) {
        if (toggleShowOrHideControl(surf)) {
            cagdRegisterCallback(CAGD_RBUTTONUP, defaultSurfaceRightUp, userData);
        }
    }
}

void handleSurfacePopupButtons(Surface* surf, enum popUpflag flag) {
    unsigned int* bitmap = &getSurfaceManager()->popUpBitMap;
    BOOL toActivate = !((*bitmap & (1 << flag)));
    if (*bitmap & (1 << FLAG_WIEGHT)) {
        surfaceWeightsHandler(surf, FALSE);
        *bitmap ^= (1 << FLAG_WIEGHT);
    }
    if (*bitmap & (1 << FLAG_EDKNOTU)) {
        knotsHandler(surf, FALSE, "u");
        *bitmap ^= (1 << FLAG_EDKNOTU);
    }
    if (*bitmap & (1 << FLAG_EDKNOTV)) {
        knotsHandler(surf, FALSE, "v");
        *bitmap ^= (1 << FLAG_EDKNOTV);
    }
    if (*bitmap & (1 << FLAG_INSKNOTU)) {
        inknotsSurfaceHandler(surf, FALSE, "u");
        *bitmap ^= (1 << FLAG_INSKNOTU);
    }
    if (*bitmap & (1 << FLAG_INSKNOTV)) {
        inknotsSurfaceHandler(surf, FALSE, "v");
        *bitmap ^= (1 << FLAG_INSKNOTV);
    }
    if (*bitmap & (1 << FLAG_CLICK)) {
        *bitmap ^= (1 << FLAG_CLICK);
        drawShapesClickHandler(surf, FALSE);
    }
    if (*bitmap & (1 << FLAG_ANIME)) {
        *bitmap ^= (1 << FLAG_ANIME);
        drawShapesAnimeHandler(surf, FALSE);
    }
    if (*bitmap & (1 << FLAG_MOVE)) {
        moveSurfaceHandler(surf, FALSE);
        *bitmap ^= (1 << FLAG_MOVE);
    }
    if (flag == FLAG_REMOVE) {
        removeSurface(surf);
        return;
    }
    if (toActivate) {
        switch (flag) {
        case FLAG_MOVE:
            moveSurfaceHandler(surf, TRUE);
            break;
        case FLAG_WIEGHT:
            surfaceWeightsHandler(surf, TRUE);
            break;
        case FLAG_EDKNOTU:
            knotsHandler(surf, TRUE, "u");
            break;
        case FLAG_EDKNOTV:
            knotsHandler(surf, TRUE, "v");
            break;
        case FLAG_CLICK:
            drawShapesClickHandler(surf, TRUE);
            break;
        case FLAG_ANIME:
            drawShapesAnimeHandler(surf, TRUE);
            break;
        case FLAG_INSKNOTV:
            inknotsSurfaceHandler(surf, TRUE,"v");
            break;
        case FLAG_INSKNOTU:
            inknotsSurfaceHandler(surf, TRUE, "u");
            break;
        case FLAG_REMOVE:
            break;
        }
        *bitmap |= (1 << flag);
    }
}
static void defaultSurfaceRightUp(int x, int y, PVOID userData)
{
    SurfaceManager* manager = getSurfaceManager();
    Surface* surf = manager->last_clicked;
    if (!surf)
        return;
    HMENU hMenu = manager->myPopup;

    EnableMenuItem(hMenu, SRF_MOVE, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_ANIM, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_WEIGHT, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_INKNOTU, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_INKNOTV, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_CLICK, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_UNICU, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_UNICV, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_UNIOPU, surf->dead ? MF_DISABLED : MF_ENABLED);
    EnableMenuItem(hMenu, SRF_UNIOPV, surf->dead ? MF_DISABLED : MF_ENABLED);

    CheckMenuItem(hMenu, SRF_MOVE, (manager->popUpBitMap & (1 << FLAG_MOVE)) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, SRF_ANIM, (manager->popUpBitMap & (1 << FLAG_ANIME)) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, SRF_WEIGHT, (manager->popUpBitMap & (1 << FLAG_WIEGHT)) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, SRF_KNOTSU, (manager->popUpBitMap & (1 << FLAG_EDKNOTU)) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, SRF_KNOTSV, (manager->popUpBitMap & (1 << FLAG_EDKNOTV)) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, SRF_INKNOTU, (manager->popUpBitMap & (1 << FLAG_INSKNOTU)) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, SRF_INKNOTV, (manager->popUpBitMap & (1 << FLAG_INSKNOTV)) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(hMenu, SRF_CLICK, (manager->popUpBitMap & (1 << FLAG_CLICK)) ? MF_CHECKED : MF_UNCHECKED);

    switch (cagdPostMenu(hMenu, x, y)) {
    case SRF_MOVE:
        handleSurfacePopupButtons(surf, FLAG_MOVE);
        break;
    case SRF_WEIGHT:
        handleSurfacePopupButtons(surf, FLAG_WIEGHT);
        break;
    case SRF_REMOVES:
        handleSurfacePopupButtons(surf, FLAG_REMOVE);
        break;
    case SRF_KNOTSU:
        handleSurfacePopupButtons(surf, FLAG_EDKNOTU);
        break;
    case SRF_KNOTSV:
        handleSurfacePopupButtons(surf, FLAG_EDKNOTV);
        break;
    case SRF_CLICK:
        handleSurfacePopupButtons(surf, FLAG_CLICK);
        break;
    case SRF_ANIM:
        handleSurfacePopupButtons(surf, FLAG_ANIME);
        break;
    case SRF_HOFF:
        offset_surface(surf, HOFF);
        break;
    case SRF_K1OFF:
        offset_surface(surf, K1OF);
        break;
    case SRF_K2OFF:
        offset_surface(surf, K2OF);
        break;
    case SRF_KOFF:
        offset_surface(surf, KOFF);
        break;
    case SRF_OFFSET:
        offset_surface(surf, OFSE);
        break;
    case SRF_INKNOTU:
        handleSurfacePopupButtons(surf, FLAG_INSKNOTU);
        break;
    case SRF_INKNOTV:
        handleSurfacePopupButtons(surf, FLAG_INSKNOTV);
        break;
    case SRF_DEGU:
        if (DialogBox(cagdGetModule(),
            MAKEINTRESOURCE(IDD_T2),
            cagdGetWindow(),
            (DLGPROC)myDialogProc))
            if (sscanf(myBuffer, "%d", &surf->u_order) != 1 || surf->u_order <= 1) {
                myMessage("Change degree", "Unacceptable degree (should by deg >= 2) !", MB_ICONERROR);
                return;
            }
        drawSurface(surf);
        break;
    case SRF_DEGV:
        if (DialogBox(cagdGetModule(),
            MAKEINTRESOURCE(IDD_T2),
            cagdGetWindow(),
            (DLGPROC)myDialogProc))
            if (sscanf(myBuffer, "%d", &surf->v_order) != 1 || surf->v_order <= 1) {
                myMessage("Change degree", "Unacceptable degree (should by deg >= 2) !", MB_ICONERROR);
                return;
            }
        drawSurface(surf);
        break;
    case SRF_UNICU:
        remake_as_closed_uni(&surf->u_kn_list, surf->u_cp_len, surf->u_order);
        drawSurface(surf);
        break;
    case SRF_UNICV:
        remake_as_closed_uni(&surf->v_kn_list, surf->v_cp_len, surf->v_order);
        drawSurface(surf);
        break;
    case SRF_UNIOPU:
        remake_as_open_uni(&surf->u_kn_list, surf->u_cp_len, surf->u_order);
        drawSurface(surf);
        break;
    case SRF_UNIOPV:
        remake_as_open_uni(&surf->v_kn_list, surf->v_cp_len, surf->v_order);
        drawSurface(surf);
        break;
    }
    cagdRedraw();
}

static void move_surface(Surface* surf, CAGD_POINT delta) {
    transform_grid(surf->cp_grid, delta);
    drawSurface(surf);
}

static void moveSurfaceMove(int x, int y, PVOID userData)
{
    SurfaceManager* manager = getSurfaceManager();
    Surface* surf = (Surface*)userData;
    CAGD_POINT q[2];
    cagdToObject(x, y, q);
    q[1] = manager->moving_ref;
    if (q[0].x - q[1].x != 0 || q[0].y - q[1].y != 0 || q[0].z - q[1].z != 0) {
        move_surface(surf, (CAGD_POINT) { q[0].x - q[1].x, q[0].y - q[1].y, q[0].z - q[1].z });
        manager->moving_ref = q[0];
    }
}

static void moveSurfaceLeftUp(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    toggleShowOrHideControl(surf);
    cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
    cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
    cagdRegisterCallback(CAGD_LBUTTONDOWN, moveSurfaceLeftDown, (PVOID)surf);

}

static void moveSurfaceLeftDown(int x, int y, PVOID userData)
{
    SurfaceManager* manager = getSurfaceManager();
    Surface* surf = (Surface*)userData;
    CAGD_POINT q[2];
    cagdToObject(x, y, q);
    manager->moving_ref = q[0];
    toggleShowOrHideControl(surf);
    cagdRegisterCallback(CAGD_MOUSEMOVE, moveSurfaceMove, (PVOID)surf);
    cagdRegisterCallback(CAGD_LBUTTONUP, moveSurfaceLeftUp, (PVOID)surf);
    cagdRedraw();
}

static void moveSurfaceHandler(Surface* surf, BOOL flag) {
    if (flag)
        cagdRegisterCallback(CAGD_LBUTTONDOWN, moveSurfaceLeftDown, (PVOID)surf);
    else {
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, editSurfaceCPLeftDown, (PVOID)surf);
    }
}
static void editSurfaceCPMove(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    Control_Point* cp = get_cp_grid(surf->cp_grid, surf->mp_vi, surf->mp_ui);
    if (cp) {
        CAGD_POINT q[2], p = get_location(cp);
        cagdToObjectAtDepth(x, y, p, &q);
        update_cp(cp, q[1]);
        update_grid_cp_at(surf->cp_grid, cp, surf->mp_vi, surf->mp_ui);
    }
    drawSurface(surf);
}
static void editSurfaceCPLeftUp(int x, int y, PVOID userData)
{
    cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
    cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, userData);
}

static void editSurfaceCPLeftDown(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    Control_Point* cp = getClosestControlPoint(surf, x, y, &surf->mp_ui, &surf->mp_vi);
    if (cp) {
        cagdRegisterCallback(CAGD_MOUSEMOVE, editSurfaceCPMove, (PVOID)surf);
        cagdRegisterCallback(CAGD_LBUTTONUP, editSurfaceCPLeftUp, (PVOID)NULL);
    }
}

static Control_Point* getClosestControlPoint(Surface* surf, int x, int y, int *u_i, int* v_i) {
    UINT id;
    Control_Point* tmp, * res = NULL;
    int res_ui = -1, res_vi = -1;
    CAGD_POINT p;
    GLdouble min_dist = 100000000000;
    for (cagdPick(x, y); id = cagdPickNext();) {
        if (cagdGetSegmentType(id) == CAGD_SEGMENT_POINT) {
            if (tmp = get_cp_by_id_grid(surf->cp_grid, id, v_i, u_i)) {
                GLdouble temp_dist;
                cagdGetSegmentLocation(id, &p);
                temp_dist = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
                if (temp_dist < min_dist) {
                    res = tmp;
                    res_ui = *u_i;
                    res_vi = *v_i;
                }
            }
        }
    }
    *u_i =res_ui;
    *v_i =res_vi;
    return res;
}

static void editSurfaceWeightsMove(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    update_weight_from_window(surf->movingPoint, x, y);
    drawSurface(surf);
}
static void editSurfaceWeightsLeftUp(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    surf->movingPoint = NULL;
    surf->mp_ui = surf->mp_vi = -1;
    cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
    cagdRedraw();
}
static void editSurfaceWeightsLeftDown(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    Control_Point* cp = getClosestControlPoint(surf, x, y, &surf->mp_ui, &surf->mp_vi);
    if (cp) {
        surf->movingPoint = cp;
        cagdRegisterCallback(CAGD_MOUSEMOVE, editSurfaceWeightsMove, (PVOID)surf);
        cagdRegisterCallback(CAGD_LBUTTONUP, editSurfaceWeightsLeftUp, (PVOID)surf);
    }
}
void surfaceWeightsHandler(Surface* surf, BOOL flag) {
    if (flag) {
        surf->showWeights = TRUE;
        cagdRegisterCallback(CAGD_LBUTTONDOWN, editSurfaceWeightsLeftDown, (PVOID)surf);
        cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
        toggle_cp_grid_visiblity(surf->cp_grid, TRUE, TRUE, surf->showWeights);
    }
    else {
        surf->showWeights = FALSE;
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, editSurfaceCPLeftDown, (PVOID)surf);
        toggle_cp_grid_visiblity(surf->cp_grid, TRUE, TRUE, surf->showWeights);
    }
}
static void updateSurfaceOnKnotsMove(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    Kn_List* kn_list = strcmp(surf->dir, "u") == 0 ? surf->u_kn_list : surf->v_kn_list;
    updateKnotsMove(x,y,(PVOID)kn_list);
    drawSurface(surf);
    cagdRedraw();
}

static void updateKnotsRightUp(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    Kn_List* kn_list = strcmp(surf->dir, "u") == 0 ? surf->u_kn_list : surf->v_kn_list;
    HMENU hMenu = getSurfaceManager()->myKnotPopup;
    Knot* knot = get_knot_from_indicator(kn_list, x, y);
    if (knot) {
        switch (cagdPostMenu(hMenu, x, y)) {
        case SRF_ADDKN:
            add_knot(knot);
            update_single_KVector(knot);
            break;
        case SRF_RMKN:
            if (!kn_list_remove(knot))
                update_single_KVector(knot);
            break;
        }
        drawSurface(surf);
    }
    else {
        defaultSurfaceRightUp(x, y, (PVOID)NULL);
    }
}

static void updateKnotsLeftDown(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    Kn_List* kn_list = strcmp(surf->dir, "u") == 0 ? surf->u_kn_list : surf->v_kn_list;
    Knot* knot = get_knot_from_indicator(kn_list, x, y);
    if (knot) {
        set_moving_knot(knot);
        cagdRegisterCallback(CAGD_MOUSEMOVE, updateSurfaceOnKnotsMove, (PVOID)surf);
        cagdRegisterCallback(CAGD_LBUTTONUP, updateKnotsLeftUp, (PVOID)kn_list);
    }
    else {
        cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
        cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
    }
}


static void knotsHandler(Surface* surf, BOOL flag, char* dir) {
    surf->dir = dir;
    Kn_List* kn_list = strcmp(surf->dir, "u") == 0 ? surf->u_kn_list : surf->v_kn_list;
    if (flag) {
        draw_KV(kn_list);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, updateKnotsLeftDown, (PVOID)surf);
        cagdRegisterCallback(CAGD_RBUTTONUP, updateKnotsRightUp, (PVOID)surf);
        cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
    }
    else {
        erase_KV(kn_list);
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, editSurfaceCPLeftDown, (PVOID)surf);
        cagdRegisterCallback(CAGD_RBUTTONUP, defaultSurfaceRightUp, (PVOID)NULL);
    }
}


static void insertKnotsMove(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    BOOL dir = strcmp(surf->dir, "u") == 0;
    Kn_List* kn_list = dir ? surf->u_kn_list : surf->v_kn_list;
    double value = updateKnotsMove(x, y, (PVOID)kn_list);
    remove_from_moving_knot(kn_list);
    int order = dir ? surf->u_order : surf->v_order;
    value = bound_to_domain(kn_list, order, value);
    cagdSetColor(255, 0, 0);
    emptyStampedPolyGroup(&surf->insertionIndicator);
    surf->insertionIndicator = draw_line_at_value(surf, value, surf->dir);
    Knot* mkn = kn_list_insert(kn_list, value, 1);
    set_moving_knot(mkn);
    update_single_KVector(mkn);
    set_moving_knot_color(kn_list, 255, 0, 0);
    cagdRedraw();
}


static void insertKnotsLeftUp(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    BOOL dir = strcmp(surf->dir, "u") == 0;
    Kn_List* kn_list = dir ? surf->u_kn_list : surf->v_kn_list;
    double value = get_value(get_moving_knot(kn_list));
    value = bound_to_domain(kn_list, dir ? surf->u_order : surf->v_order, value);
    remove_from_moving_knot(kn_list);
    emptyStampedPolyGroup(&surf->insertionIndicator);
    knotInsertionSurfaceWrapper(surf, value);
    cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
    drawSurface(surf);
    cagdRedraw();
}

void knotInsertionSurfaceWrapper(Surface* surf, double t) {
    BOOL dir = strcmp(surf->dir, "u") == 0;
    Kn_List* kn_list = dir ? surf->u_kn_list : surf->v_kn_list;
    int K = dir ? surf->u_order : surf->v_order;
    int other_cp_len = dir ? surf->v_cp_len : surf->u_cp_len;
    Control_Point*** updated_points = SAFE_MALLOC(Control_Point**, other_cp_len);
    int start_col; 
    Knot* inserted;
    for (int i = 0; i < other_cp_len; i++) {
        Control_Point** cp_arr = dir ? get_cp_row(surf->cp_grid, i) : get_cp_column(surf->cp_grid, i);
        updated_points[i] = generalKnotInsertion(cp_arr, kn_list, K, t, &start_col, &inserted);
        kn_list_remove(inserted);
        free(cp_arr);
    }
    update_grid_block(surf->cp_grid, updated_points, other_cp_len, K + 1, 0, start_col, surf->dir);
    update_single_KVector(kn_list_insert(kn_list, t, 1));
    for (int i = 0; i < other_cp_len; i++) {
        free(updated_points[i]);
    }
    dir ? surf->u_cp_len++ : surf->v_cp_len++;
    free(updated_points);
}

static Control_Point** generalKnotInsertion(Control_Point** cp_array,Kn_List* kn_list, int K, double value, int* change_s, Knot** inserted) {
    int l_lower, l_upper, l;
    l_lower = kn_list_lower_index(kn_list, value);
    l_upper = kn_list_upper_index(kn_list, value);
    l = l_lower;
    if (l - K < 0 || l > kn_list_len(kn_list) - K)
    {
        l = l_upper;
        if (l - K < 0 || l > kn_list_len(kn_list) - K)
            return;
    }
    *inserted = kn_list_insert(kn_list, value, 1);
    update_single_KVector(*inserted);
    CAGD_POINT i_0, i_1;
    Control_Point* curr_cp = cp_array[l - K];
    Control_Point* prev_cp = l - K > 0 ? cp_array[l - K - 1] : NULL;
    double w_0, w_1 = get_weight(curr_cp);
    if (prev_cp) {
        i_0 = get_location(prev_cp);// i-1
        w_0 = get_weight(prev_cp);
    }
    double* knot_range = get_knot_range(kn_list, l - K, 2 * K);
    Control_Point** update_arr = SAFE_MALLOC(Control_Point*, K+1);
    for (int i = 0; i < K; i++) {
        double t_i = knot_range[i];
        double t_k = knot_range[i + K];
        i_1 = get_location(curr_cp); // i
        w_1 = get_weight(curr_cp);
        if (!prev_cp) {
            i_0 = i_1;
            w_0 = w_1;
        }
        double alpha = (value - t_i) / (t_k - t_i);
        double new_weight = alpha * w_1 + (1 - alpha) * w_0;
        update_arr[i] = cp_create(cagdPointAdd(scaler(i_1, w_1 * alpha / new_weight), scaler(i_0, w_0*(1 - alpha) / new_weight)), new_weight);
        i_0 = i_1;
        w_0 = w_1;
        prev_cp = curr_cp;
        if(i != K - 1) curr_cp = cp_array[l - K + i + 1];
    }
    update_arr[K] = cp_create(get_location(curr_cp), get_weight(curr_cp));
    *change_s = l - K;
    return update_arr;
}

static TimeStampedPolyGroup draw_line_at_value(Surface* surf, double value, char* dir) {
    BOOL dir_bool = strcmp(dir, "u") == 0;
    Kn_List* kn_list = dir_bool ? surf->u_kn_list : surf->v_kn_list;
    Kn_List* other_kn_list = dir_bool ? surf->v_kn_list : surf->u_kn_list;
    int order = dir_bool ? surf->u_order : surf->v_order;
    int other_order = dir_bool ? surf->v_order : surf->u_order;
    int cp_len = dir_bool ? surf->u_cp_len : surf->v_cp_len;
    int other_cp_len = dir_bool ? surf->v_cp_len : surf->u_cp_len;
    char* other_dir = dir_bool ? "v" : "u";
    Control_Point** Q_points = evaluateQcpArr(surf, value, other_dir);
    TimeStampedPolyGroup res = plot_bspline_function(Q_points, other_cp_len, other_kn_list, other_order, finesse / 3, TRUE);
    deleteQcpArr(Q_points, other_cp_len); 
    return res;
}
static void insertKnotsLeftDown(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    BOOL dir = strcmp(surf->dir, "u") == 0;
    Kn_List* kn_list = dir ? surf->u_kn_list : surf->v_kn_list;
    int order = dir ? surf->u_order : surf->v_order;
    double value = knot_vector_click_to_value(kn_list, x, y);
    value = bound_to_domain(kn_list, order, value);
    cagdSetColor(255, 0, 0);
    emptyStampedPolyGroup(&surf->insertionIndicator);
    surf->insertionIndicator = draw_line_at_value(surf, value, surf->dir);
    set_moving_knot(kn_list_insert(kn_list, value, 1));
    cagdRegisterCallback(CAGD_MOUSEMOVE, insertKnotsMove, userData);
    cagdRegisterCallback(CAGD_LBUTTONUP, insertKnotsLeftUp, userData);
}


void inknotsSurfaceHandler(Surface* surf, BOOL flag, char* dir) {
     surf->dir = dir;
    Kn_List* kn_list = strcmp(dir, "u") ? surf->v_kn_list : surf->u_kn_list;
    if (flag) {
        draw_KV(kn_list);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, insertKnotsLeftDown, (PVOID)surf);
        cagdRegisterCallback(CAGD_LBUTTONUP, NULL, NULL);
    }
    else {
        erase_KV(kn_list);
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick,NULL);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, editSurfaceCPLeftDown, (PVOID)surf);
    }

}


CAGD_POINT calculateNormal(CAGD_POINT Su, CAGD_POINT Sv) {
    CAGD_POINT normal;
    normal = cross_prod(Su, Sv);
    return normal;
}

#include <stdio.h>
#include <math.h>

// Function prototypes
double determinant(double a, double b, double c, double d);
void inverseMatrix(double I[2][2], double I_inv[2][2]);
void multiplyMatrices(double A[2][2], double B[2][2], double C[2][2]);
void computeEigenvalues(double S[2][2], double* k1, double* k2);
void computeEigenvector(double S[2][2], double kappa, double v[2]);
void compute3DVector(double v2D[2], CAGD_POINT r_u, CAGD_POINT r_v, CAGD_POINT* v3D);

// Function to compute the determinant of a 2x2 matrix
double determinant(double a, double b, double c, double d) {
    return a * d - b * c;
}

// Function to compute the inverse of a 2x2 matrix
void inverseMatrix(double I[2][2], double I_inv[2][2]) {
    double det = determinant(I[0][0], I[0][1], I[1][0], I[1][1]);
    if (det == 0) {
        printf("Matrix is singular, cannot compute inverse.\n");
        return;
    }
    I_inv[0][0] = I[1][1] / det;
    I_inv[0][1] = -I[0][1] / det;
    I_inv[1][0] = -I[1][0] / det;
    I_inv[1][1] = I[0][0] / det;
}

// Function to multiply two 2x2 matrices
void multiplyMatrices(double A[2][2], double B[2][2], double C[2][2]) {
    C[0][0] = A[0][0] * B[0][0] + A[0][1] * B[1][0];
    C[0][1] = A[0][0] * B[0][1] + A[0][1] * B[1][1];
    C[1][0] = A[1][0] * B[0][0] + A[1][1] * B[1][0];
    C[1][1] = A[1][0] * B[0][1] + A[1][1] * B[1][1];
}

// Function to compute the eigenvalues of a 2x2 matrix
void computeEigenvalues(double S[2][2], double* k1, double* k2) {
    double trace = S[0][0] + S[1][1];
    double det = determinant(S[0][0], S[0][1], S[1][0], S[1][1]);
    double discriminant = sqrt(trace * trace - 4 * det);
    *k1 = (trace + discriminant) / 2;
    *k2 = (trace - discriminant) / 2;
}

// Function to compute the eigenvector corresponding to a given eigenvalue
void computeEigenvector(double S[2][2], double kappa, double v[2]) {
    double a = S[0][0] - kappa;
    double b = S[0][1];
    double c = S[1][0];
    double d = S[1][1] - kappa;

    // Solve the system (a * v1 + b * v2 = 0 or c * v1 + d * v2 = 0)
    if (fabs(a) > fabs(c)) {
        v[0] = -b;
        v[1] = a;
    }
    else {
        v[0] = -d;
        v[1] = c;
    }

    // Normalize the vector
    double norm = sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] /= norm;
    v[1] /= norm;
}

// Function to convert a 2D vector to a 3D vector using tangent basis
void compute3DVector(double v2D[2], CAGD_POINT r_u, CAGD_POINT r_v, CAGD_POINT* v3D) {
    *v3D = normalize(cagdPointAdd(scaler(r_u, v2D[0]), scaler(r_v, v2D[1])));
}

void calculatePrincipalCurvaturesAndDirections(Surface* surf, double u, double v, CAGD_POINT Su, CAGD_POINT Sv, CAGD_POINT normal, double* k1, double* k2, CAGD_POINT* d1, CAGD_POINT* d2) {
    // Calculate the first and second fundamental forms
    double E, F, G, L, M, N;
    calculateFirstFundamentalForm(Su, Sv, &E, &F, &G);
    calculateSecondFundamentalForm(surf, u, v, normal, &L, &M, &N);
    double I[2][2] = { {E, F}, {F, G} };
    double II[2][2] = { {L, M}, {M, N} };
    double I_inv[2][2];
    inverseMatrix(I, I_inv);
    double S[2][2];
    multiplyMatrices(I_inv, II, S);
    computeEigenvalues(S, k1, k2);
    // Compute the principal directions (eigenvectors in 2D)
    double v1_2D[2], v2_2D[2];
    computeEigenvector(S, *k1, v1_2D);
    computeEigenvector(S, *k2, v2_2D);
    // Convert 2D principal directions to 3D
    compute3DVector(v1_2D, Su, Sv, d1);
    compute3DVector(v2_2D, Su, Sv, d2);
}

CAGD_POINT* evaluateBsplinePointDerivatives(Control_Point** cp_array, Kn_List* knots,int J, int K, int deriv_deg) {
    int k = K - 1;
    CAGD_POINT** points = SAFE_MALLOC(CAGD_POINT*, deriv_deg + 1);
    for (int i = 0; i <= deriv_deg; i++) {
        points[i] = SAFE_MALLOC(CAGD_POINT, K);
    }
    for (int i = 0; i < K; i++) {
        points[0][i] = scaler(get_location(cp_array[J - k + i]), get_weight(cp_array[J - k + i]));
    }
    double* knotRange = get_knot_range(knots, J - k, 2*k+1);
    for (int j = 1; j <= deriv_deg; j++) {
        double row_mult = K - j;
        for (int i = 0; i < j; i++) {
            points[j][i] = (CAGD_POINT){ 0,0,0 };
        }
        for (int i = j; i < K; i++) {
            double denom = knotRange[i + K - j] - knotRange[i];
            if(denom != 0)
                points[j][i] = scaler(cagdPointSub(points[j - 1][i], points[j - 1][i - 1]), row_mult / denom);
            else
                points[j][i] = (CAGD_POINT){ 0,0,0 };
        }
    }
    double* res = points[deriv_deg];
    for (int i = 0; i < deriv_deg; i++)
        free(points[i]);
    free(points);
    free(knotRange);
    return res;
}
// Function to evaluate the partial derivative of the surface
CAGD_POINT evaluatePartialDerivatives(Control_Point** cp_array, Kn_List* knots, int K, double param, char* dir, int deriv_deg) {
    int J = evaluateJ(knots, param);
    double** basis_functions = SAFE_MALLOC(double*, deriv_deg + 1);
    for (int i = 0; i <= deriv_deg; i++) {
        basis_functions[i] = evaluateBSplineBasisDerivatives(knots, param, J, K, i);
    }
    CAGD_POINT derivative = { 0.0, 0.0, 0.0 };
    CAGD_POINT nom[3] = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } };
    double denom[3] = { 0.0, 0.0,0.0 };
    for (int i = 0; i < K; i++) {
        Control_Point* cp = cp_array[J - (K - 1) + i];
        CAGD_POINT p = get_location(cp);
        double weight = get_weight(cp);
        for (int j = 0; j <= deriv_deg; j++) {
            double mult = basis_functions[j][i] * weight;
            nom[j] = cagdPointAdd(nom[j], scaler(p, mult));
            denom[j] += mult;
        }
    }
    double denom_power_2 = denom[0] * denom[0];
    switch (deriv_deg) {
    case 0:
        if (fabs(denom[0]) > 0.0001)
            derivative = scaler(nom[0], 1.0 / denom[0]);
        break;
    case 1:
        if (fabs(denom_power_2) > 0.0001)
            derivative = scaler(cagdPointSub(scaler(nom[1], denom[0]) , scaler(nom[0], denom[1])), 1/ denom_power_2);
        break;
    case 2:
        if(fabs(denom_power_2* denom_power_2)>0.0001)
            derivative =  scaler(
                            cagdPointSub(
                                scaler(cagdPointSub(scaler(nom[2], denom[0]),scaler(nom[0],denom[2])), denom_power_2) // (d2N/duu * D + N*d2D/duu)*D^2
                               ,scaler(cagdPointSub(scaler(nom[1], denom[0]), scaler(nom[0], denom[1])),2*denom[1]*denom[0])) // (2*(dD/du)*(dN/du * D + N*dD/du))/D
                          , 1 / (denom_power_2 * denom_power_2)
                          );
        break;
    }
    for (int i = 0; i <= deriv_deg; i++) {
        free(basis_functions[i]);
    }
    free(basis_functions);
    return derivative;
}

double* evaluateBSplineBasisDerivatives(Kn_List* kn_list, double t, int J, int K, int deriv_deg) {
    int k = K - 1;
    double* knotRange = get_knot_range(kn_list, J - k, 2 * k + 1);
    double* last_base_fun = evaluateBsplineBaseFunctions(kn_list, t, J, K, deriv_deg);
    double* last_deriv_basis = SAFE_MALLOC(double, K + 1);
    for (int i = 0; i < K; i++) {
        last_deriv_basis[i] = last_base_fun[i];
    }
    free(last_base_fun);
    last_deriv_basis[K] = 0;
    for (int k_level = deriv_deg; k_level > 0; k_level--) {
        k = K - k_level; 
        for (int i = 0; i < K; i++) {
            double denom1 = (knotRange[i + k] - knotRange[i]);
            double denom2 = (knotRange[i + k + 1] - knotRange[i + 1]);
            if (denom1 != 0 && denom2 != 0)
                last_deriv_basis[i] = k * (last_deriv_basis[i] / (knotRange[i + k] - knotRange[i]) - last_deriv_basis[i + 1] / (knotRange[i + k + 1] - knotRange[i + 1]));
            else if (denom1 != 0)
                last_deriv_basis[i] = k * (last_deriv_basis[i] / (knotRange[i + k] - knotRange[i]));
            else if (denom2 != 0)
                last_deriv_basis[i] = -k * (last_deriv_basis[i + 1] / (knotRange[i + k + 1] - knotRange[i + 1]));
            else
                last_deriv_basis[i] = 0;
        }
    }
    free(knotRange);
    return last_deriv_basis;
}

CAGD_POINT evaluateMixedPartialDerivative(Surface* surf, double u, double v) {
    int order_u = surf->u_order;
    int order_v = surf->v_order;
    Kn_List* knots_u = surf->u_kn_list;
    Kn_List* knots_v = surf->v_kn_list;

    int J_u = evaluateJ(knots_u, u);
    int J_v = evaluateJ(knots_v, v);

    double* basis_derivatives_u[2] = {evaluateBSplineBasisDerivatives(knots_u, u, J_u, order_u, 0), evaluateBSplineBasisDerivatives(knots_u, u, J_u, order_u, 1) };
    double* basis_derivatives_v[2] = {evaluateBSplineBasisDerivatives(knots_v, v, J_v, order_v, 0) ,evaluateBSplineBasisDerivatives(knots_v, v, J_v, order_v, 1) };
    double weight_sum = 0;
    CAGD_POINT N = { 0.0, 0.0, 0.0 }, Nu = { 0.0, 0.0, 0.0 }, Nv = { 0.0, 0.0, 0.0 }, Nuv = { 0.0, 0.0, 0.0 };
    double D = 0.0, Du = 0.0, Dv = 0.0, Duv = 0.0;
    CAGD_POINT mixed_derivative = { 0.0, 0.0, 0.0 };
    for (int i = 0; i < order_v; i++) {
        for (int j = 0; j < order_u; j++) {
            Control_Point* cp = get_cp_grid(surf->cp_grid, J_v - (order_v-1) + i, J_u - (order_u - 1) + j);
            double weight = get_weight(cp);
            CAGD_POINT q = get_location(cp);
            double mult = basis_derivatives_u[0][j] * basis_derivatives_v[0][i] * weight;
            N = cagdPointAdd(N, scaler(q, mult));
            D += mult;
            mult = basis_derivatives_u[1][j] * basis_derivatives_v[0][i] * weight;
            Nu = cagdPointAdd(Nu, scaler(q, mult));
            Du += mult;
            mult = basis_derivatives_u[0][j] * basis_derivatives_v[1][i] * weight;
            Nv = cagdPointAdd(Nv, scaler(q, mult));
            Dv += mult;
            mult = basis_derivatives_u[1][j] * basis_derivatives_v[1][i] * weight;
            Nuv = cagdPointAdd(Nuv, scaler(q, mult));
            Duv += mult;
        }
    }
    double D_pow2 = D * D;
    double D_pow4 = D_pow2 * D_pow2;
    //if (fabs(D_pow4) > 0.0001) {
        mixed_derivative = 
            cagdPointSub(
                scaler(
                    cagdPointSub(
                        cagdPointAdd(
                            scaler(Nuv, D), scaler(Nu, Dv)),
                        cagdPointAdd(
                            scaler(Nv, Du), scaler(N, Duv))
                    ),1/D_pow2),
                scaler(
                    cagdPointSub(
                        scaler(Nu, D),
                        scaler(N, Du)
                    ), 2 * Dv * D / D_pow4
               )
        );
    //}

    for (int i = 0; i < 2; i++) {
        free(basis_derivatives_u[i]);
        free(basis_derivatives_v[i]);
    }
    return mixed_derivative;
}

static void evaluateSurfacePartialDerivative(Surface* surf, double u, double v, CAGD_POINT* Su , CAGD_POINT* Sv) {
    char* dir = "u";
    Kn_List* knots = surf->u_kn_list;
    int order = surf->u_order;
    Control_Point** cp_array = evaluateQcpArr(surf, v , dir);
    *Su = evaluatePartialDerivatives(cp_array, knots, order, u, dir, 1);
    deleteQcpArr(cp_array, surf->u_cp_len);
    dir = "v";
    knots = surf->v_kn_list;
    order = surf->v_order;
    cp_array = evaluateQcpArr(surf, u, dir);
    *Sv = evaluatePartialDerivatives(cp_array, knots, order, v, dir, 1);
    deleteQcpArr(cp_array, surf->v_cp_len);
}

void evaluateSurfaceSecondPartialDerivative(Surface* surf, double u, double v, CAGD_POINT* Suu, CAGD_POINT* Suv, CAGD_POINT* Svv) {
    // Calculate Suu
    char* dir = "u";
    Kn_List* knots = surf->u_kn_list;
    int order = surf->u_order;
    Control_Point** cp_array = evaluateQcpArr(surf, v, dir);
    *Suu = evaluatePartialDerivatives(cp_array, knots, order, u, dir, 2);
    deleteQcpArr(cp_array, surf->u_cp_len);


    // Calculate Svv
    dir = "v";
    knots = surf->v_kn_list;
    order = surf->v_order;
    cp_array = evaluateQcpArr(surf, u, dir);
    *Svv = evaluatePartialDerivatives(cp_array, knots, order, v, dir, 2);
    deleteQcpArr(cp_array, surf->v_cp_len);

    *Suv = evaluateMixedPartialDerivative(surf, u, v);
}


// Function to calculate the first fundamental form
void calculateFirstFundamentalForm(CAGD_POINT Su, CAGD_POINT Sv, double* E, double* F, double* G) {
    *E = dot_prod(Su, Su);
    *F = dot_prod(Su, Sv);
    *G = dot_prod(Sv, Sv);
}


// Function to calculate the second fundamental form
void calculateSecondFundamentalForm(Surface* surf, double u, double v, CAGD_POINT normal, double* L, double* M, double* N) {
    CAGD_POINT Suu,Suv,Svv;
    evaluateSurfaceSecondPartialDerivative(surf, u, v, &Suu, &Suv,&Svv);
    *L = dot_prod(Suu, normal);
    *M = dot_prod(Suv, normal);
    *N = dot_prod(Svv, normal);
}



void drawSurfaceShapes(Surface* surf, double u, double v, BOOL for_anime) {
    SurfaceManager* manager = getSurfaceManager();
    double k1, k2;
    CAGD_POINT Su, Sv, N, normalizedSu, normalizedSv, normalizedN;
    evaluateSurfacePartialDerivative(surf, u, v, &Su, &Sv);
    N = calculateNormal(Su, Sv);
    CAGD_POINT p = get_point_on_surface(surf, u, v);
    normalizedSu = normalize(Su);
    normalizedSv = normalize(Sv);
    normalizedN = normalize(N);

    if (manager->draw_geo[B_TANGENTS]) {
        CAGD_POINT tangU[2] = { p, cagdPointAdd(p,normalizedSu) };
        CAGD_POINT tangV[2] = { p, cagdPointAdd(p,normalizedSv) };
        int tangent_u = for_anime ? D_ANI_TANGENTU : D_CLI_TANGENTU;
        int tangent_v = for_anime ? D_ANI_TANGENTV : D_CLI_TANGENTV;
        cagdSetSegmentColor(surf->drawings[tangent_u], 0, 255, 0);
        cagdReusePolyline(surf->drawings[tangent_u], tangU, 2);
        cagdSetSegmentColor(surf->drawings[tangent_v], 0, 0, 255);
        cagdReusePolyline(surf->drawings[tangent_v], tangV, 2);
    }
    if (manager->draw_geo[B_TANGENTP]) {
        CAGD_POINT half_lengths[2] = { scaler(normalizedSu,0.5) , scaler(normalizedSv,0.5) };
        CAGD_POINT plane_corners[4] = { cagdPointAdd(p,half_lengths[0]),cagdPointSub(p,half_lengths[1]), cagdPointSub(p,half_lengths[0]), cagdPointAdd(p,half_lengths[1]) };
        int tangent_plane = for_anime ? D_ANI_TANGENTP : D_CLI_TANGENTP;
        reuse_plane(surf->drawings[tangent_plane], plane_corners);
        cagdSetSegmentColor(surf->drawings[tangent_plane], 255, 0, 0);
    }    
    if (manager->draw_geo[B_NORMAL]) {
        CAGD_POINT normal[2] = { p, cagdPointAdd(p,normalizedN) };
        int noraml_vec = for_anime ? D_ANI_NORMAL : D_CLI_NORMAL;
        cagdSetSegmentColor(surf->drawings[noraml_vec], 255, 0 , 0);
        cagdReusePolyline(surf->drawings[noraml_vec], normal, 2);
    }
    if (manager->draw_geo[B_CURVATURE]) {
        CAGD_POINT pdir[2];
        calculatePrincipalCurvaturesAndDirections(surf, u, v, Su, Sv, normalizedN, &k1, &k2, &pdir[0], &pdir[1]);
        CAGD_POINT circle_center;
        CAGD_POINT osculating_plane[2] = { normalizedN, pdir[1]};
        int ocs_circ1 = for_anime ? D_ANI_OCSCIRC1 : D_CLI_OCSCIRC1;
        int ocs_circ2 = for_anime ? D_ANI_OCSCIRC2 : D_CLI_OCSCIRC2;

        if(k1 != INFINITE && k1 != 0){
            circle_center = cagdPointAdd(p, scaler(normalizedN , 1 / k1));
            drawCircle(circle_center, 1 / k1, osculating_plane, &surf->drawings[ocs_circ1]);
            cagdSetSegmentColor(surf->drawings[ocs_circ1], 255, 255, 0);
        }
        if (k2 != INFINITE && k2 != 0) {
            osculating_plane[1] = pdir[0];
            circle_center = cagdPointAdd(p, scaler(normalizedN, 1 / k2));
            drawCircle(circle_center, 1 / k2, osculating_plane, &surf->drawings[ocs_circ2]);
            cagdSetSegmentColor(surf->drawings[ocs_circ2], 255, 255, 255);
        }
    }
    cagdRedraw();
}

static void hideAllSegments(Surface* surf) {
    for (int i = 0; i < D_LAST; i++) {
        cagdHideSegment(surf->drawings[i]);
    }
}
static void showSegments(Surface* surf) {
    SurfaceManager* manager = getSurfaceManager();
    if(manager->popUpBitMap & (1 << FLAG_CLICK)){
        if (manager->draw_geo[B_TANGENTS]) {
            cagdShowSegment(surf->drawings[D_CLI_TANGENTU]);
            cagdShowSegment(surf->drawings[D_CLI_TANGENTV]);
        }
        if (manager->draw_geo[B_TANGENTP]) {
            cagdShowSegment(surf->drawings[D_CLI_TANGENTP]);
        }
        if (manager->draw_geo[B_NORMAL])
            cagdShowSegment(surf->drawings[D_CLI_NORMAL]);
        if (manager->draw_geo[B_CURVATURE]) {
            cagdShowSegment(surf->drawings[D_CLI_OCSCIRC1]);
            cagdShowSegment(surf->drawings[D_CLI_OCSCIRC2]);
        }
    }
    if (manager->popUpBitMap & (1 << FLAG_ANIME)) {
        if (manager->draw_geo[B_TANGENTS]) {
            cagdShowSegment(surf->drawings[D_ANI_TANGENTU]);
            cagdShowSegment(surf->drawings[D_ANI_TANGENTV]);
        }
        if (manager->draw_geo[B_TANGENTP]) {
            cagdShowSegment(surf->drawings[D_ANI_TANGENTP]);
        }
        if (manager->draw_geo[B_NORMAL])
            cagdShowSegment(surf->drawings[D_ANI_NORMAL]);
        if (manager->draw_geo[B_CURVATURE]) {
            cagdShowSegment(surf->drawings[D_ANI_OCSCIRC1]);
            cagdShowSegment(surf->drawings[D_ANI_OCSCIRC2]);
        }
    }
}
void clickDraw(Surface* surf, int x, int y) {
    double u, v;
    if (getSurfaceUVFromClick(surf, x, y, &u, &v)) {
        drawSurfaceShapes(surf, u, v, FALSE);
    }
}
void clickSurfaceShapesLeftUp(int x, int y, PVOID userData)
{
    Surface* surf = (Surface*)userData;
    cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
    cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
    hideAllSegments(surf);
    showSegments(surf);

}
void clickSurfaceShapesLeftMove(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    clickDraw(surf, x, y);
    hideAllSegments(surf);
    showSegments(surf);
}
static void drawShapesClickLeftDown(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    cagdRegisterCallback(CAGD_LBUTTONUP, clickSurfaceShapesLeftUp, userData);
    cagdRegisterCallback(CAGD_MOUSEMOVE, clickSurfaceShapesLeftMove, userData);
    clickDraw(surf, x, y);
    hideAllSegments(surf);
    showSegments(surf);
}

void drawShapesClickHandler(Surface* surf, BOOL flag) {
    if (flag) {
        cagdRegisterCallback(CAGD_LBUTTONDOWN, drawShapesClickLeftDown, (PVOID)surf);
    }
    else {
        cagdRegisterCallback(CAGD_MOUSEMOVE, NULL, NULL);
        cagdRegisterCallback(CAGD_LBUTTONDOWN, NULL, NULL);
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
        hideAllSegments(surf);
        showSegments(surf);
    }
}
void surfaceShapesAnimation(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    hideAllSegments(surf);
    surf->curr_anime += getSurfaceManager()->anime_speed;
    double t = surf->curr_anime = surf->curr_anime > 1 ? 0 : surf->curr_anime;
    drawSurfaceShapes(surf, surf->to_anime[0] * t + (1-t) * surf->from_anime[0], surf->to_anime[1] * t + (1 - t) * surf->from_anime[1], TRUE);
    showSegments(surf);
    cagdRedraw();
}
void set_animation(Surface* surf) {
    cagdRegisterCallback(CAGD_TIMER, surfaceShapesAnimation, (PVOID)surf);
}
static void drawShapesAnimeLeftUp(int x, int y, PVOID userData) {
    Surface* surf = (Surface*)userData;
    double u, v; 
    if (getSurfaceUVFromClick(surf, x, y, &u, &v)) {
        if (surf->anime_points_selected == 1) {
            surf->to_anime[0] = u;
            surf->to_anime[1] = v;
            set_animation(surf);
            cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
        }
        else {
            surf->from_anime[0] = u;
            surf->from_anime[1] = v;
        }
        surf->anime_points_selected++;
    }
}
void stop_animation(Surface* surf) {
    surf->curr_anime = 0;
    hideAllSegments(surf);
    showSegments(surf);
    cagdRegisterCallback(CAGD_TIMER, NULL, NULL);

}

void drawShapesAnimeHandler(Surface* surf, BOOL flag) {
    if (flag) {
        cagdRegisterCallback(CAGD_LBUTTONUP, drawShapesAnimeLeftUp, (PVOID)surf);
    }
    else {
        stop_animation(surf);
        surf->anime_points_selected = 0;
        cagdRegisterCallback(CAGD_LBUTTONUP, defaultSurfaceLeftClick, (PVOID)NULL);
    }
}

double* get_greville_arr(Kn_List* knots, int K) {
    int k = K - 1;
    int cp_len = kn_list_len(knots) - K;
    double* knots_arr = get_knot_array(knots);
    double* greville_arr = SAFE_MALLOC(double, cp_len);
    double res = 0;
    for (int i = 0; i < k; i++) {
        res += knots_arr[i];
    }
    for (int i = 0; i < cp_len; i++) {
        res += knots_arr[k + i] - knots_arr[i];
        greville_arr[i] =  bound_to_domain(knots,K,res/k);
    }
    free(knots_arr);
    return  greville_arr;
}

void offset_surface(Surface* surf, enum Surface_Control flag) {
    double u_domain[2], v_domain[2];
    get_domain(surf->u_kn_list, surf->u_order, u_domain);
    get_domain(surf->v_kn_list, surf->v_order, v_domain);
    CAGD_POINT** offsets = SAFE_MALLOC(CAGD_POINT*, surf->u_cp_len);
    double* u_grevilles = get_greville_arr(surf->u_kn_list, surf->u_order);
    double* v_grevilles = get_greville_arr(surf->v_kn_list, surf->v_order);
    for (int i = 0; i < surf->u_cp_len; i++) {
        double u = u_grevilles[i];
        offsets[i] = SAFE_MALLOC(CAGD_POINT, surf->v_cp_len);
        for (int j = 0; j < surf->v_cp_len; j++) {
            double v, k1, k2;
            v = v_grevilles[j];
            CAGD_POINT Su, Sv, N, normalizedSu, normalizedSv, normalizedN, pdir[2];
            evaluateSurfacePartialDerivative(surf, u, v, &Su, &Sv);
            N = calculateNormal(Su, Sv);
            normalizedSu = normalize(Su);
            normalizedSv = normalize(Sv);
            normalizedN = normalize(N);
            calculatePrincipalCurvaturesAndDirections(surf, u, v, Su, Sv, N, &k1, &k2, &pdir[0], &pdir[1]);
            switch (flag) {
            case OFSE:
                //offsets[i][j] = scaler(normalizedN , 1/(surf->offset));
                offsets[i][j] = scaler(normalizedN, 1);

                break;
            case K1OF:
                if (fabs(k1) < 0.1)
                    offsets[i][j] = (CAGD_POINT){ 0.0, 0.0, 0.0 };
                else
                    offsets[i][j] = scaler(normalizedN, 1/k1);
                break;
            case K2OF:
                if (fabs(k2) < 0.1)
                    offsets[i][j] = (CAGD_POINT){ 0.0, 0.0, 0.0 };
                else
                    offsets[i][j] = scaler(normalizedN, 1/k2);
                break;
            case HOFF:
                if (fabs(k1 + k2) < 0.2)
                    offsets[i][j] = (CAGD_POINT){ 0.0, 0.0, 0.0 };
                else
                    offsets[i][j] = scaler(normalizedN, 2/(k1+k2));
                break;
            case KOFF:
                if (fabs(k1 * k2) < 1)
                    offsets[i][j] = (CAGD_POINT){ 0.0, 0.0, 0.0 };
                else
                    offsets[i][j] = scaler(normalizedN, 1/(k1 * k2));
                break;
            default:
                offsets[i][j] = (CAGD_POINT){ 0.0, 0.0, 0.0 };
                break;
            }
        }
    }
    for (int i = 0; i < surf->v_cp_len; i++) {
        for (int j =0 ; j < surf->u_cp_len; j++) {
            transform_grid_cp_at(surf->cp_grid, offsets[i][j], i, j);
        }
        free(offsets[i]);
    }
    free(offsets);
    free(u_grevilles);
    free(v_grevilles);
    drawSurface(surf);
}

typedef struct {
    HWND hUDegreeEdit;
    HWND hVDegreeEdit;
    HWND hUCpLengthEdit;
    HWND hVCpLengthEdit;
    HWND hOptionalButton;
    HWND hUKnotsEdit;
    HWND hVKnotsEdit;
    HWND hCreateButton;
} AppWidgets;

BOOL createSurfaceFromDialog(AppWidgets* widgets) {
    char line[254];
    int i = 0, u_i = 0, v_i = 0;
    Surface* res = SAFE_MALLOC(Surface, 1);
    initSurface(res);
    GetWindowText(widgets->hUCpLengthEdit, line, sizeof(line));
    res->u_cp_len = atoi(line);
    GetWindowText(widgets->hVCpLengthEdit, line, sizeof(line));
    res->v_cp_len = atoi(line);
    GetWindowText(widgets->hUDegreeEdit, line, sizeof(line));
    res->u_order = atoi(line);
    GetWindowText(widgets->hVDegreeEdit, line, sizeof(line));
    res->v_order = atoi(line);
    kn_list_init(&res->u_kn_list);
    kn_list_init(&res->v_kn_list);
    BOOL checked = IsDlgButtonChecked(widgets->hOptionalButton, IDC_OPTIONAL);
    if(checked){
        int offset = 0, j = 0;
        double knot;
        GetWindowText(widgets->hUKnotsEdit, line, sizeof(line));
        while (j < res->u_cp_len + res->u_order) {
            if (sscanf(line + offset, " %lf%n", &knot, &offset) != 1) {
                myMessage("File incorrect format!", "Unable to scan knots", MB_ICONERROR);
                return FALSE;
            }
            else {
                kn_list_insert(res->u_kn_list, knot, 1);
            //	sprintf(logMsg, "j: %d , knot = %lf \n", j, knot);
            //	LogToOutput(logMsg)
                j++;
             }
        }
        j = 0;
        GetWindowText(widgets->hVKnotsEdit, line, sizeof(line));
        while (j < res->v_cp_len + res->v_order) {
            if (sscanf(line + offset, " %lf%n", &knot, &offset) != 1) {
                myMessage("File incorrect format!", "Unable to scan knots", MB_ICONERROR);
                return FALSE;
            }
            else {
                kn_list_insert(res->v_kn_list, knot, 1);
                //	sprintf(logMsg, "j: %d , knot = %lf \n", j, knot);
                //	LogToOutput(logMsg)
                j++;
            }
        }
        toggle_cp_grid_visiblity(res->cp_grid, FALSE, FALSE, FALSE);
        pushSurface(res);
        drawSurface(res);
    }
    else {
        remake_as_closed_uni(&res->u_kn_list, res->u_cp_len, res->u_order);
        remake_as_closed_uni(&res->v_kn_list, res->v_cp_len, res->v_order);
    }
    init_cp_grid(&res->cp_grid, res->v_cp_len, res->u_cp_len);
    double x = - ((double)res->v_cp_len)/2, y = -((double)res->u_cp_len) / 2;
    for (v_i = 0; v_i < res->v_cp_len; v_i++)
        for (u_i = 0; u_i < res->u_cp_len; u_i++)
            update_grid_cp_at(res->cp_grid, cp_create((CAGD_POINT) { x + v_i, y + u_i, 0 }, 1), v_i, u_i);
    toggle_cp_grid_visiblity(res->cp_grid, FALSE, FALSE, FALSE);
    pushSurface(res);
    drawSurface(res);
    return TRUE;
}

void EnableOptionalSection(AppWidgets* widgets, BOOL enable) {
    EnableWindow(widgets->hUKnotsEdit, enable);
    EnableWindow(widgets->hVKnotsEdit, enable);
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static AppWidgets widgets;

    switch (message) {
    case WM_INITDIALOG:
        widgets.hUDegreeEdit = GetDlgItem(hDlg, IDC_U_DEGREE);
        widgets.hVDegreeEdit = GetDlgItem(hDlg, IDC_V_DEGREE);
        widgets.hUCpLengthEdit = GetDlgItem(hDlg, IDC_U_CP_LENGTH);
        widgets.hVCpLengthEdit = GetDlgItem(hDlg, IDC_V_CP_LENGTH);
        widgets.hOptionalButton = GetDlgItem(hDlg, IDC_OPTIONAL);
        widgets.hUKnotsEdit = GetDlgItem(hDlg, IDC_U_KNOTS);
        widgets.hVKnotsEdit = GetDlgItem(hDlg, IDC_V_KNOTS);
        widgets.hCreateButton = GetDlgItem(hDlg, IDC_CREATE);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CREATE) {
            createSurfaceFromDialog(&widgets) ? EndDialog(hDlg, TRUE) : (void*)0;
        }
        else if (LOWORD(wParam) == IDC_OPTIONAL) {
            BOOL checked = IsDlgButtonChecked(hDlg, IDC_OPTIONAL);
            EnableOptionalSection(&widgets, checked);
        }
        return (INT_PTR)TRUE;

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}
