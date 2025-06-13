# Test script for connecting and reading temperature from Sensor #0 connected to PiHub

import socket
import time
import threading
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import re

# PiHub connection configuration
PIHUB_HOST = '192.168.0.102'
PIHUB_PORT = 65002
SENSOR_ID = 0
INTERVAL = 0.1  # seconds

# Shared data for plotting
timestamps = []
temperatures = []

# Lock for thread-safe access to shared data
data_lock = threading.Lock()

def connect_to_pihub():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((PIHUB_HOST, PIHUB_PORT))
    # Read initial welcome messages
    sock.recv(4096)
    return sock

def read_temperature_loop():
    try:
        sock = connect_to_pihub()
        while True:
            cmd = f"sensor get {SENSOR_ID} temp\n"
            sock.sendall(cmd.encode())
            response = sock.recv(1024).decode()

            match = re.search(r'temp:\s*([\d.]+)', response)
            if match:
                temp = float(match.group(1))
                with data_lock:
                    timestamps.append(time.time())
                    temperatures.append(temp)
            else:
                print(f"Warning: Failed to parse temperature from response: {response.strip()}")

            time.sleep(INTERVAL)
    except Exception as e:
        print(f"Error: {e}")

def animate(i):
    with data_lock:
        if not timestamps:
            return
        plt.cla()
        plt.plot([t - timestamps[0] for t in timestamps], temperatures, label='Sensor #0 Temp (°C)')
        plt.xlabel("Time (s)")
        plt.ylabel("Temperature (°C)")
        plt.title("Live Temperature Reading from PiHub Sensor #0")
        plt.legend(loc="upper left")
        plt.tight_layout()

if __name__ == "__main__":
    thread = threading.Thread(target=read_temperature_loop, daemon=True)
    thread.start()

    plt.figure(figsize=(10, 5))
    ani = FuncAnimation(plt.gcf(), animate, interval=500)
    plt.show()
