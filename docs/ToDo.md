## ToDo:
- Add server_get_client_ip(), server_get_clients_list() [4h]
- Add on_server_failure() [8h]
- Commit all changes to git (and remove the earlier commit!)
- Review the code with Mateusz (should I modify sth before I write the tests and move on to the next components)
- Write unit tests for network.c and list.c [10h]

- Learn how to approach memory management (RAII) and error checking and handling
- What should be my policy regarding checking if pointers are NULL? 

## Key Improvements
- Maybe I could add on_server_failure so that it can read the error, clean the memory and potentially restart the server

## Optional improvements
- Add compiler specification to CMakeLists.txt
- I could add a Cat.M/NB-IoT module to the RPI; install Ofono, use a SIM with a static IP and make the whole server completely remote (e.g. for farming)

## Usefull commands
> cmake -B build -DUT=ON --debug-output
> cmake --build build/ --target clean [*to clean*]
> rm build/ -rf && cmake -B build -DUT=ON && cmake --build build && ./build/tests/test_piHub
> rm build/ -rf && cmake -B build && cmake --build build && ./build/src/piHub
> rm build/ -rf && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ./build/src/piHub
> clang-tidy src/... (e.g. clang-tidy src/comm/network.c)
> lsof -ti:65001 | xargs kill -9 (to kill the process for socket listening)
> nc 127.0.0.1 65001 (to test the server)
> valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./build/src/piHub (for memory leak check)

## Questions 
- What is the best approach to handling errors? An example of a common problematic situation: a client thread receives data, invokes a callback, which calls the server_read function, which then may try to modify the linked list via associated llist functions - if llist function fails due to malloc returning NULL, should I try again within the llist function itself, or maybe I should return and try invoking llist function again in the server_read function, or maybe I should keep returning error codes from the called functions up to the point where the callback returns an error to the client thread, and this in response terminates (?) Are there maybe any general guidelines or rules of thumb on how to deal with error handling? 
- What is the best approach to object/memory ownership management? Are there maybe any general guidelines or rules of thumb on how to deal with freeing memory / garbage collection? I came across "RAII patterns for cleanup" while quickly reading on this topic - could that be a valid approach in my case, or are there approaches better suited to C (Linux) code?
- Should I generally use OOP-like structs (classes) or only when it is completely necessary? The main problem -> whenever I get a ptr to astruct, I should probably check for all pointers (incl. 5/10 function pointers) :/
