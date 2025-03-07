# 📡 PiHub | Smart Home Control Hub

## 📌 Project Description
### Objective
Develop a Raspberry Pi-based smart home control system with a C-based server and a Python client, enabling:
- Remote control of home appliances via GPIO.
- Environmental sensor data retrieval (temperature, humidity, etc.).
- Monitoring of system parameters such as network status, uptime, and CPU temperature.
- All via a structured, UNIX-style command interface over TCP for extensibility and automation.

### Typical high-level use case description
A user runs a Python client on a PC, laptop, or tablet to:
- Control home appliances via relays connected to the Raspberry Pi’s GPIO.
- Query environmental sensor readings (temperature, humidity) from multiple rooms.
- Retrieve server operational statistics, including network status, system uptime, and CPU temperature.

## 📌 Requirements
> *Note*: Requirements are grouped into three priority tiers from critical [P0] to optional [P2] 
### Functional Requirements
1. [P0] The system shall allow clients to **control GPIOs** using a Unix-style command structure (should follow format: `<target> <action [parameters]`, e.g. `gpio set 10`)
2. [P0] The system shall support **querying the current temperature and humidity** from any currently available environmental sensor using commands that follow the same format;
3. [P0] The server shall **acknowledge valid commands within 1 second** of reception;
4. [P0] The system shall provide **structured error messages** for invalid commands or connection issues;
5. [P0] The system shall allow **querying server status** (including basic network statistics, system uptime, CPU temperature or sensor's status) using commands that follow the defined structure;
6. [P0] The system shall support **graceful client disconnections** via a designated command;
7. [P0] The system shall allow clients to **shut down the server** using a designated command;
8. [P1] The server shall **log all incoming commands** and their execution status, including timestamps;
9. [P1] The system shall allow **commands to be customizable** via a configuration file;
10. [P2] The server shall **validate connected environmental sensors** at startup and notify the client of the system’s state after a successful connection;
11. [P2] The system shall support **automatic disconnection of abandoned clients** via a timeout mechanism;
12. [P2] The server shall be capable of **running as a daemon** on Linux;
13. [P2] The system shall support **secure authentication** using password/login or key exchange;

### Non-Functional Requirements
1. [P0] The system shall handle **up to 5 simultaneous clients** without response time degradation;
2. [P0] The system shall implement a **structured, text-based protocol over TCP**;
2. [P0] The server shall **run reliably on a Raspberry Pi Zero 2W** with Raspberry Pi OS;
3. [P0] The client shall run on **any device supporting Python 3.6+** (Windows, Linux, macOS);
4. [P0] The server shall support **up to 5 environmental sensors via I2C or SPI** (e.g., DHT22, BME280);
5. [P0] The system shall guarantee a **maximum response time of 2 seconds under normal load** conditions;
6. [P0] The server app shall be **modular**, allowing additional GPIO devices or sensors to be integrated with **minimal code changes** - in particular, a virtual sensor interface should be provided allowing integration of new sensor without making any modifications to the rest of the code.
7. [P1] The server shall **log all operations in a standardized format** (e.g., JSON, plain text);
8. [P1] The server shall include **robust error handling** to prevent crashes due to malformed commands;
9. [P2] For improved security, if deployed as a daemon, the server shall **run under a dedicated user account** with limited privileges (e.g. no shell access, "append only" mode on logs)
10. [P2] If implemented as a daemon, an **installation/uninstallation script** shall be provided;

## 📌 PiHub Protocol Definition

> **This section defines the communication protocol used by PiHub, ensuring a structured, Unix-style command interface over TCP.**

### **General Command Structure**

```
<target> <action> [parameters]
```
- `target`: The resource being acted upon (e.g., `gpio`, `sensor`, `server`, `client`).
- `action`: The operation being performed (e.g., `set`, `get`, `status`, `disconnect`, `shutdown`).
- `parameters`: Optional parameters (e.g., GPIO pin number, sensor ID, GPIO status to be set).

**note**: command dispatcher should be case-insensitive and *parameters* should be commutative (i.e. `gpio set 10 on` and `GPIO set ON 10` should produce the same result)

---

### **1️⃣ Device Control (GPIO, Relays, Actuators)**

| Command                   | Description                           | Example           | Priority   |
| ------------------------- | ------------------------------------- | ----------------- | ---------- |
| `gpio set <on\off> <PIN>` | Set GPIO state.                       | `gpio set 10 on`  | P0         |
| `gpio toggle <PIN>`       | Toggle GPIO state.                    | `gpio toggle 5`   | P0         |
| `gpio get [PIN]`          | Get GPIO state (or all if omitted).   | `gpio get 5`      | P0         |
---

### **2️⃣ Sensor Queries**

| Command                       | Description                               | Example              | Priority   |
| ----------------------------- | ----------------------------------------- | -------------------- | ---------- |
| `sensor list`                 | List available sensors and their IDs.     | `sensor list`        | P0         |
| `sensor get <SENSOR_ID> temp` | Get temperature.                          | `sensor get S1 temp` | P0         |
| `sensor get <SENSOR_ID> hum`  | Get humidity.                             | `sensor get S1 hum`  | P0         |
---

### **3️⃣ Server & System Monitoring**

| Command         | Description             | Example         | Priority   |
| --------------- | ----------------------- | --------------- | ---------- |
| `server status` | Get system health.      | `server status` | P0         |
| `server uptime` | Get system uptime.      | `server uptime` | P0         |
| `server temp`   | Get CPU temperature.    | `server temp`   | P0         |
| `server net`    | Get network statistics. | `server net`    | P2         |
---

### **4️⃣ Connection & Client Management**

| Command             | Description           | Example             | Priority   |
| ------------------- | --------------------- | ------------------- | ---------- |
| `client disconnect` | Disconnect client.    | `client disconnect` | P0         |
| `server shutdown`   | Shut down the server. | `server shutdown`   | P0         |
---

### **5️⃣ Configuration & Security**

| Command                    | Description           | Example                      | Priority   |
| -------------------------- | --------------------- | ---------------------------- | ---------- |
| `config reload`            | Reload configuration. | `config reload`              | P2         |
| `auth login <USER> <PASS>` | Authenticate user.    | `auth login admin mypass123` | P2         |
| `auth logout`              | Log out user.         | `auth logout`                | P2         |
---

### **Notes**

- Commands follow a **strict Unix-style format**: `<target> <action> [parameters]`.
- Responses from the server should be **structured and machine-readable**.
- New commands can be added following the same structured format to maintain consistency.
- The system should handle **invalid commands gracefully** and return meaningful error messages.

---

### **Example Command Workflow**

```
# Turn on a smart light connected to GPIO 8
gpio set 8 on  

# Get the current temperature from sensor S1
sensor get S1 temp  

# Get system uptime
server uptime  

# Disconnect from the server
client disconnect  
```

## 📌 Architecture

### Software stack / technologies
| Subsystem | Environment | Language | Build system | Compiler | Test framework | Debugger | Code formatter | Static analysis | Test coverage analysis | Memory analysis |
| -- | -- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
| Server | Linux | C99 or higher | CMake | gcc | CMocka | gdb | Clang-format | Clang-tidy | ? | Valgrind |
| Client | Linux/Windows/Mac | Python | - | - | - | - | - | - | - | - |

### Server (components / src structure)
- main (calls piHub_init(), piHub_start())
- app: **piHub** (Application layer: configures the whole server and external hardware, run main application loop)
- app: **dispatcher** (Interpretter for pre-defined/custom commands: validating and interpretting commands, managing other PiHub protocol functionalities, e.g. encrytpion if used)
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
One for the Server (to monitor incoming connections)
One for each client

### Client
- ...
