# PiHub | Smart Home Control Hub

## Project Description
### Objective
Create a Raspberry Pi smart home system (C server and Python client) for remote control of home appliances (through GPIO), and querying the readings of multiple environmental sensors (temperature and humidity), all via a pre-defined set of commands (custom text-based protocol running over TCP).

### Typical high-level use case description
A user would run a Python client script on a PC/laptop/tablet to control devices (through relays connected to RPI GPIOs) and query the temperature and humidity in various rooms where sensors are installed, as well as the current server parameters (in particular: basic network statistics, system uptime, CPU temperature).

## Requirements
> *Note*: Requirements are grouped into three priority tiers from the most critical [P0] to optional [P2] 
### Functional Requirements
1. [P0] The system shall allow clients to control the state of available GPIO pins (up to 10) by sending commands which specify the operation (`TURN_ON`/`TURN_OFF`) and GPIO number (e.g. `TURN_ON 10`);
2. [P0] The system shall support querying the current temperature and humidity (`GET_TEMP`/`GET_HUM`) from any currently available environmental sensor identified in the query by a pre-defined sensor ID (e.g. `GET_TEMP S1`);
3. [P0] The server shall respond with an acknowledgment (`OK`) within 1 second of receiving a valid command;
4. [P0] The system shall provide error messages for invalid commands or connection issues (e.g., `INVALID CMD` or `CONN ERROR`).
5. [P0] The server shall support a command to retrieve the status of connected sensors by their pre-defined sensor ID (e.g. `GET_STATE S1`).
6. [P0] The system shall allow clients to disconnect gracefully by sending a `DISCONNECT` command.
7. [P0] The system shall allow any client to shut down the server by sending a `SHUTDOWN` command.
8. [P1] The system shall support a set of commands to retrieve the server's operational parameters, in particular: basic network statistics, system uptime or CPU temperature (e.g.`GET_NETWORK_STATS`, `GET_UPTIME`, `GET_CPU_TEMP`).
9. [P1] The server shall log all incoming commands and their execution status, including timestamps.
10. [P1] The commands should be customizable via a config file in which the user can overwrite default cmds.
11. [P2] The server shall validate the connected environmental sensor`s presence and functionality during startup and notify the user of the state of the system (incl. connected sensors) in a first message after succesfull connection.
12. [P2] The server should provide a protection mechanism against abandoned connections from the clients in form of a timeout.
13. [P2] The server should be implemented so that it can run as a deamon in Linux environment.
14. [P2] The server should verify all incomming connections (e.g. via password/login or key exchange)

### Non-Functional Requirements
1. [P0] The system shall handle up to 5 simultaneous client connections without degradation in response time.
2. [P0] The system shall implement a text-based application-layer protocol (as defined in this document) designed to run over TCP.
2. [P0] The server shall operate reliably on a Raspberry Pi Zero 2W with default Raspberry Pi OS (32-bit version, Lite or Full).
3. [P0] The client script shall operate reliably on any device that supports Python 3.6 or higher and is connected to the network (e.g., Windows, Linux, or macOS).
4. [P0] The server shall support up to 5 environmental sensors connected through I2C or SPI (e.g., DHT22, BME280, or similar).
5. [P0] The system's response time for any command shall not exceed 2 seconds under typical load conditions.
6. [P0] The server app shall be modular, allowing additional GPIO devices or sensors to be integrated with minimal code changes - in particular, a virtual interface should be provided for each sensor that would allow to integrate a new sensor without making any modifications into current server's code components.
7. [P1] The server app shall provide logs in a standard format (e.g., JSON or plain text) that can be exported or analyzed offline.
8. [P1] The server app shall provide robust error handling, ensuring that invalid or incomplete commands do not crash the server.
9. [P2] For improved security, a deamon user should be created on server installation, which would have limited rights (e.g. no default shell access) and would run the server's deamon. In addition "append only" mode should be specified for log files.
10. [P2] In case the server is implemented as a deamon an install/uninstall script should be provided as well.

## Architecture

### Software stack / technologies
| Subsystem | Environment | Language | Build system | Compiler | Test framework | Debugger | Code formatter | Static analysis | Test coverage analysis | Memory analysis |
| -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Server | Linux | C99 or higher | CMake | gcc | CMocka | gdb | Clang-format | Clang-tidy | ? | Valgrind |
| Client | Linux/Windows/Mac | Python | - | - | - | - | - | - | - | - |

### Server (components / src structure)
- main (calls piHub_init(), piHub_start())
- app: **piHub** (Application layer: configures the whole server and external hardware, run main application loop)
- app: **cmd_parser** (Interpretter for pre-defined/custom commands: validating and interpretting commands, managing other PiHub protocol functionalities, e.g. encrytpion if used)
- comm: **network** (TCP/IP Connection handler: initialising TCP server, listening for connections, sending / receiving data, managing connections - disconnect, shutdown)
- hw: **gpio** (Abstraction for GPIO: initializing GPIO, configuring input/output, setting/resetting GPIOs, reading the value on GPIOs)
- hw: **i2c** (Abstraction for I2C: initializing and configuring I2C, managing sending/receiving data)
- hw: **spi** (Abstraction for SPI: initializing and configuring SPI, managing sending/receiving data)
- hw: **sensor** (Header file with a virtual interface (class) typedef)
- hw: **dht22** (Abstraction for a particular sensor, e.g. dht22: initializing and configuring DHT22; reading measurements)
- logs: **log** (Logging utilities: creating a log file; writing to the file when required adding valuable log/debug informations)
- utils: **list** (Linked List: creating, using and deleting linked lists)
- utils: **config** (Server configuration: )
- utils: **common** (Common utilities: )

### Server (Multithreading):
One for the app (main)
One for the Server (to monitor incomming connections)
One for each client

### Client
- ...
