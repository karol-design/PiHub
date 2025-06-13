import socket
import threading
import time
import random

# List of test commands to send
COMMANDS = [
    "server uptime",
    "gpio set 0 1",
    "server net",
    "sensor list",
    "server help",
    "gpio get 1",
    "sensor read 0",
]

# Number of threads (connections)
NUM_THREADS = 25

# Number of commands per connection
COMMANDS_PER_THREAD_MIN = 100
COMMANDS_PER_THREAD_MAX = 200

# Target PiHub server
HOST = '127.0.0.1'
PORT = 65002

def flood_connection(thread_id):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((HOST, PORT))
            print(f"[Thread-{thread_id}] Connected to server.")
            time.sleep(0.5)  # Let the welcome message come in

            # Receive welcome message
            try:
                welcome = sock.recv(1024).decode()
                print(f"[Thread-{thread_id}] Server says:\n{welcome.strip()}")
            except socket.timeout:
                pass

            for i in range(random.randint(COMMANDS_PER_THREAD_MIN,COMMANDS_PER_THREAD_MAX)):
                cmd = random.choice(COMMANDS)
                sock.sendall((cmd + "\n").encode())
                try:
                    response = sock.recv(2048).decode()
                    print(f"[Thread-{thread_id}] Sent: {cmd} | Response: {response.strip()}")
                except socket.timeout:
                    print(f"[Thread-{thread_id}] Timeout waiting for response.")
                time.sleep(0.05)  # Small delay to simulate human interaction
    except Exception as e:
        print(f"[Thread-{thread_id}] Error: {e}")

# Start all threads
threads = []
for i in range(NUM_THREADS):
    t = threading.Thread(target=flood_connection, args=(i,))
    t.start()
    threads.append(t)

# Wait for all threads to finish
for t in threads:
    t.join()

print("All threads finished.")
