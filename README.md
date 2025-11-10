# Smart Bridge Control System

## Overview
This project is a **Smart Bridge Control System**, developed as part of a university systems engineering assignment. The goal is to design, prototype, and program an automated bridge capable of managing both marine and road traffic through integrated sensors, actuators, and microcontroller-based control logic.

The system uses **ultrasonic sensors, infrared sensors, DC motors, servos, and LEDs** to simulate a functional drawbridge. It detects approaching vehicles or boats, raises or lowers accordingly, and communicates system status through a Wi-Fi-enabled web interface.

---

## Features
- **Ultrasonic detection system** for marine vessel detection and bridge activation  
- **Light sensor array** for vehicle detection and traffic control  
- **DC motor and servo mechanisms** for bridge movement simulation  
- **LED traffic light system** for vehicle and marine traffic signalling  
- **ESP32 microcontroller with Wi-Fi** for remote monitoring and control  
- **Web-based UI** for real-time status display and manual override  
- **Modular code structure** separating front-end, back-end, and hardware logic  

---

## System Architecture
The system operates across three primary layers:

1. **Hardware Layer**  
   - Ultrasonic and Light sensors, servos, DC motors, and LEDs connected to the ESP32.
2. **Software Layer**  
   - Embedded C/C++ code (via Arduino IDE) manages control logic and communication.
3. **User Interface Layer**  
   - Web-based dashboard (HTML, JavaScript) provides live monitoring and manual control.

---

## Team Contributions

| Team Member | Key Contributions |
|--------------|------------------|
| **Trystan** | Developed ultrasonic and DC motor control code, integrated bridge logic, coordinated software-hardware testing, and aligned front-end and back-end design. |
| **Tom** | Designed traffic system architecture, implemented IR sensor code, and collabarated with the structures team for mechanical integration. |
| **Walter** | Updated and refined front-end UI, assisted in IR sensor and traffic system code development. |
| **Agrim** | Implemented Wi-Fi communication and live bridge status updates within the web interface. |
| **Kyle** | Designed and assembled electrical components, LED system, and wiring layout; supported testing of IR sensors. |
| **Nick** | Led hardware prototyping, assembled components, and tested servo and sensor integration. |

---

## Testing & Validation
- **Ultrasonic Sensor:** Calibrated for accurate distance detection to trigger bridge lift sequence.  
- **Infrared Sensors:** Tested for reliable vehicle detection and response.  
- **Motor & Servo Systems:** Verified stable movement and synchronization.  
- **UI Integration:** Confirmed live data feedback and control signal consistency.  
- **Full System Test:** Successfully integrated hardware and software into a working prototype.

---

## Technologies Used
- **Microcontroller:** ESP32  
- **Languages:** C++, Arduino, JavaScript, HTML  
- **Tools:** Arduino IDE, Visual Studio Code, GitHub, Platofrm IO (VS Code extention) 
- **Hardware:** Ultrasonic sensors, Light sensors, DC motors, servos, LEDs, MOSFETs, 

---

## Demonstration Video

This is a basic Demonstration Video to show what our bridge was capable of:

https://drive.google.com/file/d/1cECo7knFVezSamyvi6PMrSR8C2qPfrnr/view


## License
This project is provided for **educational and demonstration purposes only**.  
Not intended for commercial use or redistribution.  

© 2025 - Trystan Frederick, Walter Chibowora, Agrim, Kyle Dufficy, Nick Dufficy, Thomas Nikolovski