#include "window_manager.hpp"
#include <glog/logging.h>
#include <cstring>

using ::std::unique_ptr;
using ::std::string;

bool WindowManager::wm_detected_;

unique_ptr<WindowManager> WindowManager::Create(){
    // 1. Open X display
    Display* display = XOpenDisplay(nullptr);

    // error handling for opening display
    if (display == nullptr){
        LOG(ERROR) << "Failed to open X display" << XDisplayName(nullptr);
        return nullptr;
    }

    // 2. Construct WM instance
    return unique_ptr<WindowManager> (new WindowManager(display));
}

WindowManager::WindowManager(Display* display)
    : display_(CHECK_NOTNULL(display)),
      root_(DefaultRootWindow(display_)){
}

WindowManager::~WindowManager(){
    XCloseDisplay(display_);
}

void WindowManager::Run() { 
    // 1. Init
    //  - select events on root window, quit if another WM present

    wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWMDetected); // defer to OnWMDetected if error encountered

    XSelectInput (
        display_,
        root_,
        SubstructureRedirectMask | SubstructureNotifyMask
    );

    XSync(display_, false);

    if (wm_detected_){
        LOG(ERROR) << "Another window manager is already running" << XDisplayString(display_);
        return;
    }

    //  - set error handler
    XSetErrorHandler(&WindowManager::OnXError);

    // 2. Event Loop
    /* TODO */

    for (;;){
        // Next event
        XEvent e;
        XNextEvent(display_, &e);

        //Handle event depending on type
        switch (e.type){
            case CreateNotify:
                OnCreateNotify(e.xcreatewindow);
                break;
            case ConfigureRequest:
                OnConfigureRequest(e.xconfigurerequest);
                break;
            case MapRequest:
                OnMapRequest(e.xmaprequest);
            default:
                LOG(WARNING) << "Unhandled Event";
        }
    }
}

int WindowManager::OnWMDetected(Display* display, XErrorEvent* e){
    // Check error code - should be BadAccess
    CHECK_EQ(static_cast<int>(e->error_code), BadAccess);
    // Set flag then return
    wm_detected_ = true;
    return 0;
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent& e){}

void WindowManager::OnConfigureRequest(const XConfigureRequestEvent& e){
    XWindowChanges changes;
    // 1. Changes from e -> changes object
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    // 2. Apply changes using XConfigureWindow()
    XConfigureWindow(display_, e.window, e.value_mask, &changes);

    // 3. Log event for debugging
    LOG(INFO) << "Resize " << e.window << " to " << e.width << "x" << e.height;
}

void WindowManager::OnMapRequest(const XMapRequestEvent& e){
    Frame(e.window);
    XMapWindow(display_, e.window);
}

void WindowManager::Frame(Window w){ //Draws window decorations
    /* TODO */
}

int WindowManager::OnXError(Display* display, XErrorEvent* e){
    /*TODO*/
}