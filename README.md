# 📡 PiHub – Smart Home Control Hub

> Smart home server running on Raspberry Pi with modular C backend and Python client.

## 📌 Overview

PiHub is a multithreaded smart home controller for Raspberry Pi, using a command-driven TCP protocol. It allows control over GPIOs, querying environmental sensors, and monitoring system health. Designed to run as a daemon with systemd, it supports modular extensions and includes a Python client for graphing and control.

---

## 🔧 Key Features

- GPIO control and state queries
- I2C/SPI environmental sensor support (BME280, etc.)
- Server status (CPU temp, uptime, network)
- Python client with graphing support
- Daemonized via systemd; unit tested (CMocka)
- Extensible via sensor interface

---

## 📁 Project Structure

```
src/
├── app/         # Main loop, dispatcher, sysstat
├── comm/        # TCP server
├── hw/          # GPIO, I2C, SPI
├── sensors/     # Bme280
├── utils/       # Linked list, logging
tools/           # Python client and tools
tests/           # Unit tests
docs/            # Project documentation
```

---

## 🧠 Protocol

```
<target> <action> [parameters]
```

Examples:
- `gpio set 8 on`
- `sensor get 1 temp`
- `server status`
- `client disconnect`

Features:
- Case-insensitive commands
- Parameter order is flexible

---

## ⚙️ Build & Run

```bash
cmake -B build -DUT=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/tests/test_piHub
cmake --install build
systemctl daemon-reload
systemctl enable pihub
systemctl start pihub
journalctl -u pihub -n 20
```

### Python Client

```bash
python3 ./tools/plot_temperature.py
```

---

## 🧪 Development & Debug

```bash
./build.sh --run
valgrind ./build/src/pihubd
nc 127.0.0.1 65002               # Test socket connection
sudo i2cdetect -y 1              # List I2C devices
gdb ./build/src/pihubd          # Debug
```

---

## 📃 License

MIT License