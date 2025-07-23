![Laser Organizer](https://raw.githubusercontent.com/JonahPi/LaserOrganizer_Make04-25/main/pics/IMG_9098.JPG)

# 🔧 Laser Organizer for ESP32 – Smarte Werkstattboxen Add-on

The **Laser Organizer** is an extension to the [Smarte Werkstattboxen project](https://www.heise.de/select/make/2023/5/2317908275419508878) published in the German *Make Magazin* (Issue 5/2023).

It enhances the functionality of the original system by adding laser-controlled automation using a compact, modular hardware platform.

You'll find a very brief description in the project [Wiki](https://github.com/JonahPi/LaserOrganizer_ESP32-code/wiki), a more comprehensive project report has been published in the [German Make-Magazin (4/2025)](https://www.heise.de/select/make/2025/4/2514113463070489857).

The STL-Files to print the Pan/Tilt construction are saved in the CAD-Folder.

---

## 💡 Features

- ✅ Based on the **DLC32 V2.1** platform
- 🔌 Uses the **ESP32** microcontroller
- 🧠 Built-in **Shift Register** for extended I/O control
- ⚙️ Supports **Stepper Motor Drivers** via dedicated slots
- 🛠️ Designed for DIY makers and workshop enthusiasts

---

## 🛠️ Hardware Requirements

- [DLC32 V2.1 Board](https://github.com/makerbase-mks/MKS-DLC32)
- Stepper Motor Drivers (e.g. TMC2209)
- Power supply & connectors

---

## 🚀 Getting Started

1. **Clone this repository:**

   ```bash
   git clone https://github.com/JonahPi/LaserOrganizer_ESP32-code

2. **Add your passwords:**

   Open secrets.h and enter your Wifi and Adafruit passwords

3. **Flash Firmware:**

   Open the project folder in PlatformIO (Add on the Visual Studio) and upload to your DSC32

4. **Check output:**

   Use the PlatformIO monitor to check if the ESP gets a connection to your Wifi and to the Adafruit MQTT Broker
