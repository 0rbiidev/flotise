#include "window_manager.hpp"

extern "C"{
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
}

#include <glog/logging.h>
#include <cstring>
#include <algorithm>

using ::std::unique_ptr;
using ::std::string;
using ::std::max;

static const char* const X_EVENT_TYPE_NAMES[] = {
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
      root_(DefaultRootWindow(display_)),
      WM_DELETE_WINDOW(XInternAtom(display_, "WM_DELETE_WINDOW", false)),
      WM_PROTOCOLS(XInternAtom(display_, "WM_PROTOCOLS", false))
{}

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

    //  - prevent changes to existing windows during framing
    XGrabServer(display_);

    //  - frame existing windows
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;

    CHECK(XQueryTree(
        display_,
        root_,
        &returned_root,
        &returned_parent,
        &top_level_windows,
        &num_top_level_windows
    ));
    CHECK_EQ(returned_root, root_);

    for (unsigned int i = 0; i < num_top_level_windows; i++){
        Frame(top_level_windows[i], true /*was created before flotise*/);
    }

    //  - allow changes again
    XFree(top_level_windows);
    XUngrabServer(display_);

    // 2. Event Loop
    for (;;){
        // Next event
        XEvent e;
        XNextEvent(display_, &e);
        LOG(INFO) << "Received event: " << X_EVENT_TYPE_NAMES[e.type];

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
                break;
            case ConfigureNotify:
                OnConfigureNotify(e.xconfigure);
                break;
            case UnmapNotify:
                OnUnmapNotify(e.xunmap);
                break;
            case DestroyNotify:
                OnDestroyNotify(e.xdestroywindow);
                break;
            case ButtonPress:
                OnButtonPress(e.xbutton);
                break;
            case KeyPress:
                OnKeyPress(e.xkey);
                break;
            case ButtonRelease:
                OnButtonRelease(e.xbutton);
                break; 
            case KeyRelease:
                OnKeyRelease(e.xkey);
                break;
            case MotionNotify:
                OnMotionNotify(e.xmotion);
                break;
            default:
                LOG(WARNING) << "Unhandled Event" << X_EVENT_TYPE_NAMES[e.type];
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

void WindowManager::OnConfigureNotify(const XConfigureEvent& e){}

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

    if (clients_.count(e.window)){ //apply changes to window frame...
        const Window frame = clients_[e.window];
        XConfigureWindow(display_, frame, e.value_mask, &changes);
        LOG(INFO) << "Resize [" << frame << "] to " << e.width << "x" << e.height;
    }

    XConfigureWindow(display_, e.window, e.value_mask, &changes); //...then to window

    // 3. Log event for debugging
    LOG(INFO) << "Resize " << e.window << " to " << e.width << "x" << e.height;
}

void WindowManager::OnMapRequest(const XMapRequestEvent& e){
    Frame(e.window, false);
    XMapWindow(display_, e.window);
}

void WindowManager::OnUnmapNotify(const XUnmapEvent& e){
    // Ignore request to unmap non-client window
    // eg. user-destroyed frame

    if(!clients_.count(e.window)){
        LOG(INFO) << "Ignore UnmapNotify for non-client window " << e.window;
        return;
    }

    Unframe(e.window);
}

void WindowManager::Frame(Window w, bool created_before_wm){ //Draws window decorations
    const unsigned int BORDER_WIDTH = 3;
    const unsigned long BORDER_COLOUR = 0x9c353e;
    const unsigned long BG_COLOUR = 0x594646;

    CHECK(!clients_.count(w));

    // Get attrs on window to frame (+ error checking)
    XWindowAttributes attributes;
    CHECK(XGetWindowAttributes(display_, w, &attributes));

    // if framing pre-existing window during init
    // have to check that it is visible and doesnt set override_redirect
    if (created_before_wm){
        if(attributes.override_redirect /*flag set means window should not be managed by a wm*/ ||
           attributes.map_state != IsViewable){
            return;
        }
    }

    // Create frame
    const Window frame = XCreateSimpleWindow (
        display_,
        root_,
        attributes.x,
        attributes.y,
        attributes.width,
        attributes.height,
        BORDER_WIDTH,
        BORDER_COLOUR,
        BG_COLOUR
    );

    // Request that X report events associated with the frame
    XSelectInput(
        display_,
        frame,
        SubstructureRedirectMask | SubstructureNotifyMask
    );

    // Restore client if crash
    XAddToSaveSet(display_, w);

    XReparentWindow(
        display_,
        w,
        frame,
        0, 0
    );
    /* XSetWindowBorder( //research into pixmaps for window border
        display_,
        frame,

    ); */

    // map frame to display
    XMapWindow(display_, frame);

    

    // Save handle
    clients_[w] = frame;

    XGrabButton( // Move window (alt + lclick & drag)
        display_,
        Button1,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None
    );

    XGrabButton( // Resize window (alt + rclick & drag)
        display_,
        Button3,
        Mod1Mask,
        w,
        false,
        ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None
    );

    XGrabKey( // Kill window (alt+f4)
        display_,
        XKeysymToKeycode(display_, XK_F4),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync
    );

    XGrabKey( // Switch window (alt+tab)
        display_,
        XKeysymToKeycode(display_, XK_Tab),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync
    );

    LOG(INFO) << "Framed window " << w << " [" << frame << "]";
}

void WindowManager::Unframe(Window w){
    CHECK(clients_.count(w));

    // Get frame
    const Window frame = clients_[w];
    // Unmap frame
    XUnmapWindow(display_, frame);
    // Reparent frameless client to root window
    XReparentWindow(
        display_,
        w,
        root_,
        0, 0
    );
    // Remove client from save set
    XRemoveFromSaveSet(display_, w);
    // Destroy frame
    XDestroyWindow(display_, frame);
    clients_.erase(w);

    LOG(INFO) << "Unframed Window " << w << " [" << frame << "]";
}

void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e){}

void WindowManager::OnKeyPress(const XKeyEvent& e){
    //alt+f4 - close window
    if ((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(display_, XK_F4))){
        Atom* supported_protocols;
        int num_supported_protocols;
        // Try the WM_DELETE_WINDOW protocol (preferred)
        if (XGetWMProtocols
            (
                display_,
                e.window,
                &supported_protocols,
                &num_supported_protocols
            ) &&supported_protocols + 
            (
                ::std::find(supported_protocols,
                            supported_protocols + num_supported_protocols,
                            WM_DELETE_WINDOW
                            ) !=
                    supported_protocols + num_supported_protocols
            )
        ){
            LOG(INFO) << "Gracefully deleting window " << e.window;

            //create message
            XEvent msg;
            memset(&msg, 0, sizeof(msg));
            msg.xclient.type = ClientMessage;
            msg.xclient.message_type = WM_PROTOCOLS;
            msg.xclient.window = e.window;
            msg.xclient.format = 32;
            msg.xclient.data.l[0] = WM_DELETE_WINDOW;

            // send message
            CHECK(XSendEvent(display_, e.window, false, 0, (XEvent *)&msg));
        } else { // if protocol unsupported, kill window
            LOG(INFO) << "Killing Window " << e.window;
            XKillClient(display_, e.window);
        }
    } 
    // alt+tab - cycle windows
    else if ((e.state & Mod1Mask) && e.keycode == XKeysymToKeycode(display_, XK_Tab))
    {
        //Get next window
        auto i = clients_.find(e.window);
        CHECK(i != clients_.end());
        ++i;
        if (i == clients_.end())
        {
            i = clients_.begin();
        }

        //Raise window
        XRaiseWindow(display_, i->second);
        XSetInputFocus(display_, i->first, RevertToPointerRoot, CurrentTime);
    }
}

void WindowManager::OnKeyRelease(const XKeyEvent& e){}

void WindowManager::OnButtonPress(const XButtonEvent& e){
    CHECK(clients_.count(e.window));
    const Window frame = clients_[e.window];

    dragStartX_ = e.x_root; //
    dragStartY_ = e.y_root; // save cursor's starting position

    Window returnedroot;
    int x,y;
    unsigned width, height, borderwidth, depth;

    CHECK(XGetGeometry(
        display_,
        frame,
        &returnedroot,
        &x, &y,
        &width, &height,
        &borderwidth,
        &depth
    ));

    dragStartFrameX_ = x;
    dragStartFrameY_ = y;

    dragStartFrameWidth_ = width;
    dragStartFrameHeight_ = height;

    XRaiseWindow(display_, frame);
}

void WindowManager::OnButtonRelease(const XButtonEvent& e){}

void WindowManager::OnMotionNotify(const XMotionEvent& e){
    CHECK(clients_.count(e.window));
    const Window frame = clients_[e.window];

    const int dragX = e.x_root;
    const int dragY = e.y_root;

    const int deltaX = dragX - dragStartX_;
    const int deltaY = dragY - dragStartY_;

    if (e.state & Button1Mask)
    // alt+lmouse = move window
    {
        int destFrameX = dragStartFrameX_ + deltaX;
        int destFrameY = dragStartFrameY_ + deltaY;

        XMoveWindow(
            display_,
            frame,
            destFrameX, destFrameY
        );
    }

    else if (e.state & Button3Mask)
    // alt+rmouse = resize window
    {
        int deltaWidth = max(deltaX, -dragStartFrameWidth_);
        int deltaHeight = max(deltaY, -dragStartFrameY_);

        int destFrameWidth = dragStartFrameWidth_ + deltaWidth;
        int destFrameHeight = dragStartFrameHeight_ + deltaHeight;

        XResizeWindow(
            display_,
            frame,
            destFrameWidth, destFrameHeight
        );

        XResizeWindow(
            display_,
            e.window,
            destFrameWidth, destFrameHeight
        );
    }
}

int WindowManager::OnXError(Display* display, XErrorEvent* e){
    const int MAX_ERROR_TEXT_LENGTH = 1024;
    char error_text[MAX_ERROR_TEXT_LENGTH];
    XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
    LOG(ERROR) << "Received X error:\n"
             << "    Request: " << int(e->request_code)
             //<< " - " << XRequestCodeToString(e->request_code) << "\n"
             << "    Error code: " << int(e->error_code)
             << " - " << error_text << "\n"
             << "    Resource ID: " << e->resourceid;
  // The return value is ignored.
  return 0;
}    /*TODO*/

int 
