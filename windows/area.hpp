#ifndef __LIBUI_AREA_HPP__
#define __LIBUI_AREA_HPP__

// TODOs
// - things look very wrong on initial draw
// - initial scrolling is not set properly
// - should background be inherited from parent control?

struct uiArea {
	uiWindowsControl c;
	HWND hwnd;
	uiAreaHandler *ah;

	BOOL scrolling;
	int scrollWidth;
	int scrollHeight;
	int hscrollpos;
	int vscrollpos;
	int hwheelCarry;
	int vwheelCarry;

	uiprivClickCounter cc;
	BOOL capturing;

	BOOL inside;
	BOOL tracking;

	ID2D1HwndRenderTarget *rt;
};

typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);

struct uiOpenGLArea {
	uiWindowsControl c;
	HWND hwnd;
	uiOpenGLAreaHandler *ah;

	BOOL scrolling;
	int scrollWidth;
	int scrollHeight;
	int hscrollpos;
	int vscrollpos;
	int hwheelCarry;
	int vwheelCarry;

	uiprivClickCounter cc;
	BOOL capturing;

	BOOL inside;
	BOOL tracking;

	ID2D1HwndRenderTarget *rt;

	// ABOVE IS EQUIVALENT TO uiArea

	uiOpenGLAttributes *attribs;
	HDC hDC;
	HGLRC hglrc;
	// Not guarenteed to work across multiple contexts, therfore area specific.
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
};

// areadraw.cpp
extern BOOL areaDoDraw(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult, BOOL isOpenGLArea);
extern void areaDrawOnResize(uiArea *, RECT *);

// areascroll.cpp
extern BOOL areaDoScroll(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);
extern void areaScrollOnResize(uiArea *, RECT *);
extern void areaUpdateScroll(uiArea *a);

// areaevents.cpp
extern BOOL areaDoEvents(uiArea *a, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult);

// areautil.cpp
extern void loadAreaSize(uiArea *a, ID2D1RenderTarget *rt, double *width, double *height);
extern void pixelsToDIP(uiArea *a, double *x, double *y);
extern void dipToPixels(uiArea *a, double *x, double *y);

#endif

