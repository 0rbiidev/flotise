CXXFLAGS ?= -Wall -g
CXXFLAGS += -std=c++1y
CXXFLAGS += `pkg-config --cflags x11 libglog freetype2`
LDFLAGS += `pkg-config --libs x11 libglog freetype2`

all: flotise

HEADERS = \
	window_manager.hpp

SOURCES = \
	window_manager.cpp \
	main.cpp 
OBJECTS = $(SOURCES:.cpp=.o)

flotise: $(HEADERS) $(OBJECTS)
		$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f flotise $(OBJECTS)