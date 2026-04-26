# Assignment 6

**Name:** Alexander Green  
**Class:** Real Time Systems - Spring 2026  
**Thematic Context:** Theme Park Ride Control Systems

![C](https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white)
![ESP32](https://img.shields.io/badge/ESP32-E7352C?style=flat&logo=espressif&logoColor=white)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-27A062?style=flat&logo=freertos&logoColor=white)
![Build](https://img.shields.io/badge/Build-passing-brightgreen?style=flat)

---

## Project Description within Context

The system starts with a speed of 1, representing a ride vehicle at the loading platform. When a cast member presses the dispatch button (BTN1), the motor control task ramps speed from 1 to 10 in 500 ms increments, reflected on the 10-segment LED bar graph. When the PIR motion sensor (simulating a vehicle breaking a beam at the brake zone), an ISR fires immediately and signals the motor control task via binary semaphore. The motor task then activates the red brake LED and ramps speed back down from 10 to 1 in 300 ms increments, turning the LED off when braking is complete.

A separate emergency stop slide switch (SW1) can be thrown at any time. When active, the highest-priority task preempts all others and ramps speed to 0 in rapid 50 ms decrements. While the E-Stop is engaged, dispatch and braking inputs are ignored. Flipping the switch back clears the E-Stop state and returns the system to a ready-for-dispatch condition at speed 0.

---

## Demonstration Video

<iframe width="560" height="315" src="https://www.youtube.com/embed/huG63geHwBI?si=vOq56vD5rQy_1U9a" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

---

## Wokwi

**Wokwi Simulation Link:** [https://wokwi.com/projects/462233705096228865](https://wokwi.com/projects/462233705096228865)

### Tasks

| Task                         | Period                      | H/S      | Miss Consequence                                                       |
| :--------------------------- | :-------------------------- | :------- | :--------------------------------------------------------------------- |
| `handleBrakeSensor` (ISR)    | Interrupt-driven, immediate | **Hard** | Vehicle enters brake zone without braking = collision or overrun       |
| `taskEStop` (Priority 4)     | 20 ms                       | **Hard** | Emergency condition not acted on in time = rider safety hazard         |
| `taskMotor` (Priority 3)     | 10 ms poll                  | **Hard** | Brake signal missed or dispatch ignored = vehicle control lost         |
| `taskDisplay` (Priority 2)   | 100 ms                      | **Soft** | Bar graph lags or telemetry drops a frame = cosmetic/diagnostic only   |
| `taskHeartbeat` (Priority 1) | 1000 ms (full cycle)        | **Soft** | LED stops blinking, indicates system hang = no immediate safety impact |

### Hardware Configuration

| Hardware Component             | Code ID            | Pinning                                |
| :----------------------------- | :----------------- | :------------------------------------- |
| Pushbutton (Dispatch)          | `BTN_DISPATCH`     | 4                                      |
| PIR Motion Sensor (Brake Zone) | `SENSOR_BRAKE`     | 18                                     |
| Slide Switch (E-Stop)          | `SW_ESTOP`         | 5                                      |
| LED - Red (Brake Active)       | `LED_BRAKE_ACTIVE` | 2                                      |
| LED - Lime Green (Heartbeat)   | `LED_HEARTBEAT`    | 15                                     |
| 10-Segment LED Bar Graph       | `BAR_PINS`         | 23, 22, 32, 33, 25, 26, 27, 14, 13, 12 |

_Note: The `BAR_PINS` array order is determined by wiring._
