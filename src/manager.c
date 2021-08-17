#include "manager.h"
#include "log.h"
#include "mem.h"
#include <X11/Xutil.h>
#include <stdio.h>
#include <unistd.h>

static int wm_detected = 0;

static int onXError(Display *Display, XErrorEvent *e)
{
}

static int onWMDetected(Display *Display, XErrorEvent *e)
{
	if (e->error_code == BadAccess)
	{
		wm_detected = 1;
		return 0;
	}
}

LimeWM *lime_window_manager_create()
{
	LimeWM *wm = lime_mallocz(sizeof(*wm));
	wm->clients = lime_list_create();
	return wm;
}

int lime_window_manager_init(LimeWM *wm)
{
	wm->main_display = XOpenDisplay(NULL);
	if (wm->main_display == NULL)
	{
		lime_error("create display error name: %s", XDisplayName(NULL));
		return -1;
	}

	wm->main_window = DefaultRootWindow(wm->main_display);
	lime_info("create display done name %s", XDisplayString(wm->main_display));

	XSetErrorHandler(onWMDetected);
	XSelectInput(wm->main_display, wm->main_window,
				 SubstructureRedirectMask | SubstructureNotifyMask);
	XSync(wm->main_display, 0);
	if (wm_detected)
	{
		lime_error("detected another window manager on display %s", XDisplayString(wm->main_display));
		return -1;
	}

	XSetErrorHandler(onXError);

	return 0;
}

static const char *const X_EVENT_TYPE_NAMES[] = {
	"",
	"",
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"MappingNotify",
	"GeneralEvent",
};

const char *ToString(const XEvent e)
{
	if (e.type < 2 || e.type >= LASTEvent)
	{
		printf("UNKONW (%d)", e.type);
		return NULL;
	}

	return X_EVENT_TYPE_NAMES[e.type];
}

static LimeClient *get_client(Window w, LimeWM *wm)
{
	LimeClient *c = NULL;
	for (LimeListEntry *entry = wm->clients->root; entry != NULL; entry = entry->next)
	{
		c = entry->data;
		if (c->window == w)
		{
			break;
		}
		c = NULL;
	}
	return c;
}

static void on_create_notify(XCreateWindowEvent e, LimeWM *wm)
{
	XTextProperty p = {};
	XGetWMName(wm->main_display, e.window, &p);
	char **data = NULL;
	int count = 0;
	XTextPropertyToStringList(&p, &data, &count);
	for(int i=0; i<count; i++){
		printf("%s\n",data[i]);
	}

}

static void on_destroy_notify(XDestroyWindowEvent e)
{
}

static void on_reparent_notify(XReparentEvent e)
{
}

static void on_configure_request(XConfigureRequestEvent e, LimeWM *wm)
{
	XWindowChanges changes;
	changes.x = e.x;
	changes.y = e.y;
	changes.width = e.width;
	changes.height = e.height;
	changes.border_width = e.border_width;
	changes.sibling = e.above;
	changes.stack_mode = e.detail;
	XConfigureWindow(wm->main_display, e.window, e.value_mask, &changes);
	lime_info("resize window:%d to %dx%d", e.window, e.width, e.height);
}

static void frame(Window w, LimeWM *wm, int created_before)
{
	const uint32_t BORDER_WIDTH = 3;
	const uint32_t BORDER_COLOR = 0x118888;
	const uint32_t BG_COLOR = 0xeeeeee;

	XWindowAttributes x_window_attrs;
	XGetWindowAttributes(wm->main_display, w, &x_window_attrs);

	if (created_before)
	{
		if (x_window_attrs.override_redirect ||
			x_window_attrs.map_state != IsViewable)
		{
			return;
		}
	}

	const Window frame = XCreateSimpleWindow(wm->main_display, wm->main_window,
											 x_window_attrs.x,
											 x_window_attrs.y,
											 x_window_attrs.width,
											 x_window_attrs.height,
											 BORDER_WIDTH,
											 BORDER_COLOR,
											 BG_COLOR);
	XSelectInput(wm->main_display, frame,
				 SubstructureNotifyMask | SubstructureRedirectMask);
	XAddToSaveSet(wm->main_display, w);
	XReparentWindow(wm->main_display, w,
					frame, 0, 0);
	XMapWindow(wm->main_display, frame);
	LimeClient *c = lime_mallocz(sizeof(*c));
	c->frame = frame;
	c->window = w;
	lime_list_add(wm->clients, c);
	XGrabButton(
		wm->main_display,
		Button1,
		Mod1Mask,
		w,
		0,
		ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
		GrabModeAsync,
		GrabModeAsync,
		None,
		None);

	XGrabButton(
		wm->main_display,
		Button3,
		Mod1Mask,
		w,
		0,
		ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
		GrabModeAsync,
		GrabModeAsync,
		None,
		None);

//	XGrabButton(
//		wm->main_display,
//		Button1,
//		0,
//		frame,
//		0,
//		ButtonPressMask | ButtonReleaseMask ,
//		GrabModeAsync,
//		GrabModeAsync,
//		None,
//		None
//	);

	XGrabKey(
		wm->main_display,
		XKeysymToKeycode(wm->main_display, XK_F4),
		Mod1Mask,
		w,
		0,
		GrabModeAsync,
		GrabModeAsync);

	XGrabKey(
		wm->main_display,
		XKeysymToKeycode(wm->main_display, XK_Tab),
		Mod1Mask,
		w,
		0,
		GrabModeAsync,
		GrabModeAsync);

	lime_info("framed widnow %d [%d]", w, frame);
}

static void on_map_request(XMapRequestEvent e, LimeWM *wm)
{
	frame(e.window, wm, 0);
	XMapWindow(wm->main_display, e.window);
}

static void unfame(Window w, LimeWM *wm, LimeClient *c)
{
	Window frame = c->frame;
	XUnmapWindow(wm->main_display, frame);
	XReparentWindow(
		wm->main_display,
		w,
		wm->main_window,
		0,
		0);

	XRemoveFromSaveSet(wm->main_display, w);
	XDestroyWindow(wm->main_display, frame);
	lime_list_del(wm->clients, c);
	lime_free(c);
	lime_info("unframed window %d [%d]", w, frame);
}

static void on_unmap_notify(XUnmapEvent e, LimeWM *wm)
{

	if (e.event == wm->main_window)
	{
		//lime_info("ignore unmap notify for reparented pre-existing window %d", e.window);
		return;
	}

	LimeClient *c = get_client(e.window, wm);

	if (c == NULL)
	{
		lime_info("unmap notify can not find window %d", e.window);
		return;
	}
	unfame(e.window, wm, c);
}

static void on_button_press(XButtonEvent e, LimeWM *wm)
{
	LimeClient *c = get_client(e.window, wm);
	if (c == NULL)
	{
		lime_info("can not find window %d", e.window);
		return;
	}

	wm->sdragx = e.x_root;
	wm->sdragy = e.y_root;
	int x_from, y_from;
	Window return_root;
	uint32_t width, height, border_width, depth;

	XGetGeometry(wm->main_display, c->frame,
				 &return_root,
				 &x_from, &y_from,
				 &width, &height,
				 &border_width,
				 &depth);
	wm->framex = x_from;
	wm->framey = y_from;
	wm->framew = width;
	wm->frameh = height;

	printf("move frame post %d,%d | drag pos %d,%d\n", wm->framex, wm->framey,
		   wm->sdragx, wm->sdragy);

	XRaiseWindow(wm->main_display, c->frame);
	XSetInputFocus(wm->main_display, c->window,
	 RevertToPointerRoot, CurrentTime);
}

void on_motion_notify(XMotionEvent e, LimeWM *wm)
{
	LimeClient *c = get_client(e.window, wm);
	if (c == NULL)
	{
		lime_error("motion can not find window %d", e.window);
		return;
	}

	//printf("%d,%d %d,%d\n",e.x_root, e.y_root, wm->sdragx, wm->sdragy);
	int deltax = e.x_root - wm->sdragx;
	int deltay = e.y_root - wm->sdragy;
	if (e.state & Button1Mask)
	{
		int dstx = wm->framex + deltax;
		int dsty = wm->framey + deltay;
		XMoveWindow(wm->main_display,
					c->frame,
					dstx,
					dsty);
	}
	else if (e.state & Button3Mask)
	{
		deltax = deltax > -wm->framew ? deltax : wm->framew;
		deltay = deltay > -wm->frameh ? deltay : wm->frameh;
		int dstw = wm->framew + deltax;
		int dsth = wm->frameh + deltay;

		XResizeWindow(
			wm->main_display,
			c->frame,
			dstw,
			dsth);

		XResizeWindow(
			wm->main_display,
			c->window,
			dstw,
			dsth);
	}
}

void on_key_press(XKeyEvent e, LimeWM *wm)
{
	if ((e.state & Mod1Mask) &&
		(e.keycode == XKeysymToKeycode(wm->main_display, XK_F4)))
	{
		Atom *supported_protocols;
		int num_supported_protocols;
		if (XGetWMProtocols(wm->main_display,
							e.window,
							&supported_protocols,
							&num_supported_protocols))
		{
			for (size_t i = 0; i < num_supported_protocols; i++)
			{
				if (supported_protocols[i])
				{
					XEvent msg;
					memset(&msg, 0, sizeof(msg));
					msg.xclient.type = ClientMessage;
					Atom WM_PROTOCOLS = XInternAtom(wm->main_display, "WM_PROTOCOLS", 0);
					msg.xclient.message_type = WM_PROTOCOLS;
					msg.xclient.window = e.window;
					;
					msg.xclient.format = 32;
					Atom WM_DELETE_WINDOW = XInternAtom(wm->main_display, "WM_DELETE_WINDOW", 0);
					msg.xclient.data.l[0] = WM_DELETE_WINDOW;
					XSendEvent(wm->main_display, e.window, 0, 0, &msg);
				}
			}
		}
		else
		{
			XKillClient(wm->main_display, e.window);
		}
	}
	else if ((e.state & Mod1Mask) &&
			 (e.keycode == XKeysymToKeycode(wm->main_display, XK_Tab)))
	{
		LimeListEntry *next;
		LimeClient *c = get_client(e.window, wm);
		for (LimeListEntry *cur = wm->clients->root; cur != NULL; cur = cur->next)
		{
			if (cur->data == c)
			{
				next = cur->next;
				break;
			}
		}

		if (next == NULL)
		{
			next = wm->clients->root;
		}

		LimeClient *nc = next->data;

		XRaiseWindow(wm->main_display, nc->frame);
		XSetInputFocus(wm->main_display, nc->window, RevertToPointerRoot, CurrentTime);
	}else if((e.state & Mod1Mask && e.state & Mod2Mask)&&
	(e.keycode == XKeysymToKeycode(wm->main_display, XK_t))){
		printf("IM HERE\n");
		execl("xterm",NULL);
	}
}


void on_map_notify(XMapEvent e, LimeWM *wm)
{
}

void lime_window_manager_run(LimeWM *wm)
{

	XGrabKey(
		wm->main_display,
		XKeysymToKeycode(wm->main_display,XK_t),
		Mod1Mask | Mod2Mask,
		wm->main_window,
		0,
		GrabModeAsync,
		GrabModeAsync
	);

	XGrabServer(wm->main_display);

	Window root, parent;
	Window *topwindows;
	uint32_t nums;
	XQueryTree(
		wm->main_display,
		wm->main_window,
		&root,
		&parent,
		&topwindows,
		&nums);
	for (size_t i = 0; i < nums; i++)
	{
		frame(topwindows[i], wm, 1);
	}

	XFree(topwindows);
	XUngrabServer(wm->main_display);

	while (!wm->exit)
	{
		XEvent e;
		XNextEvent(wm->main_display, &e);
		lime_info("event: %s", ToString(e));

		switch (e.type)
		{
		case CreateNotify:
			on_create_notify(e.xcreatewindow, wm);
			break;

		case ConfigureRequest:
			on_configure_request(e.xconfigurerequest, wm);
			break;

		case ConfigureNotify:
			printf("configure notify\n");
			break;

		case MapNotify:
			printf("Map notify\n");
			on_map_notify(e.xmap, wm);
			break;

		case DestroyNotify:
			on_destroy_notify(e.xdestroywindow);
			break;

		case ReparentNotify:
			on_reparent_notify(e.xreparent);
			break;

		case MapRequest:
			on_map_request(e.xmaprequest, wm);
			break;

		case UnmapNotify:
			on_unmap_notify(e.xunmap, wm);
			break;

		case ButtonPress:
			on_button_press(e.xbutton, wm);
			break;
		case MotionNotify:
			while (XCheckTypedWindowEvent(
				wm->main_display,
				e.xmotion.window,
				MotionNotify, &e))
			{
			}
			on_motion_notify(e.xmotion, wm);

		case KeyPress:
			on_key_press(e.xkey, wm);
			break;

		default:
			lime_info("ignored event: %s", ToString(e));
			//lime_info("ignored event", NULL);
			break;
		}
	}
}

void lime_window_manager_exit(LimeWM *wm)
{
	wm->exit = 1;
}

void lime_window_manager_destroy(LimeWM *wm)
{
}
