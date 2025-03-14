# Gather all test sources
file(GLOB_RECURSE UT_SOURCES
        ${CMAKE_SOURCE_DIR}/tests/**.c
        ${CMAKE_SOURCE_DIR}/src/app/*.c
        ${CMAKE_SOURCE_DIR}/src/comm/*.c
        ${CMAKE_SOURCE_DIR}/src/hw/*.c
        ${CMAKE_SOURCE_DIR}/src/utils/*.c)

# Create a variable for mock functions
set(MOCK_FUNCTIONS)

# Add mock functions for std lib networking
# string(APPEND MOCK_FUNCTIONS "-Wl,--wrap=getaddrinfo -Wl,--wrap=socket -Wl,--wrap=bind -Wl,--wrap=freeaddrinfo -Wl,--wrap=listen ")
# Add mock functions for pthread
# string(APPEND MOCK_FUNCTIONS "-Wl,--wrap=pthread_mutex_init -Wl,--wrap=pthread_mutex_lock -Wl,--wrap=pthread_mutex_unlock -Wl,--wrap=pthread_mutex_destroy -Wl,--wrap=pthread_create -Wl,--wrap=pthread_detach -Wl,--wrap=pthread_exit ")
# Add mock functions for std lib epoll management
# string(APPEND MOCK_FUNCTIONS "-Wl,--wrap=epoll_create1 -Wl,--wrap=epoll_ctl ")
# Add mock functions for server component methods
# string(APPEND MOCK_FUNCTIONS "-Wl,--wrap=server_run_event_loop")

### Register the test with CMocka
add_cmocka_test(test_piHub
                SOURCES ${UT_SOURCES}
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_OPTIONS ${DEFAULT_LINK_FLAGS} ${MOCK_FUNCTIONS}
                LINK_LIBRARIES cmocka::cmocka)

# Define UT to enable weak linkage for mocked internal functions and external linkage for all internal functions
target_compile_definitions(test_piHub PRIVATE UT=1)

# Add 'include' to dirs the compiler uses to search for include files.
include_directories(${CMAKE_SOURCE_DIR}/tests)
