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

#define APP_PIHUB_INFO_MSG "> "
#define APP_PIHUB_ERROR_MSG "> err: "
#define APP_PIHUB_PROMPT_CHAR "$ "

#define APP_TEMP_MSG_BUF_SIZE 2048 // Size of the generic temp buffer for building message strings
#define APP_DISCONNECT_MSG "one of the clients disconnected from the server" // Msg broadcasted on disconnect
#define APP_CONNECT_MSG " connected to the server" // Msg broadcasted on new connection
#define APP_WELCOME_MSG \
    "Welcome to PiHub — type `server help` for available commands." // Msg sent to all new clients on connection

#define APP_GENERIC_FAILURE_MSG "generic system failure, please try again"
#define APP_CMD_INCOMPLETE_MSG "command incomplete (hint: type `server help` for syntax manual)"
#define APP_CMD_ERR_MSG "command not found (hint: type `server help` for available commands)"

#define APP_HUM_STRING "hum"     // String argument for reading the humidity
#define APP_PRESS_STRING "press" // String argument for reading the pressure
#define APP_TEMP_STRING "temp"   // String argument for reading the temperature

#endif // __CONFIG_H__