// #include <cmocka.h>
// #include <netdb.h> // For: struct addrinfo, getaddrinfo(), socket()
// #include <setjmp.h>
// #include <stdarg.h>
// #include <stddef.h>
// #include <stdint.h>
// #include <sys/epoll.h> // For: epoll_event struct, epoll_create1, epoll_ctl

// #include "comm/network.h"

// extern ServerError_t server_init (ServerHandle_t* server);

// /************************ Mock functions ************************/

// /**
//  * @brief Mock function for getaddrinfo() from the Standard C LIbrary.
//  */
// int __wrap_getaddrinfo (const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res) {
//     check_expected_ptr (service);
//     check_expected_ptr (hints);
//     *res = (struct addrinfo*)mock ();
//     return (int)mock ();
// }

// /**
//  * @brief Mock function for socket() from the Standard C LIbrary.
//  */
// int __wrap_socket (int domain, int type, int protocol) {
//     check_expected (domain);
//     check_expected (type);
//     check_expected (protocol);
//     return (int)mock ();
// }

// /**
//  * @brief Mock function for bind() from the Standard C LIbrary.
//  */
// int __wrap_bind (int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
//     check_expected (sockfd);
//     check_expected_ptr (addr);
//     check_expected (addrlen);
//     return (int)mock ();
// }

// /**
//  * @brief Mock function for freeaddrinfo() from the Standard C LIbrary.
//  */
// void __wrap_freeaddrinfo (struct addrinfo* res) {
//     check_expected_ptr (res);
// }

// /**
//  * @brief Mock function for listen() from the Standard C LIbrary.
//  */
// int __wrap_listen (int __fd, int __n) {
//     check_expected (__fd);
//     check_expected (__n);
//     return (int)mock ();
// }

// /**
//  * @brief Mock function for bind() from the Standard C LIbrary.
//  */
// int __wrap_epoll_create1 (int __flags) {
//     check_expected (__flags);
//     return (int)mock ();
// }


// /**
//  * @brief Mock function for epoll_ctl() from the Standard C LIbrary.
//  */
// int __wrap_epoll_ctl (int __epfd, int __op, int __fd, struct epoll_event* event) {
//     check_expected (__epfd);
//     check_expected (__op);
//     check_expected (__fd);
//     check_expected_ptr (event);
//     return (int)mock ();
// }

// /**
//  * @brief Mock function for server_run_event_loop() from the "network" component
//  * @note Possible thanks to the fact that the internal function has weak linkage during Unit Testing
//  */
// ServerError_t server_run_event_loop (ServerHandle_t* server, int epoll_fd) {
//     check_expected_ptr (server);
//     check_expected (epoll_fd);
//     return (int)mock ();
// }


// /************************ Unit tests ************************/

// /* A test case for server_init - everything as expected; server_init should suceed */
// static void test_server_init_success (void** state) {
//     (void)state; // Unused parameter

//     ServerHandle_t server;
//     server.port = "65001";

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_string (__wrap_getaddrinfo, service, server.port); // For service we expect port from the Server Handle struct
//     expect_any (__wrap_getaddrinfo, hints);                   // For hints we do not have any expectations
//     will_return (__wrap_getaddrinfo, &addr);                  // For *res return &addr
//     will_return (__wrap_getaddrinfo, 0);                      // The whole function should return 0

//     expect_value (__wrap_socket, domain, AF_INET); // For domain we expect IPv4 (as defined previously in this test)
//     expect_value (__wrap_socket, type, SOCK_STREAM); // For type we expect TCP
//     expect_value (__wrap_socket, protocol, 0);       // For protocol we expect 0
//     will_return (__wrap_socket, 3);                  // Socket endpoint file descriptor to be returned

//     expect_value (__wrap_bind, sockfd, 3); // For sockfd we expect 3 since that should be returned by mocked socket()
//     expect_any (__wrap_bind, addr);        // No expectations for addr
//     expect_any (__wrap_bind, addrlen); // Nor for addrlen
//     will_return (__wrap_bind, 0);      // bind() should return 0

//     expect_value (__wrap_freeaddrinfo, res, &addr); // Make sure &addr is passed to freeaddrinfo() at the end of the function

//     // Call the function under test and verify the result
//     ServerError_t result = server_init (&server);
//     assert_int_equal (result, SERVER_ERR_OK);
// }

// /* A test case for server_init - incorrect parameter */
// static void test_server_init_null_param (void** state) {
//     (void)state; // Unused parameter

//     // Call the function under test and verify the result
//     ServerError_t result = server_init (NULL);
//     assert_int_equal (result, SERVER_ERR_NULL_ARGUMENT);
// }

// /* A test case for server_init - getaddrinfo() failed and server_init should return error */
// static void test_server_init_getaddrinfo_failed (void** state) {
//     (void)state; // Unused parameter

//     ServerHandle_t server;
//     server.port = "65001";

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_string (__wrap_getaddrinfo, service, server.port); // For service we expect port from the Server Handle struct
//     expect_any (__wrap_getaddrinfo, hints);                   // For hints we do not have any expectations
//     will_return (__wrap_getaddrinfo, &addr);                  // For *res return &addr
//     will_return (__wrap_getaddrinfo, -1);                     // Should return 0 (-1 means error)

//     expect_value (__wrap_freeaddrinfo, res, &addr); // Make sure freeaddrinfo() is called (server_addr is freed)

//     // Call the function under test and verify the result
//     ServerError_t result = server_init (&server);
//     assert_int_equal (result, SERVER_ERR_INIT_FAILURE);
// }

// /* A test case for server_init - socket() failed and server_init should return error */
// static void test_server_init_socket_failed (void** state) {
//     (void)state; // Unused parameter

//     ServerHandle_t server;
//     server.port = "65001";

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_string (__wrap_getaddrinfo, service, server.port); // For service we expect port from the Server Handle struct
//     expect_any (__wrap_getaddrinfo, hints);                   // For hints we do not have any expectations
//     will_return (__wrap_getaddrinfo, &addr);                  // For *res return &addr
//     will_return (__wrap_getaddrinfo, 0);                      // The whole function should return 0

//     expect_value (__wrap_socket, domain, AF_INET); // For domain we expect IPv4 (as defined previously in this test)
//     expect_value (__wrap_socket, type, SOCK_STREAM); // For type we expect TCP
//     expect_value (__wrap_socket, protocol, 0);       // For protocol we expect 0
//     will_return (__wrap_socket, -1); // Socket endpoint file descriptor to be returned (-1 means error)

//     expect_value (__wrap_freeaddrinfo, res, &addr); // Make sure freeaddrinfo() is called (server_addr is freed)

//     // Call the function under test and verify the result
//     ServerError_t result = server_init (&server);
//     assert_int_equal (result, SERVER_ERR_INIT_FAILURE);
// }

// /* A test case for server_init - bind() failed and server_init should return error */
// static void test_server_init_bind_failed (void** state) {
//     (void)state; // Unused parameter

//     ServerHandle_t server;
//     server.port = "65001";

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_string (__wrap_getaddrinfo, service, server.port); // For service we expect port from the Server Handle struct
//     expect_any (__wrap_getaddrinfo, hints);                   // For hints we do not have any expectations
//     will_return (__wrap_getaddrinfo, &addr);                  // For *res return &addr
//     will_return (__wrap_getaddrinfo, 0);                      // The whole function should return 0

//     expect_value (__wrap_socket, domain, AF_INET); // For domain we expect IPv4 (as defined previously in this test)
//     expect_value (__wrap_socket, type, SOCK_STREAM); // For type we expect TCP
//     expect_value (__wrap_socket, protocol, 0);       // For protocol we expect 0
//     will_return (__wrap_socket, 3);                  // Socket endpoint file descriptor to be returned

//     expect_value (__wrap_bind, sockfd, 3); // For sockfd we expect 3 since that should be returned by mocked socket()
//     expect_any (__wrap_bind, addr);        // No expectations for addr
//     expect_any (__wrap_bind, addrlen); // Nor for addrlen
//     will_return (__wrap_bind, -1);     // bind() should return 0 (-1 means error)

//     expect_value (__wrap_freeaddrinfo, res, &addr); // Make sure freeaddrinfo() is called (server_addr is freed)

//     // Call the function under test and verify the result
//     ServerError_t result = server_init (&server);
//     assert_int_equal (result, SERVER_ERR_INIT_FAILURE);
// }

// /* A test case for server_init - everything as expected; server_start should suceed */
// static void test_server_start_success (void** state) {
//     (void)state; // Unused parameter

//     ServerHandle_t server;
//     server.port = "65001";
//     server.fd = 1;

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_value (__wrap_listen, __fd, server.fd);
//     expect_value (__wrap_listen, __n, server.max_queued_incomming_conn);
//     will_return (__wrap_listen, 0);

//     expect_value (__wrap_epoll_create1, __flags, 0);
//     will_return (__wrap_epoll_create1, 5); // Mocked epoll File Descriptor = 5

//     expect_value (__wrap_epoll_ctl, __epfd, 5);
//     expect_value (__wrap_epoll_ctl, __op, EPOLL_CTL_ADD);
//     expect_value (__wrap_epoll_ctl, __fd, server.fd);
//     expect_any (__wrap_epoll_ctl, event);
//     will_return (__wrap_epoll_ctl, 0);

//     expect_value (server_run_event_loop, server, &server);
//     expect_value (server_run_event_loop, epoll_fd, 5);
//     will_return (server_run_event_loop, SERVER_ERR_OK);

//     ServerError_t result = server_start (&server);
//     assert_int_equal (result, SERVER_ERR_OK);
// }

// int run_network_tests (void) {
//     const struct CMUnitTest network_tests[] = { cmocka_unit_test (test_server_init_success),
//         cmocka_unit_test (test_server_init_null_param), cmocka_unit_test (test_server_init_getaddrinfo_failed),
//         cmocka_unit_test (test_server_init_socket_failed), cmocka_unit_test (test_server_init_bind_failed),
//         cmocka_unit_test (test_server_start_success) };
//     return cmocka_run_group_tests (network_tests, NULL, NULL);
// }