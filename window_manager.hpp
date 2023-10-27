extern "C"{
#include <X11/Xlib.h>
}

#include <memory>
#include <unordered_map>

class WindowManager{
    private:
      WindowManager(Display* display);
      Display* display_;
      const Window root_;
      ::std::unordered_map<Window, Window> clients_; //Maps windows to their respective frames

      void Frame(Window w);
      void Unframe(Window w);

      // Event handlers
      void OnCreateNotify(const XCreateWindowEvent& e);
      void OnDestroyNotify(const XDestroyWindowEvent& e);
      void OnConfigureRequest(const XConfigureRequestEvent& e);
      void OnMapRequest(const XMapRequestEvent& e);
      void OnConfigureNotify(const XConfigureEvent& e);
      void OnUnmapNotify(const XUnmapEvent& e);

      // Error handlers
      static int OnXError(Display* display, XErrorEvent* e); // error handler, passes address to Xlib
      static int OnWMDetected(Display* display, XErrorEvent* e); // detects if trying to run while another WM is running
      static bool wm_detected_; // set by OnWMDetected

    public: 
      static ::std::unique_ptr<WindowManager> Create(); //Factory Method
      ~WindowManager(); //Discnnects from the X server
      void Run(); //Entry point, begins main loop
    
};