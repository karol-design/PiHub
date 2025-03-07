#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// Cmocka must be included last (!)
#include <cmocka.h>

extern int run_network_tests(void);
extern int run_llist_tests(void);
extern int run_dispatcher_tests(void);

int main() {
    // Configure the CMocka results generation
    cmocka_set_message_output(CM_OUTPUT_STANDARD); // Can be ORed with CM_OUTPUT_XML for .xml generation

    int result = 0;
    // result += run_network_tests();
    result += run_llist_tests();
    result += run_dispatcher_tests();
    return result;
}
