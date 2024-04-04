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

void frame(window_manager* wm, Window w, bool created_before_wm) {
  const unsigned int BORDER_WIDTH = 3;
  const unsigned long BORDER_COLOR = 0xFF0000;
  const unsigned long BG_COLOR = 0x00FF;

  XWindowAttributes window_attrs;
  XGetWindowAttributes(wm->display, w, &window_attrs);

  if(created_before_wm) {
    if(window_attrs.override_redirect || window_attrs.map_state != IsViewable) {
      return;
    }
  }

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

  XGrabServer(wm->display);
  Window returned_root, returned_parent;
  Window* top_level_windows;
  unsigned int num_top_level_windows;
  XQueryTree(wm->display, wm->root_, &returned_root, &returned_parent,
      &top_level_windows, &num_top_level_windows);
  if(returned_root != wm->root_)
    return;

  for(unsigned int i = 0; i < num_top_level_windows; ++i) {
    frame(wm, top_level_windows[i], true);
  }

  XFree(top_level_windows);
  XUngrabServer(wm->display);

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
    case UnmapNotify:
      wm_on_unmap_notify(wm, e.xunmap);
      break;
    case ButtonPress:
      wm_on_button_press(wm, e.xbutton);
      break;
    case ButtonRelease:
      wm_on_button_release(wm, e.xbutton);
      break;
    case KeyPress:
      wm_on_key_press(wm, e.xkey);
      break;
    case KeyRelease:
      wm_on_key_release(wm, e.xkey);
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

void wm_on_map_request(window_manager* wm, XMapRequestEvent e) {
  frame(wm, e.window, false);

  return;
}

void wm_on_map_notify(XMapEvent e) {
  return;
}

void unframe(window_manager* wm, Window w) {
  const Window* frame = wm->clients_[w];
  XUnmapWindow(wm->display, *frame);
  XReparentWindow(wm->display, w, wm->root_, 0, 0);
  XRemoveFromSaveSet(wm->display, *frame);
  wm->clients_[w] = NULL;

  return;
}

void wm_on_unmap_notify(window_manager* wm, XUnmapEvent e) {
  if(wm->clients_[e.window] == NULL)
    return;

  if(e.event == wm->root_) {
    return;
  }

  unframe(wm, e.window);

  return;
}

void wm_on_button_press(window_manager* wm, XButtonPressedEvent e) {
}
void wm_on_button_release(window_manager* wm, XButtonReleasedEvent e) {
  return;
}
void wm_on_key_press(window_manager* wm, XKeyPressedEvent e) {
  printf("pressed alt + Q\n");
  if((e.state & Mod1Mask) &&
      (e.keycode == XKeysymToKeycode(wm->display, XK_Q))) {
    printf("pressed alt + Q\n");
  }
}
void wm_on_key_release(window_manager* wm, XKeyReleasedEvent e) {
  return;
}
