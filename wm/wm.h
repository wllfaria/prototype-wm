#include <X11/Xlib.h>
#include <stdbool.h>

typedef struct {
  Display* display;
  const Window root_;
  Window** clients_;
} window_manager;

window_manager wm_create();
void wm_run(window_manager* wm);
static int wm_on_x_error(Display* display, XErrorEvent* e);
static int wm_on_wm_detected(Display* display, XErrorEvent* e);
void wm_on_create_notify(XCreateWindowEvent e);
void wm_on_destroy_notify(XDestroyWindowEvent e);
void wm_on_reparent_notify(XReparentEvent e);
void wm_on_configure_request(window_manager* wm, XConfigureRequestEvent e);
void wm_on_map_request(window_manager* wm, XMapRequestEvent e);
void wm_on_map_notify(XMapEvent e);
void wm_on_unmap_notify(window_manager* wm, XUnmapEvent e);
void wm_on_button_press(window_manager* wm, XButtonPressedEvent e);
void wm_on_button_release(window_manager* wm, XButtonReleasedEvent e);
void wm_on_key_press(window_manager* wm, XKeyPressedEvent e);
void wm_on_key_release(window_manager* wm, XKeyReleasedEvent e);
