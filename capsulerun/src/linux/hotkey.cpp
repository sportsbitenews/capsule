
#include <capsulerun.h>
#include "../shared/MainLoop.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <thread>

using namespace std;

Display *capsule_x11_dpy;
Window capsule_x11_root;

static void capsule_hotkey_poll (MainLoop *ml) {
    XEvent ev;

    while(1) {
        XNextEvent(capsule_x11_dpy, &ev);

        switch (ev.type) {
            case KeyPress:
                ml->capture_flip();
            default:
                break;
        }
    }
}

// int capsule_x11_error_handler (Display *dpy, XErrorEvent *err) {
//     fprintf(stderr, "X11 error type %d\n", err->type);
// }

int capsule_hotkey_init(MainLoop *ml) {
    // XSetErrorHandler(capsule_x11_error_handler);
    capsule_x11_dpy = XOpenDisplay(0);
    capsule_x11_root = DefaultRootWindow(capsule_x11_dpy);

    int num_modifiers = 4;
    unsigned int modifier_list[] = {
        0,
        Mod2Mask,
        Mod3Mask,
        Mod2Mask | Mod3Mask,
    };

    int keycode = XKeysymToKeycode(capsule_x11_dpy, XK_F9);
    Window grab_window = capsule_x11_root;
    Bool owner_events = False;
    int pointer_mode = GrabModeAsync;
    int keyboard_mode = GrabModeAsync;

    for (int i = 0; i < num_modifiers; i++) {
        XGrabKey(
            capsule_x11_dpy,
            keycode,
            modifier_list[i],
            grab_window,
            owner_events,
            pointer_mode,
            keyboard_mode
        );
    }
    XSelectInput(capsule_x11_dpy, capsule_x11_root, KeyPressMask);

    thread poll_thread(capsule_hotkey_poll, ml);
    poll_thread.detach();

    return 0;
}

