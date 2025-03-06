## ToDo:
- Write unit tests for list.c [10h]
- Generate unit tests for network.c and parser.c [16h]
- Create the virtual interface (class) for the sensor [8h]
- Implement I2C support [24h]
- Add component for a specific sensor [12h]
- Implement GPIO support [12h]
- Implement System stats support [16h]
- Implement the logging library [16h]

## Identified problems / improvements
- server: Remove malloc from server_create (replace that with _init) and instead use the approach from cmd_parser; this will also simplify _destroy() as no free() will be required;
- server: When client worker thread receives the disconnect signal it eventually destroys its mutex, but at this time it can be already taken by another thread (e.g. main, which wants to write to that client);
- server: The shutdown procedure could use a semaphore to wait for all client threads to finish (!)

## Usefull commands
> cmake -B build -DUT=ON --debug-output
> cmake --build build/ --target clean [*to clean*]
> rm build/ -rf && cmake -B build -DUT=ON && cmake --build build && ./build/tests/test_piHub
> rm build/ -rf && cmake -B build && cmake --build build && ./build/src/piHub
> rm build/ -rf && cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ./build/src/piHub
> clang-tidy src/... (e.g. clang-tidy src/comm/network.c)
> lsof -ti:65002 | xargs kill -9 (to kill the process for socket listening)
> nc 127.0.0.1 65002 (to test the server)
> valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./build/src/piHub (for memory leak check)
> gdb ./build/src/piHub

## Questions 
- Should I create an abstraction for server, e.g. server_read() calls server->read() which points to _server_read()?

## Optional improvements
- Add compiler specification to CMakeLists.txt
- I could add a Cat.M/NB-IoT module to the RPI; install Ofono, use a SIM with a static IP and make the whole server completely remote (e.g. for farming)

## TO consider with each component:
- API architecture
- Threads, shared resources and synchronisation
- Error handling procedures
- Memory management (in particular garbage collection)

# Components:
## Finished:
1) network
2) parser
3) llist
4) log [opt]

## ToDo:
1) i2c
2) sensor & dht22
3) gpio
4) system
5) piHub