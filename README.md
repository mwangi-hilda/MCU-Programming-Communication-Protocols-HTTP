# MCU-Programming-Communication-Protocols-HTTP
This project connects an ATmega2560 microcontroller to a SIM800 GSM/GPRS module to remotely monitor and control home devices (light and curtain) via a PHP/MySQL web interface.

The project uses an **ATmega2560** microcontroller and a **SIM800 GSM/GPRS module** to:
- Periodically fetch device status (light & curtain) from a **PHP/MySQL web server**.
- Control devices based on the fetched status.
- Send **SMS notifications** when the status changes.

The PHP/MySQL backend is hosted (e.g., on InfinityFree/XAMPP) and provides:
- A control panel with buttons (`Light ON/OFF`, `Curtain OPEN/CLOSE`, `View Status`).
- A `status.php` endpoint that returns the current state for remote monitoring.

---

## Features
- **HTTP GET request** to a PHP `status.php` file using SIM800 AT commands.
- **Automatic control** of devices via GPIO pins on the ATmega2560.
- **SMS notifications** on status changes.
- **Timer-based polling** (default: every 2 minutes using Timer1 CTC mode).
- **Interrupt-driven UART** reception for SIM800 responses.

---
