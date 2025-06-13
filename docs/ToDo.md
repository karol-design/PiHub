## ToDo:
- Design the electronics for the project [16h] (in progress)
- Develop a client script in python (automatic graphing temp + another user) [8h] (in progress)

- Implement SPI support and complete hw_interface component [3h]
- Make logging thread-safe [3h]
- Do a final review of all 10 components [4h]
- Prepare a demo and a presentation (What was my goal; what I achieved; What could be improved - by category) reviewing key topics in the process [30h]

- Test remote debugging with gdb [1]
- Add a cellular module with static SIM and oFono [6]

Remote SSH: https://www.raspberrypi.com/documentation/services/connect.html
DEMO: showing PiHub running as a Systemd daemon; showing remote access; showing logs in journalctl; reading sensor data; reading GPIO state; writing GPIO state; checking server status; crashing the server (and showing restart attempt);
LEDs: 5,6,7,8 | SWs: 21,22,23,24

## Building, testing, installing and running
> sudo su --> cmake -B build -DUT=ON -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ./build/tests/test_piHub && cmake --build ./build --target uninstall && cmake --install build && systemctl daemon-reload && systemctl enable pihub && systemctl start pihub && journalctl -u pihub -n 20 && exit

## Usefull commands
> ./build.sh --run
> cmake -B build -DUT=ON --debug-output
> cmake --build build/ --target clean [*to clean*]
> rm build/ -rf && cmake -B build -DUT=ON -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ./build/tests/test_piHub && ./build/src/pihubd
> clang-tidy src/... (e.g. clang-tidy src/comm/network.c)
> lsof -ti:65002 | xargs kill -9 (to kill the process for socket listening)
> nc 127.0.0.1 65002 (to test the server)
> valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./build/src/piHub (for memory leak check)
> gdb ./build/src/piHub

> sudo i2cdetect -y 1 (To list all I2C devices on dev/i2c-2)
> sudo i2cget -y 1 0x76 0xD0 (To check the status register)
> sudo i2cdump -y 1 0x76 (To read and dump the content of all registers)

> sudo apt install libsystemd-dev

> sudo cp pihub.service /etc/systemd/system/ (to copy the service unit file to the systemd/system)
> sudo chmod 644 /etc/systemd/system/pihub.service
> sudo mv build/src/piHub /usr/local/bin/pihubd (to copy the pihubd binary to bin/)

> sudo cmake --install build (alternative for the commands above)
> sudo cmake --build ./build --target uninstall (includes disabling, stopping and removing pihubd)

> sudo systemctl enable pihub.service
> sudo systemctl daemon-reload (to reload unit files in systemd)
> sudo systemctl start pihub.service
> sudo systemctl stop pihub.service
> sudo systemctl status pihub.service
> sudo journalctl --since="today" -u pihub.service -f

> python3 ./tools/plot_temperature.py (to run a test script)

> eval "$(ssh-agent -s)" && ssh-add ~/.ssh/id_ed25519_gitlabua

## Identified problems / improvements
- build automation: add build/debug/test integration to Cmake or VScode
- unit tests: should include checks whether mutexes are taken and !released
- server: When client worker thread receives the disconnect signal it eventually destroys its mutex, but at this time it can be already taken by another thread (e.g. main, which wants to write to that client);
- server: The shutdown procedure could use a semaphore to wait for all client threads to finish (!)

## Optional improvements
- Add compiler specification to CMakeLists.txt
- I could add a Cat.M/NB-IoT module to the RPI; install Ofono, use a SIM with a static IP and make the whole server completely remote (e.g. for farming)

## To consider with each component:
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
5) i2c & sensor
6) bme280
7) gpio
8) system (partially)

## ToDo:
1) SPI
2) piHub