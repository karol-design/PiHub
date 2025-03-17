## ToDo:
- Implement I2C support [12h]
- Add component for a specific sensor [12h]
- Implement GPIO support [12h]
- Implement System stats support [16h]
- Learn python fundamentals and develop a client script [24h]


## Identified problems / improvements
- build automation: add build/debug/test integration to Cmake or VScode
- unit tests: should include checks whether mutexes are taken and !released
- server: When client worker thread receives the disconnect signal it eventually destroys its mutex, but at this time it can be already taken by another thread (e.g. main, which wants to write to that client);
- server: The shutdown procedure could use a semaphore to wait for all client threads to finish (!)

## Usefull commands
> ./build.sh --run
> cmake -B build -DUT=ON --debug-output
> cmake --build build/ --target clean [*to clean*]
> rm build/ -rf && cmake -B build -DUT=ON -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ./build/tests/test_piHub && ./build/src/piHub
> clang-tidy src/... (e.g. clang-tidy src/comm/network.c)
> lsof -ti:65002 | xargs kill -9 (to kill the process for socket listening)
> nc 127.0.0.1 65002 (to test the server)
> valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./build/src/piHub (for memory leak check)
> gdb ./build/src/piHub

> sudo i2cdetect -y 1 (To list all I2C devices on dev/i2c-2)
> sudo i2cget -y 1 0x5d 0xD0 (To check the status register)
> sudo i2cdump -y 1 0x5d (To read and dump the content of all registers)

## Questions 
- Should I create an abstraction for server, e.g. server_read() calls server->read() which points to _server_read()? I think I will just remove function pointers completely, as I see I do not have an application for them yet and they add an additional complexity and safety thread

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
2) dispatcher
3) llist
4) log [opt]

## ToDo:
1) i2c
2) sensor & dht22
3) gpio
4) system
5) piHub

## Project achievements:
- Unit and functional tests with simple mocks/stubs and fixtures