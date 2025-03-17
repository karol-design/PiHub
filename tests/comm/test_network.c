// #include <netdb.h> // For: struct addrinfo, getaddrinfo(), socket()
// #include <setjmp.h>
// #include <stdarg.h>
// #include <stddef.h>
// #include <stdint.h>
// // Cmocka must be included last (!)
// #include <cmocka.h>
// #include <sys/epoll.h> // For: epoll_event struct, epoll_create1, epoll_ctl

// #include "comm/network.h"

// /* IGNORE: pthread_mutex_init, pthread_mutex_lock, pthread_mutex_unlock, pthread_mutex_destroy,
// /* pthread_create, pthread_detach, pthread_exit, eventfd*, epoll_create1, epoll_wait, epoll_ctl
// /* STUB: send, recv, inet_ntop, getpeername
// /* MOCK:
// /* ASSESS: accept, freeaddrinfo, bind, close, listen, getaddrinfo, socket, write


// /************************ Mock functions ************************/

// int __wrap_getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res) {
//     *res = (struct addrinfo*)mock();
//     return (int)mock();
// }

// int __wrap_socket(int domain, int type, int protocol) {
//     return (int)mock();
// }

// int __wrap_bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
//     return (int)mock();
// }

// void __wrap_freeaddrinfo(struct addrinfo* res) {
//     check_expected_ptr(res);
// }

// int __wrap_listen(int __fd, int __n) {
//     return (int)mock();
// }

// int __wrap_epoll_create1(int __flags) {
//     check_expected(__flags);
//     return (int)mock();
// }

// int __wrap_epoll_ctl(int __epfd, int __op, int __fd, struct epoll_event* event) {
//     return (int)mock();
// }


// /************************ Unit tests ************************/

// static void test_server_init_success(void** state) {
//     (void)state;

//     Server_t server;
//     ServerConfig_t cfg = { .port = "65001",
//         .cb_list = { .on_client_connect = (void*)1,
//         .on_client_disconnect = (void*)1,
//         .on_data_received = (void*)1,
//         .on_server_failure = (void*)1 } };

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     will_return(__wrap_getaddrinfo, &addr);
//     will_return(__wrap_getaddrinfo, 0);

//     will_return(__wrap_socket, 3);

//     will_return(__wrap_bind, 0);

//     expect_value(__wrap_freeaddrinfo, res, &addr);

//     ServerError_t result = server_init(&server, cfg);
//     assert_int_equal(result, SERVER_ERR_OK);
// }

// static void test_server_init_null_param(void** state) {
//     (void)state;

//     ServerError_t result = server_init(NULL, (ServerConfig_t){ 0 });
//     assert_int_equal(result, SERVER_ERR_NULL_ARGUMENT);
// }

// static void test_server_init_getaddrinfo_failed(void** state) {
//     (void)state;

//     Server_t server;
//     ServerConfig_t cfg = { .port = "65001",
//         .cb_list = { .on_client_connect = (void*)1,
//         .on_client_disconnect = (void*)1,
//         .on_data_received = (void*)1,
//         .on_server_failure = (void*)1 } };

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_string(__wrap_getaddrinfo, service, cfg.port);
//     expect_any(__wrap_getaddrinfo, hints);
//     will_return(__wrap_getaddrinfo, &addr);
//     will_return(__wrap_getaddrinfo, -1);

//     expect_value(__wrap_freeaddrinfo, res, &addr);

//     ServerError_t result = server_init(&server, cfg);
//     assert_int_equal(result, SERVER_ERR_NET_FAILURE);
// }

// static void test_server_init_socket_failed(void** state) {
//     (void)state;

//     Server_t server;
//     ServerConfig_t cfg = { .port = "65001",
//         .cb_list = { .on_client_connect = (void*)1,
//         .on_client_disconnect = (void*)1,
//         .on_data_received = (void*)1,
//         .on_server_failure = (void*)1 } };

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_string(__wrap_getaddrinfo, service, cfg.port);
//     expect_any(__wrap_getaddrinfo, hints);
//     will_return(__wrap_getaddrinfo, &addr);
//     will_return(__wrap_getaddrinfo, 0);

//     expect_value(__wrap_socket, domain, AF_INET);
//     expect_value(__wrap_socket, type, SOCK_STREAM);
//     expect_value(__wrap_socket, protocol, 0);
//     will_return(__wrap_socket, -1);

//     expect_value(__wrap_freeaddrinfo, res, &addr);

//     ServerError_t result = server_init(&server, cfg);
//     assert_int_equal(result, SERVER_ERR_NET_FAILURE);
// }

// static void test_server_init_bind_failed(void** state) {
//     (void)state;

//     Server_t server;
//     ServerConfig_t cfg = { .port = "65001",
//         .cb_list = { .on_client_connect = (void*)1,
//         .on_client_disconnect = (void*)1,
//         .on_data_received = (void*)1,
//         .on_server_failure = (void*)1 } };

//     struct addrinfo addr;
//     addr.ai_family = AF_INET;
//     addr.ai_socktype = SOCK_STREAM;
//     addr.ai_protocol = 0;

//     expect_string(__wrap_getaddrinfo, service, cfg.port);
//     expect_any(__wrap_getaddrinfo, hints);
//     will_return(__wrap_getaddrinfo, &addr);
//     will_return(__wrap_getaddrinfo, 0);

//     expect_value(__wrap_socket, domain, AF_INET);
//     expect_value(__wrap_socket, type, SOCK_STREAM);
//     expect_value(__wrap_socket, protocol, 0);
//     will_return(__wrap_socket, 3);

//     expect_value(__wrap_bind, sockfd, 3);
//     expect_any(__wrap_bind, addr);
//     expect_any(__wrap_bind, addrlen);
//     will_return(__wrap_bind, -1);

//     expect_value(__wrap_freeaddrinfo, res, &addr);

//     ServerError_t result = server_init(&server, cfg);
//     assert_int_equal(result, SERVER_ERR_NET_FAILURE);
// }

// int run_network_tests(void) {
//     const struct CMUnitTest network_tests[] = {
//         cmocka_unit_test(test_server_init_success),
//         cmocka_unit_test(test_server_init_null_param),
//         cmocka_unit_test(test_server_init_getaddrinfo_failed),
//         cmocka_unit_test(test_server_init_socket_failed),
//         cmocka_unit_test(test_server_init_bind_failed),
//     };
//     return cmocka_run_group_tests(network_tests, NULL, NULL);
// }
