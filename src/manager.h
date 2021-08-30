#ifndef __LIME_MANAGER_H__
#define __LIME_MANAGER_H__

#include "list.h"
#include <X11/Xlib.h>

typedef enum lime_event_src{
	LIME_TITLE_BAR,
	LIME_LSIDE,
	LIME_RSIDE,
	LIME_BSIDE,
	LIME_LCORNER,
	LIME_RCORNER,
	LIME_FRAME,
	LIME_WINDOW,
}LimeEventSrc;

typedef struct lime_client
{
	Window window;
	Window frame;
	Window title;
	Window downSide;
	Window leftSide;
	Window rightSide;
	Window downLeftCorner;
	Window downRightCorner;
	LimeEventSrc event_src;
	int frame_posx;
	int frame_posy;
	int frame_width;
	int frame_height;
} LimeClient;

typedef struct lime_window_manager
{
	Window main_window;
	Display *main_display;
	LimeList *clients;
	int sdragx;
	int sdragy;
	int framex;
	int framey;
	int framew;
	int frameh;
	int exit;
} LimeWM;

LimeWM *lime_window_manager_create();

int lime_window_manager_init(LimeWM *wm);

void lime_window_manager_run(LimeWM *wm);

void lime_window_manager_exit(LimeWM *wm);

void lime_window_manager_destroy(LimeWM *wm);

#endif
