extern "C"{
#include <X11/Xlib.h>
}

#include <memory>

class WindowManager{
    private:
      WindowManager(Display* display);
      Display* display_;
      const Window root_;
      static int OnXError(Display* display, XErrorEvent* e); // error handler, passes address to Xlib
      static int OnWMDetected(Display* display, XErrorEvent* e); // detects if trying to run while another WM is running
      static bool wm_detected_; // set by OnWMDetected

    public: 
      static ::std::unique_ptr<WindowManager> Create(); //Factory Method
      ~WindowManager(); //Discnnects from the X server
      void Run(); //Entry point, begins main loop
    
};