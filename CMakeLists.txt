cmake_minimum_required(VERSION 3.0.2)
project(execution_context_shmtime)

add_compile_options(-std=c++11)
set(CMAKE_BUILD_TYPE Release)

# openrtm has deprecated coding. This causes too many warnings.
#   warning: dynamic exception specifications are deprecated in C++11
add_definitions(-Wno-deprecated)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  )

find_package(PkgConfig)
pkg_check_modules(openrtm_aist openrtm-aist REQUIRED)
catkin_package(
#  INCLUDE_DIRS include
  LIBRARIES ${PROJECT_NAME}
#  CATKIN_DEPENDS other_catkin_pkg
#  DEPENDS system_lib
)

include_directories(
  ${catkin_INCLUDE_DIRS}
  ${openrtm_aist_INCLUDE_DIRS})
link_directories(
  ${catkin_LIBRARY_DIRS}
  ${openrtm_aist_LIBRARY_DIRS})
add_library(ShmTimePeriodicExecutionContext
  src/ShmTimePeriodicExecutionContext.cpp)
target_link_libraries(ShmTimePeriodicExecutionContext
  ${catkin_LIBRARIES}
  ${openrtm_aist_LIBRARIES})
set_target_properties(ShmTimePeriodicExecutionContext PROPERTIES PREFIX "")
install(TARGETS ShmTimePeriodicExecutionContext
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION})
