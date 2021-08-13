#ifndef __LIME_MANAGER_H__
#define __LIME_MANAGER_H__

#include "list.h"
#include <X11/Xlib.h>

typedef struct lime_client
{
	Window window;
	Window frame;
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
