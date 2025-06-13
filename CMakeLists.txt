# Works with 3.15
cmake_minimum_required(VERSION 3.15)

# Define basic project details
project(
     pihubd
     VERSION 0.1
     DESCRIPTION "Simple smart-home server"
     LANGUAGES C)

# Add 'include' to dirs the compiler uses to search for include files.
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add CMocka Tests support
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/cmocka)

add_subdirectory(src)

# Unit Testing can be enabled by -DUT=ON (OFF by default)
option(UT "Build Unit Tests along the main app" OFF)
if (UT)
     find_package(cmocka 1.1.0 REQUIRED)
     include(AddCMockaTest)
     add_subdirectory(tests)
else()
     # No action at the moment
endif()
unset(USE_MY_LIB CACHE)

# Add configuration for the install target

# Install binary to /usr/local/bin
install(TARGETS pihubd RUNTIME DESTINATION bin)

# Install systemd service to the correct location
install(FILES pihub.service DESTINATION /etc/systemd/system)

# Add uninstall target
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY
)

add_custom_target(uninstall
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)