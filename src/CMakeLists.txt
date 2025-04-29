file(GLOB_RECURSE PIHUB_SRC_FILES
        ${CMAKE_SOURCE_DIR}/src/app/*.c
        ${CMAKE_SOURCE_DIR}/src/comm/*.c
        ${CMAKE_SOURCE_DIR}/src/hw/*.c
        ${CMAKE_SOURCE_DIR}/src/sensors/*.c
        ${CMAKE_SOURCE_DIR}/src/utils/*.c)

# Create static library
add_library(pihub_static STATIC ${PIHUB_SRC_FILES})

# Create main executable
add_executable(${CMAKE_PROJECT_NAME} main.c)

# Link the main executable with the PiHub and Gpiod Libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE pihub_static)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE gpiod)

# Enable logs by default
target_compile_definitions(pihub_static PRIVATE LOGS_ENABLED=1)