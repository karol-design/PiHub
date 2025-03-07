# Project testing guidelines

## Unit tests:
### Rules of thumbs:
1) **Avoid striving for 100% code coverage** – Focus on critical paths; complete coverage is rarely necessary and can be counterproductive.  
2) **Test behavior, not implementation** - Unit tests should verify input/output behavior, allowing refactoring without breaking tests.  
3) **Ensure tests can fail** – Tests that always pass provide no value; they must be able to detect failures.  
4) **Write clear, well-structured tests** – Use the Arrange-Act-Assert pattern for readability and maintainability.  
5) **Automate test execution** – Run tests on every build to catch issues early and ensure consistency.  
6) **Use stubs and mocks appropriately** – Isolate external dependencies like APIs, hardware interfaces, or system calls when necessary (e.g. output changes in real-time).  
7) **Employ parameterized tests** – Use different inputs in the same test logic to improve coverage and avoid redundancy.  
8) **Write simple and readable tests** – Avoid complex logic in tests to minimize errors and improve clarity.  
9) **Ensure tests are deterministic** – Tests should always yield the same results under identical conditions.  
10) **Test one scenario per test** – Keep tests focused to make debugging failures easier.  
11) **Avoid test interdependence** – Tests should not depend on each other’s execution or results.  
12) **Avoid logic in tests** – Minimize loops or conditionals in tests to prevent errors.  
13) **Use descriptive test names** – Clearly state the method being tested, the scenario, and the expected outcome.  
14) **Write minimally passing tests** – Start with the simplest input to verify behavior before adding complexity.  
15) **Avoid magic strings and numbers** – Use named constants or variables instead of hardcoded values.  
16) **Isolate the unit under test** – Ensure it runs independently of external factors.  
17) **Ensure tests are fast** – Quick tests encourage frequent execution and smooth integration.  
18) **Combine unit and integration testing** – Unit tests validate components, while integration tests check interactions.  
19) **Test for security issues** – Include tests for vulnerabilities like input validation failures.  
20) **Maintain tests alongside code changes** – Regularly update tests to reflect changes and prevent outdated test cases.  

### General guidelines:
1) Check include list of the component and identify all external API function calls ignoring those from the std library dealing with simple memory, string etc. operations and ignoring any calls to functions from other components (we do not want to mock own functions, especially if they are tested too). Example of calls to be noted down: pthread_mutex_init, getpeername, accept etc.
2) Analyse which of these API calls cannot be used during unit testing (e.g. because they rely on some real-time elements [e.g. SPI/I2C/Time])
3) Use stubs when you do not want to test system behaviour if these API calls fail (simply return a constant value indicating success)
4) Use mocks (more or less complete) if you want to explicitly test failures and input parameters
5) In general avoid using stubs and mocks as this tend to get overcomplicated and unnecessary quickly
