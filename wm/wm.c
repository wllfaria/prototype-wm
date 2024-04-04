#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>

#include "wm.h"

static bool wm_detected_;

window_manager wm_create() {
  Display* display = XOpenDisplay(NULL);

  if(display == NULL)
    exit(1);

  window_manager wm = {.display = display,
      .root_ = DefaultRootWindow(display),
      .clients_ = calloc(10000, sizeof(Window))};

  return wm;
}

void wm_run(window_manager* wm) {
  wm_detected_ = false;

  XSetErrorHandler(&wm_on_wm_detected);
  XSelectInput(wm->display, wm->root_,
      SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(wm->display, false);

  if(wm_detected_) {
    return;
  }

  XSetErrorHandler(&wm_on_x_error);

  for(;;) {
    XEvent e;
    XNextEvent(wm->display, &e);

    switch(e.type) {
    case CreateNotify:
      wm_on_create_notify(e.xcreatewindow);
      break;
    case DestroyNotify:
      wm_on_destroy_notify(e.xdestroywindow);
    case ReparentNotify:
      wm_on_reparent_notify(e.xreparent);
    case ConfigureRequest:
      wm_on_configure_request(wm, e.xconfigurerequest);
      break;
    case MapRequest:
      wm_on_map_request(wm, e.xmaprequest);
    case MapNotify:
      wm_on_map_notify(e.xmap);
      break;
    default:
      printf("ignored event");
    }
  }

  return;
}

static int wm_on_wm_detected(Display* display, XErrorEvent* e) {
  if(e->error_code == BadAccess)
    wm_detected_ = true;
  return 0;
}

static int wm_on_x_error(Display* display, XErrorEvent* e) {
  printf("%d", e->error_code);
  return 0;
}

void wm_on_create_notify(XCreateWindowEvent e) {
  return;
}

void wm_on_destroy_notify(XDestroyWindowEvent e) {
  return;
}

void wm_on_reparent_notify(XReparentEvent e) {
  return;
}

void wm_on_configure_request(window_manager* wm, XConfigureRequestEvent e) {
  XWindowChanges changes;

  changes.x = e.x;
  changes.y = e.y;
  changes.width = e.width;
  changes.height = e.height;
  changes.border_width = e.border_width;
  changes.sibling = e.above;
  changes.stack_mode = e.detail;

  if(wm->clients_[e.window] != NULL) {
    const Window* frame = wm->clients_[e.window];
    XConfigureWindow(wm->display, *frame, e.value_mask, &changes);
  }

  XConfigureWindow(wm->display, e.window, e.value_mask, &changes);
}

void frame(window_manager* wm, Window w) {
  const unsigned int BORDER_WIDTH = 3;
  const unsigned long BORDER_COLOR = 0xFF0000;
  const unsigned long BG_COLOR = 0x00FF;

  XWindowAttributes window_attrs;
  XGetWindowAttributes(wm->display, w, &window_attrs);

  const Window frame = XCreateSimpleWindow(wm->display, wm->root_,
      window_attrs.x, window_attrs.y, window_attrs.width, window_attrs.height,
      BORDER_WIDTH, BORDER_COLOR, BG_COLOR);

  XSelectInput(
      wm->display, frame, SubstructureRedirectMask | SubstructureNotifyMask);

  XAddToSaveSet(wm->display, w);
  XReparentWindow(wm->display, w, frame, 0, 0);
  XMapWindow(wm->display, frame);
  wm->clients_[w] = (Window*)frame;

  // alt + left
  XGrabButton(wm->display, Button1, Mod1Mask, w, false,
      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
      GrabModeAsync, None, None);

  // alt + right
  XGrabButton(wm->display, Button3, Mod1Mask, w, false,
      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
      GrabModeAsync, None, None);

  // alt + q
  XGrabButton(wm->display, Button3, Mod1Mask, w, false,
      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
      GrabModeAsync, None, None);

  XGrabKey(wm->display, XKeysymToKeycode(wm->display, XK_Q), Mod1Mask, w, false,
      GrabModeAsync, GrabModeAsync);

  XGrabKey(wm->display, XKeysymToKeycode(wm->display, XK_Tab), Mod1Mask, w,
      false, GrabModeAsync, GrabModeAsync);
}

void wm_on_map_request(window_manager* wm, XMapRequestEvent e) {
  frame(wm, e.window);

  return;
}

void wm_on_map_notify(XMapEvent e) {
  return;
}
