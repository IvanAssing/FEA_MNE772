cmake_minimum_required(VERSION 2.8.11)

project(FEA_MNE772)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set directories
SET(VTK_DIR "/opt/vtk8r/lib/cmake/vtk-8.1" CACHE PATH "VTK directory override" FORCE)
SET(Qt5_DIR "/opt/qt-5.9.1/5.9.1/gcc_64/lib/cmake/Qt5")
SET(Qt5Core_DIR "/opt/qt-5.9.1/5.9.1/gcc_64/lib/cmake/Qt5Core")
SET(Qt5Gui_DIR "/opt/qt-5.9.1/5.9.1/gcc_64/lib/cmake/Qt5Gui")
SET(Qt5Widgets_DIR "/opt/qt-5.9.1/5.9.1/gcc_64/lib/cmake/Qt5Widgets")
SET(Qt5OpenGL_DIR "/opt/qt-5.9.1/5.9.1/gcc_64/lib/cmake/Qt5OpenGL")

# List source files & resources
file (GLOB_RECURSE Sources *.cpp)
file (GLOB_RECURSE Headers *.h)
file (GLOB_RECURSE Resources *.qrc)
file (GLOB_RECURSE UIs *.ui)

# Find package Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)

# Use Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
qt5_add_resources(Resources ${Resources})

# Find package VTK
find_package(VTK REQUIRED)

# Use VTK
include(${VTK_USE_FILE})
include_directories(${VTK_INCLUDE_DIRS})

# Build executable
add_executable(FEA_MNE772 ${Sources} ${Headers} ${Resources} ${UIs})

# Link libraries
target_link_libraries(FEA_MNE772 Qt5::Widgets Qt5::OpenGL ${VTK_LIBRARIES})
