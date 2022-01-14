#include "manager.h"
#include "log.h"
#include "mem.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static int wm_detected = 0;

static int onXError(Display *Display, XErrorEvent *e) { return -1; }

static int onWMDetected(Display *Display, XErrorEvent *e) {
  if (e->error_code == BadAccess) {
    wm_detected = 1;
    return 0;
  }
  return -1;
}

LimeWM *lime_window_manager_create() {
  LimeWM *wm = lime_mallocz(sizeof(*wm));
  wm->clients = lime_list_create();
  return wm;
}

int lime_window_manager_init(LimeWM *wm) {
  wm->main_display = XOpenDisplay(NULL);
  if (wm->main_display == NULL) {
    lime_error("create display error name: %s", XDisplayName(NULL));
    return -1;
  }

  wm->main_window = DefaultRootWindow(wm->main_display);
  lime_info("create display done name %s", XDisplayString(wm->main_display));

  XSetErrorHandler(onWMDetected);
  XSelectInput(wm->main_display, wm->main_window,
               SubstructureRedirectMask | SubstructureNotifyMask);
  XGrabKey(wm->main_display, XKeysymToKeycode(wm->main_display, XK_T),
           Mod1Mask | ControlMask, wm->main_window, false, GrabModeAsync,
           GrabModeAsync);
  XSync(wm->main_display, 0);
  if (wm_detected) {
    lime_error("detected another window manager on display %s",
               XDisplayString(wm->main_display));
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

const char *ToString(const XEvent e) {
  if (e.type < 2 || e.type >= LASTEvent) {
    printf("UNKONW (%d)", e.type);
    return NULL;
  }

  return X_EVENT_TYPE_NAMES[e.type];
}

static LimeClient *get_client(Window w, LimeWM *wm) {
  LimeClient *c = NULL;
  for (LimeListEntry *entry = wm->clients->root; entry != NULL;
       entry = entry->next) {
    c = entry->data;
    if (c->window == w) {
      c->event_src = LIME_WINDOW;
      break;
    }
    if (c->frame == w) {
      c->event_src = LIME_FRAME;
      break;
    }
    if (c->title == w) {
      c->event_src = LIME_TITLE_BAR;
      break;
    }
    if (c->leftSide == w) {
      c->event_src = LIME_LSIDE;
      break;
    }
    if (c->rightSide == w) {
      c->event_src = LIME_RSIDE;
      break;
    }
    if (c->downSide == w) {
      c->event_src = LIME_BSIDE;
      break;
    }
    if (c->downLeftCorner == w) {
      c->event_src = LIME_LCORNER;
      break;
    }
    if (c->downRightCorner == w) {
      c->event_src = LIME_RCORNER;
      break;
    }
    c = NULL;
  }
  return c;
}

static LimeClient *get_client_use_frame(Window frame, LimeWM *wm) {

  LimeClient *c = NULL;
  for (LimeListEntry *entry = wm->clients->root; entry != NULL;
       entry = entry->next) {
    c = entry->data;
    if (c->frame == frame) {
      c->event_src = LIME_FRAME;
      break;
    }
    c = NULL;
  }
  return c;
}

static void on_create_notify(XCreateWindowEvent e, LimeWM *wm) {
  XTextProperty p = {};
  XGetWMName(wm->main_display, e.window, &p);
  char **data = NULL;
  int count = 0;
  XTextPropertyToStringList(&p, &data, &count);
  for (int i = 0; i < count; i++) {
    printf("%s\n", data[i]);
  }
}

static void on_destroy_notify(XDestroyWindowEvent e) {}

static void on_reparent_notify(XReparentEvent e) {}

static void on_configure_request(XConfigureRequestEvent e, LimeWM *wm) {
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

static Window createTitlebar(LimeWM *wm, Window frame, int width) {

  const uint32_t BG_COLOR = 0xf22222;
  Window title = XCreateSimpleWindow(wm->main_display, frame, 0, 0, width, 10,
                                     0, 0, BG_COLOR);
  // XAddToSaveSet(wm->main_display, title);
  XReparentWindow(wm->main_display, frame, title, 0, 0);
  XMapWindow(wm->main_display, title);
  XSelectInput(wm->main_display, title, EnterWindowMask | LeaveWindowMask);
  XGrabButton(wm->main_display, Button1, 0, title, 0,
              ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
              title, None);

  return title;
}

#define CORNER_WIDTH 10

static void grabButton1(LimeWM *wm, Window w) {

  XGrabButton(wm->main_display, Button1, 0, w, 0,
              ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
              None, None);
}

static Window createDownSide(LimeWM *wm, Window frame, int pwidth,
                             int pheight) {
  const uint32_t BG_COLOR = 0x725222;
  Window side =
      XCreateSimpleWindow(wm->main_display, frame, CORNER_WIDTH, pheight - 2,
                          pwidth - CORNER_WIDTH * 2, 2, 0, 0, BG_COLOR);
  XReparentWindow(wm->main_display, frame, side, 0, 0);
  XMapWindow(wm->main_display, side);

  XSelectInput(wm->main_display, side, EnterWindowMask | LeaveWindowMask);
  grabButton1(wm, side);
  return side;
}

static Window createLRSide(LimeWM *wm, Window frame, int left, int pwidth,
                           int pheight) {
  const uint32_t BG_COLOR = 0x725222;

  int x = 0;
  if (!left) {
    x = pwidth - 2;
  }

  Window side =
      XCreateSimpleWindow(wm->main_display, frame, x, 10, // window pos
                          2, pheight - 2,                 // window size
                          0, 0, BG_COLOR);

  XReparentWindow(wm->main_display, frame, side, 0, 0);
  XSelectInput(wm->main_display, side, EnterWindowMask | LeaveWindowMask);
  XMapWindow(wm->main_display, side);
  grabButton1(wm, side);
  return side;
}

static Window createCorner(LimeWM *wm, Window frame, int left, int pwidth,
                           int pheight) {
  const uint32_t BG_COLOR = 0x225222;
  int x = 0;
  if (!left) {
    x = pwidth - CORNER_WIDTH;
  }

  Window side = XCreateSimpleWindow(wm->main_display, frame, x, pheight - 2,
                                    CORNER_WIDTH, 2, 0, 0, BG_COLOR);
  XReparentWindow(wm->main_display, frame, side, 0, 0);
  XMapWindow(wm->main_display, side);
  grabButton1(wm, side);
  return side;
}

static void frame(Window w, LimeWM *wm, int created_before) {
  const uint32_t BORDER_WIDTH = 0;
  const uint32_t BORDER_COLOR = 0x118888;
  const uint32_t BG_COLOR = 0x222222;

  XWindowAttributes x_window_attrs;
  XGetWindowAttributes(wm->main_display, w, &x_window_attrs);
  Atom *protocols;
  Atom *ap;
  int n, i;
  int status = XGetWMProtocols(wm->main_display, w, &protocols, &n);

  if (status && (protocols != NULL)) {
    for (i = 0, ap = protocols; i < n; i++, ap++) {
      printf("ap: %d\n", *ap);
    }
  }

  if (created_before) {
    if (x_window_attrs.override_redirect ||
        x_window_attrs.map_state != IsViewable) {
      return;
    }
  }

  const Window frame = XCreateSimpleWindow(
      wm->main_display, wm->main_window, x_window_attrs.x, x_window_attrs.y,
      x_window_attrs.width, x_window_attrs.height, BORDER_WIDTH, BORDER_COLOR,
      BG_COLOR);
  XSelectInput(wm->main_display, frame,
               SubstructureNotifyMask | SubstructureRedirectMask);
  XAddToSaveSet(wm->main_display, w);
  XResizeWindow(wm->main_display, w, x_window_attrs.width,
                x_window_attrs.height - 10);
  XReparentWindow(wm->main_display, w, frame, 0, 10);
  XMapWindow(wm->main_display, frame);

  Window title = createTitlebar(wm, frame, x_window_attrs.width);

  LimeClient *c = lime_mallocz(sizeof(*c));

  c->downSide =
      createDownSide(wm, frame, x_window_attrs.width, x_window_attrs.height);

  c->leftSide =
      createLRSide(wm, frame, 1, x_window_attrs.width, x_window_attrs.height);

  c->rightSide =
      createLRSide(wm, frame, 0, x_window_attrs.width, x_window_attrs.height);

  //c->downLeftCorner =
  //    createCorner(wm, frame, 1, x_window_attrs.width, x_window_attrs.height);

  //c->downRightCorner =
  //    createCorner(wm, frame, 0, x_window_attrs.width, x_window_attrs.height);

  c->frame = frame;
  c->window = w;
  c->title = title;
  lime_list_add(wm->clients, c);

  // XGrabPointer(
  //	wm->main_display,
  //	frame,
  //	0,
  //         EnterWindowMask|LeaveWindowMask,
  //	GrabModeAsync,
  //	GrabModeAsync,
  //	None,
  //	None,
  //	CurrentTime
  //);

  XGrabKey(wm->main_display, XKeysymToKeycode(wm->main_display, XK_F4),
           Mod1Mask, w, 0, GrabModeAsync, GrabModeAsync);

  XGrabKey(wm->main_display, XKeysymToKeycode(wm->main_display, XK_Tab),
           Mod1Mask, w, 0, GrabModeAsync, GrabModeAsync);

  lime_info("framed widnow %d [%d]", w, frame);
}

static void on_map_request(XMapRequestEvent e, LimeWM *wm) {
  frame(e.window, wm, 0);
  XMapWindow(wm->main_display, e.window);
}

static void unfame(Window w, LimeWM *wm, LimeClient *c) {
  Window frame = c->frame;
  XUnmapWindow(wm->main_display, frame);
  XReparentWindow(wm->main_display, w, wm->main_window, 0, 0);

  XRemoveFromSaveSet(wm->main_display, w);
  XDestroyWindow(wm->main_display, frame);
  lime_list_del(wm->clients, c);
  lime_free(c);
  lime_info("unframed window %d [%d]", w, frame);
}

static void on_unmap_notify(XUnmapEvent e, LimeWM *wm) {

  if (e.event == wm->main_window) {
    // lime_info("ignore unmap notify for reparented pre-existing window %d",
    // e.window);
    return;
  }

  LimeClient *c = get_client(e.window, wm);

  if (c == NULL) {
    lime_info("unmap notify can not find window %d", e.window);
    return;
  }
  unfame(e.window, wm, c);
}

static void on_button_release(XButtonEvent e, LimeWM *wm, XEvent *xe) {
  LimeClient *c = NULL;
  c = get_client(e.window, wm);
  if (c == NULL) {
    lime_error("can not find window %d", e.window);
    printf("can not find window\n");
    return;
  }

  if (c->event_src == LIME_TITLE_BAR) {
    c->on_drag = 0;
    XUngrabPointer(wm->main_display, 0);
  } else if (c->event_src == LIME_LSIDE) {
    c->on_left_resize = 0;
    XUngrabPointer(wm->main_display, 0);
  } else if (c->event_src == LIME_RSIDE) {
    c->on_right_resize = 0;
    XUngrabPointer(wm->main_display, 0);
    printf("ButtonRelease c->on_right_resize = 0\n");
  } else if (c->event_src == LIME_BSIDE) {
    c->on_bottom_resize = 0;
    XUngrabPointer(wm->main_display, 0);
  }
  if (c->on_bottom_resize == 1 || c->on_left_resize == 1 ||
      c->on_right_resize == 1 || c->on_drag == 1) {
    c->on_bottom_resize = c->on_left_resize = c->on_right_resize = c->on_drag =
        0;
  }
}

static void process_button_press(LimeWM *wm, LimeClient *c, XButtonEvent e) {
  c->drag_src_posx = e.x_root;
  c->drag_src_posy = e.y_root;

  int x_from, y_from;
  Window return_root;
  uint32_t width, height, border_width, depth;

  XGetGeometry(wm->main_display, c->frame, &return_root, &x_from, &y_from,
               &width, &height, &border_width, &depth);
  c->frame_posx = x_from;
  c->frame_posy = y_from;
  c->frame_width = width;
  c->frame_height = height;

  XGetGeometry(wm->main_display, c->rightSide, &return_root, &x_from, &y_from,
               &width, &height, &border_width, &depth);
  c->rside_posx = x_from;
  c->rside_posy = y_from;
  c->rside_width = width;
  c->rside_height = height;

  XGetGeometry(wm->main_display, c->downSide, &return_root, &x_from, &y_from,
               &width, &height, &border_width, &depth);
  c->bside_posx = x_from;
  c->bside_posy = y_from;
  c->bside_width = width;
  c->bside_height = height;

  XGrabPointer(wm->main_display, c->title, 0,
               PointerMotionMask | ButtonReleaseMask | ButtonPressMask,
               GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
}

static void on_button_press(XButtonEvent e, LimeWM *wm, XEvent *xe) {
  LimeClient *c = NULL;
  if (!(e.state & Mod1Mask)) {
    c = get_client(e.window, wm);
    // c = get_client_use_frame(e.window, wm);
  } else {
    c = get_client(e.window, wm);
  }
  if (c == NULL) {
    lime_info("can not find window %d", e.window);
    return;
  }
  if (c->event_src == LIME_TITLE_BAR) {
    c->on_drag = 1;
    process_button_press(wm, c, e);
  } else if (c->event_src == LIME_LSIDE) {
    c->on_left_resize = 1;
    process_button_press(wm, c, e);
  } else if (c->event_src == LIME_RSIDE) {
    c->on_right_resize = 1;
    process_button_press(wm, c, e);
    printf("c->on_right_resize = 1\n");
  } else if (c->event_src == LIME_BSIDE) {
    c->on_bottom_resize = 1;
    process_button_press(wm, c, e);
  }

  if (c->on_drag == 1) {
    printf("move frame post %d,%d | drag pos %d,%d\n", c->frame_posx,
           c->frame_posy, c->drag_src_posx, c->drag_src_posy);
  }

  XRaiseWindow(wm->main_display, c->frame);
  XSetInputFocus(wm->main_display, c->window, RevertToPointerRoot, CurrentTime);
  return;
  if (!(e.state & Mod1Mask)) {
    XUngrabButton(wm->main_display, Button1, AnyModifier, c->frame);
  }
}

void on_motion_notify(XMotionEvent e, LimeWM *wm) {
  LimeClient *c = get_client(e.window, wm);
  if (c == NULL) {
    lime_error("motion can not find window %d", e.window);
    printf("can not find window");
    return;
  }

  int ep = XPending(wm->main_display);

  if (c->on_drag == 1) {
    int deltax = e.x_root - c->drag_src_posx;
    int deltay = e.y_root - c->drag_src_posy;
    int dstx = c->frame_posx + deltax;
    int dsty = c->frame_posy + deltay;
    // limit
    if (dsty < 10) {
      dsty = 10;
    }
    XMoveWindow(wm->main_display, c->frame, dstx, dsty);
  } else if (c->on_right_resize == 1) {
    int deltax = e.x_root - c->drag_src_posx;
    int deltay = e.y_root - c->drag_src_posy;

    deltax = deltax > -c->frame_width ? deltax : c->frame_width;
    deltay = deltay > -c->frame_height ? deltay : c->frame_height;

    int dstw = c->frame_width + deltax;
    int dsth = c->frame_height;

    int dstx = c->rside_posx + deltax;
    int dsty = c->rside_posy;

    XMoveWindow(wm->main_display, c->rightSide, dstx, dsty);

    XResizeWindow(wm->main_display, c->frame, dstw, dsth);
    XResizeWindow(wm->main_display, c->window, dstw, dsth - 10);
    XResizeWindow(wm->main_display, c->title, dstw, 10);
    XResizeWindow(wm->main_display, c->downSide, dstw , dsth);
  } else if (c->on_bottom_resize == 1) {
    int deltax = e.x_root - c->drag_src_posx;
    int deltay = e.y_root - c->drag_src_posy;
    deltax = deltax > -c->frame_width ? deltax : c->frame_width;
    deltay = deltay > -c->frame_height ? deltay : c->frame_height;
    int dstw = c->frame_width;
    int dsth = c->frame_height + deltay;

    int dstx = c->bside_posx;
    int dsty = c->bside_posy + deltay;

    XMoveWindow(wm->main_display, c->downSide, dstx, dsty);
    XResizeWindow(wm->main_display, c->frame, dstw, dsth);
    XResizeWindow(wm->main_display, c->window, dstw, dsth - 10);
    XResizeWindow(wm->main_display, c->title, dstw, 10);
  }

  return;
}

void on_key_press(XKeyEvent e, LimeWM *wm) {
  if ((e.state & Mod1Mask && e.state & ControlMask &&
       e.keycode == XKeysymToKeycode(wm->main_display, XK_T))) {
    printf("exec xterm\n");

    pid_t pid = fork();
    switch (pid) {
    case -1:
      break;
    case 0: {
      int ret = execl("/usr/bin/xterm", "");
      if (ret != 0) {
        printf("exec error:%s\n", strerror(errno));
      }
    } break;
    }
  }
  if ((e.state & Mod1Mask) &&
      (e.keycode == XKeysymToKeycode(wm->main_display, XK_F4))) {
    Atom *supported_protocols;
    int num_supported_protocols;
    if (XGetWMProtocols(wm->main_display, e.window, &supported_protocols,
                        &num_supported_protocols)) {
      for (size_t i = 0; i < num_supported_protocols; i++) {
        if (supported_protocols[i]) {
          XEvent msg;
          memset(&msg, 0, sizeof(msg));
          msg.xclient.type = ClientMessage;
          Atom WM_PROTOCOLS = XInternAtom(wm->main_display, "WM_PROTOCOLS", 0);
          msg.xclient.message_type = WM_PROTOCOLS;
          msg.xclient.window = e.window;
          ;
          msg.xclient.format = 32;
          Atom WM_DELETE_WINDOW =
              XInternAtom(wm->main_display, "WM_DELETE_WINDOW", 0);
          msg.xclient.data.l[0] = WM_DELETE_WINDOW;
          XSendEvent(wm->main_display, e.window, 0, 0, &msg);
        }
      }
    } else {
      XKillClient(wm->main_display, e.window);
    }
  } else if ((e.state & Mod1Mask) &&
             (e.keycode == XKeysymToKeycode(wm->main_display, XK_Tab))) {
    LimeListEntry *next;
    LimeClient *c = get_client(e.window, wm);
    for (LimeListEntry *cur = wm->clients->root; cur != NULL; cur = cur->next) {
      if (cur->data == c) {
        next = cur->next;
        break;
      }
    }

    if (next == NULL) {
      next = wm->clients->root;
    }

    LimeClient *nc = next->data;

    XRaiseWindow(wm->main_display, nc->frame);
    XSetInputFocus(wm->main_display, nc->window, RevertToPointerRoot,
                   CurrentTime);
  } else if ((e.state & Mod1Mask && e.state & Mod2Mask) &&
             (e.keycode == XKeysymToKeycode(wm->main_display, XK_t))) {
    printf("IM HERE\n");
    execl("xterm", NULL);
  }
}

void on_map_notify(XMapEvent e, LimeWM *wm) {}

void on_focus_in(XFocusInEvent e, LimeWM *wm) {
  //	XUngrabButton(
  //		wm->main_display,
  //		Button1,
  //		ButtonPressMask ,
  //		e.window
  //	);
}
void on_focus_out(XFocusOutEvent e, LimeWM *wm) {
  //	XGrabButton(
  //		wm->main_display,
  //		Button1,
  //		0,
  //		e.window,
  //		0,
  //		ButtonPressMask,
  //		GrabModeAsync,
  //		GrabModeAsync,
  //		None,
  //		None
  //	);
}
void on_pointer_enter(XCrossingEvent e, LimeWM *wm) {
  LimeClient *c = get_client(e.window, wm);
  if (c->event_src == LIME_TITLE_BAR) {
    Cursor cr = XCreateFontCursor(wm->main_display, XC_left_ptr);
    XDefineCursor(wm->main_display, c->title, cr);
  } else if (c->event_src == LIME_LSIDE) {
    Cursor cr = XCreateFontCursor(wm->main_display, XC_sb_h_double_arrow);
    XDefineCursor(wm->main_display, c->leftSide, cr);
  } else if (c->event_src == LIME_RSIDE) {
    Cursor cr = XCreateFontCursor(wm->main_display, XC_sb_h_double_arrow);
    XDefineCursor(wm->main_display, c->rightSide, cr);
  } else if (c->event_src == LIME_BSIDE) {
    Cursor cr = XCreateFontCursor(wm->main_display, XC_sb_v_double_arrow);
    XDefineCursor(wm->main_display, c->downSide, cr);
  } else if (c->event_src == LIME_FRAME) {
  } else if (c->event_src == LIME_WINDOW) {
  }
}

void on_pointer_leave(XCrossingEvent e, LimeWM *wm) {
  LimeClient *c = get_client(e.window, wm);
  if (c->event_src == LIME_TITLE_BAR) {
    printf("@@@@@@@@@@@@@@@@@@@@@\n");
  } else {
    printf("**********************\n");
  }
}

void lime_window_manager_run(LimeWM *wm) {

  XGrabKey(wm->main_display, XKeysymToKeycode(wm->main_display, XK_t),
           Mod1Mask | Mod2Mask, wm->main_window, 0, GrabModeAsync,
           GrabModeAsync);

  XGrabServer(wm->main_display);

  Window root, parent;
  Window *topwindows;
  uint32_t nums;
  XQueryTree(wm->main_display, wm->main_window, &root, &parent, &topwindows,
             &nums);
  for (size_t i = 0; i < nums; i++) {
    frame(topwindows[i], wm, 1);
  }

  XFree(topwindows);
  XUngrabServer(wm->main_display);

  while (!wm->exit) {
    XEvent e;
    XNextEvent(wm->main_display, &e);
    lime_info("event: %s", ToString(e));

    switch (e.type) {
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
      on_button_press(e.xbutton, wm, &e);
      break;

    case ButtonRelease:
      printf("Button release\n");
      on_button_release(e.xbutton, wm, &e);
      break;

    case MotionNotify:
      // while (XCheckTypedWindowEvent(
      //  wm->main_display,
      //  e.xmotion.window,
      //  MotionNotify, &e))
      //{
      // }
      on_motion_notify(e.xmotion, wm);

    case KeyPress:
      printf("key press\n");
      on_key_press(e.xkey, wm);
      break;

    case FocusIn:
      on_focus_in(e.xfocus, wm);
      break;

    case FocusOut:
      on_focus_out(e.xfocus, wm);
      break;

    case EnterNotify:
      on_pointer_enter(e.xcrossing, wm);
      break;

    case LeaveNotify:
      on_pointer_leave(e.xcrossing, wm);
      break;

    default:
      lime_info("ignored event: %s", ToString(e));
      // lime_info("ignored event", NULL);
      break;
    }
  }
}

void lime_window_manager_exit(LimeWM *wm) { wm->exit = 1; }

void lime_window_manager_destroy(LimeWM *wm) {}
