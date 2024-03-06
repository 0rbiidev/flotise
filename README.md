# flotise
A concept window manager utilising **flo**ating **ti**le **se**ts

## About 

**flotise** is a concept window manager which allows windows to be tiled within floating, rearrangeable and resizable frames. 
The idea behind **flotise** is to offer an alternative to the typical virtual desktop system of arranging applications
The benefit of this system over virtual desktops is the ability to arrange these collections of applications in a similar way
to a multi-monitor setup

## Dependencies

### Build
- google-glog library
- CMake
- Xlib libraries and headers
- A C++ compiler with C++-11 compatibility

### Test
To run flotise in testing mode, ensure the build dependencies are met along with following additional dependencies

- Xephyr
- xinit
- xterm, xeyes

Then run the following script in the root of the repository.

    ./run.sh

## Thanks

Special thanks to [basic_wm](https://github.com/jichu4n/basic_wm) by jichu4n and it's accompanying tutorial for acting as a 
jumping off point for this project and teaching me the basics of the Xlib.h library.