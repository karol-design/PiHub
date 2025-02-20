#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

extern int run_network_tests(void);

int main()
{
  // Configure the CMocka results generation
  cmocka_set_message_output(CM_OUTPUT_STANDARD); // Can be ORed with CM_OUTPUT_XML for .xml generation

  int result = 0;
  result += run_network_tests();
  return result;
}
