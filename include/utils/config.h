#ifndef __CONFIG_H__
#define __CONFIG_H__

// Board config
#define PIHUB_I2C_ADAPTER 1        // On RPI the I2C adapter is mounted as '/dev/i2c-1'
#define NET_INTERFACE_NAME "wlan0" // Name of the network interface
#define BME280_COUNT 1             // Number of connected BME280 sensors

// Board-independent PiHub config
#define APP_SERVER_PORT "65002" // Port number (as a string) under which the PiHub server should be accessible
#define APP_SERVER_MAX_CLIENTS 5          // Maximum number of clients connected at the same time
#define APP_SERVER_MAX_CONN_REQUESTS 10   // Maximum number of pending connection reuqests
#define APP_SERVER_RECV_DATA_BUF_SIZE 128 // Size of the buffer for new data from the clients

#define APP_DISPATCHER_DELIM " " // Delimiter in commands handled by the dispatcher

#define APP_PIHUB_INFO_MSG "[PiHub] info: "
#define APP_PIHUB_ERROR_MSG "[PiHub] error: "
#define APP_PIHUB_PROMPT_CHAR "> "

#define APP_TEMP_MSG_BUF_SIZE 1024   // Size of the generic temp buffer for building message strings
#define APP_GENERIC_MSG_MAX_LEN 1024 // Maximum length of macro-defined string messages
#define APP_DISCONNECT_MSG "one of the clients disconnected" // Msg broadcasted on disconnect
#define APP_CONNECT_MSG_BUF_SIZE 128                         // New connection buffer size
#define APP_CONNECT_MSG " connected to the server"           // Msg broadcasted on new connection
#define APP_WELCOME_MSG "Welcome to PiHub!"                  // Msg sent to all new clients on connection

#define APP_GENERIC_FAILURE_MSG "generic system failure"
#define APP_CMD_INCOMPLETE_MSG "command incomplete"
#define APP_CMD_ERR_MSG "command not found (incorrect cmd / buffer empty or overflow / token too long)"

#define APP_HUM_STRING "hum"     // String argument for reading the humidity
#define APP_PRESS_STRING "press" // String argument for reading the pressure
#define APP_TEMP_STRING "temp"   // String argument for reading the temperature


// Array with the help/man message (divided into lines)
const char* APP_HELP_MSG[] = {
    "PIHUB(1)                      User Commands                     PIHUB(1)",
    "",
    "NAME",
    "    pihub - Smart Home Control Hub command interface",
    "",
    "SYNOPSIS",
    "    <target> <action> [parameters]",
    "",
    "DESCRIPTION",
    "    A structured, Unix-style TCP command interface to control GPIOs,",
    "    read sensors, and query Raspberry Pi system status.",
    "",
    "COMMANDS",
    "  GPIO Commands:",
    "    gpio set <PIN> <state>        Set GPIO pin state [0/1]",
    "    gpio get <PIN>                Get GPIO pin state",
    "",
    "  Sensor Commands:",
    "    sensor list                   List available sensors",
    "    sensor get <ID> temp          Get temperature in [*C]",
    "    sensor get <ID> hum           Get relative humidity [%]",
    "    sensor get <ID> press         Get pressure [Pa]",
    "",
    "  Server Commands:",
    "    server help                   Display this man page",
    "    server status                 Show system health info",
    "    server uptime                 Show server's uptime",
    "    server net                    Show network stats",
    "    server disconnect             Disconnect this client",
    "",
    "EXAMPLES",
    "    gpio set 10 on               Turn on relay at GPIO 10",
    "    sensor get S1 temp           Get temperature from sensor S1",
    "    server uptime                Check how long the Pi has been running",
};

#endif // __CONFIG_H__