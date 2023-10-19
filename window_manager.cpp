#include "window_manager.hpp"
#include <glog/logging.h>

using ::std::unique_ptr;

unique_ptr<WindowManager> WindowManager::Create(){
    // 1. Open X display
    Display* display = XOpenDisplay(nullptr);

    // error handling for 
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

void WindowManager::Run() { /* TODO */}