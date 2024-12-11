#pragma once
#include <cagd.h>
#include "resource.h"

#define CMD_BUFFER_SIZE 256
#define FERNET_CMD_START CAGD_USER
#define FERNET_CMD_END (FERNET_CMD_START+CMD_BUFFER_SIZE)
#define BSPLINE_CMD_START FERNET_CMD_END
#define BSPLINE_CMD_END (BSPLINE_CMD_START+CMD_BUFFER_SIZE)
#define SURFACES_CMD_START BSPLINE_CMD_END
#define SURFACES_CMD_END (SURFACES_CMD_START+CMD_BUFFER_SIZE)

#define BUFSIZ  512

#define NA -1

#if defined(_WIN32)
	#if _MSC_VER >= 1900
		#pragma comment(lib, "legacy_stdio_definitions.lib")
	#endif
#endif

typedef enum {
	TYPE_BSPLINE,
	TYPE_PARAMETRIC,
	TYPE_GRID,
} DataType;

typedef struct {
	void* data;
	DataType type;
} TypedData;

static void* safe_malloc(size_t size) {
	void* ptr = malloc(size);
	if (ptr == NULL) {
		exit(EXIT_FAILURE);
	}
	return ptr;
}

#define SAFE_MALLOC(type, count) ((type*)safe_malloc(sizeof(type) * (count)))

LRESULT CALLBACK myDialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK myDialogProc2(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam);

CAGD_POINT* createCurvePointCollection(double domain[2], e2t_expr_node* func[3], BOOL transform, e2t_expr_node* trans_func, int fin, long long* size);

char myBuffer[BUFSIZ];

char logMsg[2000];

void LogToOutput(const char* message);

int finesse;

void myMessage(PSTR title, PSTR message, UINT type);

static char fileName[0xffff];

static OPENFILENAME openFileName = {
  sizeof(OPENFILENAME),
  NULL,
  NULL,
  "All files (*.*)\0*.*\0\0",
  NULL,
  0,
  0,
  fileName,
  sizeof(fileName),
  NULL,
  0,
  NULL,
  NULL,
  OFN_HIDEREADONLY,
  0,
  0,
  NULL,
  0,
  NULL
};

double norm(CAGD_POINT p);
double dot_prod(CAGD_POINT p1, CAGD_POINT p2);
CAGD_POINT cross_prod(CAGD_POINT p1, CAGD_POINT p2);
CAGD_POINT cagdPointAdd(CAGD_POINT p, CAGD_POINT q);
CAGD_POINT cagdPointSub(CAGD_POINT p, CAGD_POINT q);
CAGD_POINT normalize(CAGD_POINT p);
CAGD_POINT scaler(CAGD_POINT p, double scalar);