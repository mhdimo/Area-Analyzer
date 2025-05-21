cmake_minimum_required(VERSION 3.10)
project(TabletAnalyzer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific flags
if(APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices)
    set(EXTRA_LIBS ${COREFOUNDATION_LIBRARY} ${APPLICATIONSERVICES_LIBRARY})
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    set(EXTRA_LIBS ${X11_LIBRARIES})
elseif(WIN32)
    set(EXTRA_LIBS user32)
endif()

# Include headers
include_directories(include)

# Source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(tablet_analyzer ${SOURCES})
target_link_libraries(tablet_analyzer ${EXTRA_LIBS})
