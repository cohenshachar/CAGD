#include "EnumHell.h"

LRESULT CALLBACK myDialogProc(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_COMMAND)
		return FALSE;
	switch (LOWORD(wParam)) {
	case IDOK:
		GetDlgItemText(hDialog, IDC_EDIT, myBuffer, sizeof(myBuffer));
		EndDialog(hDialog, TRUE);
		return TRUE;
	case IDCANCEL:
		EndDialog(hDialog, FALSE);
		return TRUE;
	default:
		return FALSE;
	}
}

LRESULT CALLBACK myDialogProc2(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_COMMAND)
		return FALSE;
	switch (LOWORD(wParam)) {
	case IDOK:
		GetDlgItemText(hDialog, IDC_EDIT, myBuffer, sizeof(myBuffer));
		int first_s_len = strlen(myBuffer);
		myBuffer[first_s_len] = ' ';
		GetDlgItemText(hDialog, IDC_EDIT2, myBuffer + first_s_len + 1, sizeof(myBuffer) - first_s_len - 1);
		EndDialog(hDialog, TRUE);
		return TRUE;
	case IDCANCEL:
		EndDialog(hDialog, FALSE);
		return TRUE;
	default:
		return FALSE;
	}
}

char logMsg[2000];

void LogToOutput(const char* message) {
	OutputDebugStringA(message); // A for ANSI, W for wide characters
}

int finesse = 100;

CAGD_POINT* createCurvePointCollection(double domain[2], e2t_expr_node* func[3], BOOL transform, e2t_expr_node* trans_func, int fin, long long* size) {
	int i = 0;
	if (domain[1] - domain[0] <= 0)
		return NULL;
	CAGD_POINT* p = (CAGD_POINT*)malloc(((domain[1] - domain[0]) + 1) * fin * sizeof(CAGD_POINT));
	for (double j = domain[0]; j <= domain[1]; j += (1 / ((double)fin)), i++) {
		if (transform) {
			e2t_setparamvalue(j, E2T_PARAM_R);
			e2t_setparamvalue((GLdouble)e2t_evaltree(trans_func), E2T_PARAM_T);
		}
		else
			e2t_setparamvalue(j, E2T_PARAM_T);
		p[i].x = (GLdouble)e2t_evaltree(func[0]);
		p[i].y = (GLdouble)e2t_evaltree(func[1]);
		p[i].z = (GLdouble)e2t_evaltree(func[2]);
	}
	*size = i;
	return p;
}

void myMessage(PSTR title, PSTR message, UINT type)
{
	MessageBox(cagdGetWindow(), message, title, MB_OK | MB_APPLMODAL | type);
}

CAGD_POINT cagdPointAdd(CAGD_POINT p, CAGD_POINT q) {
	CAGD_POINT res;
	res.x = p.x + q.x;
	res.y = p.y + q.y;
	res.z = p.z + q.z;
	return res;
}

CAGD_POINT cagdPointSub(CAGD_POINT p, CAGD_POINT q) {
	CAGD_POINT res;
	res.x = p.x - q.x;
	res.y = p.y - q.y;
	res.z = p.z - q.z;
	return res;
}


double dot_prod(CAGD_POINT p1, CAGD_POINT p2) {
	return (p1.x * p2.x + p1.y * p2.y + p1.z * p2.z);
}

CAGD_POINT cross_prod(CAGD_POINT p1, CAGD_POINT p2) {
	return (CAGD_POINT) {
		(p1.y * p2.z - p1.z * p2.y),
			(p1.z * p2.x - p1.x * p2.z),
			(p1.x * p2.y - p1.y * p2.x)
	};
}

double norm(CAGD_POINT p) {
	return sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
}

CAGD_POINT normalize(CAGD_POINT p) {
	return scaler(p, 1 / norm(p));
}

CAGD_POINT scaler(CAGD_POINT p, double scalar) {
	return (CAGD_POINT) { (p.x * scalar), (p.y * scalar), (p.z * scalar) };
}