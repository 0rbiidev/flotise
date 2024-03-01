cmake_minimum_required(VERSION 3.20)

find_package (glog 0.7.0 REQUIRED)

add_executable(flotise)
target_link_libraries(flotise glog::glog)
target_link_libraries(flotise -lX11 -lfreetype)
target_sources(flotise PRIVATE main.cpp window_manager.cpp)
target_include_directories(flotise PRIVATE /usr/include/freetype2)
install(TARGETS flotise)
