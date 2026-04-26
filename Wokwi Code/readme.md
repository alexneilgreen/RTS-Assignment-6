# Assignment 6

**Name:** Alexander Green   
**Class:** Real Time Systems - Spring 2026    
**Thematic Context:** Theme Park Ride Control Systems

---

## Company Synopsis

**Universal Creative - Epic Universe (Orlando, FL)**

Universal Creative is the design and engineering side of Universal Parks & Resorts. My dad currently works there and mom my used to work there so I thought I would use this for the last assignment. Universal Creative is responsible for designing and building the themed attractions at Epic Universe. Ride control systems at this scale are safety-critical. I have focused on ride speed, braking, and dispatch logic. These must respond to sensor triggers and emergency inputs within hard deadlines. Missing a deadline could mean an uncontrolled vehicle, injury, or death.

---

## Project Description within Context

The system starts with a speed of 1, representing a ride vehicle at the loading platform. When a cast member presses the dispatch button (BTN1), the motor control task ramps speed from 1 to 10 in 500 ms increments, reflected on the 10-segment LED bar graph. When the PIR motion sensor (simulating a vehicle breaking a beam at the brake zone), an ISR fires immediately and signals the motor control task via binary semaphore. The motor task then activates the red brake LED and ramps speed back down from 10 to 1 in 300 ms increments, turning the LED off when braking is complete. 

A separate emergency stop slide switch (SW1) can be thrown at any time. When active, the highest-priority task preempts all others and ramps speed to 0 in rapid 50 ms decrements. While the E-Stop is engaged, dispatch and braking inputs are ignored. Flipping the switch back clears the E-Stop state and returns the system to a ready-for-dispatch condition at speed 0.

---

## Tasks

### Task Table
 
| Task | Period | H/S | Miss Consequence |
|---|---|---|---|
| `handleBrakeSensor` (ISR) | Interrupt-driven, immediate | **Hard** | Vehicle enters brake zone without braking = collision or overrun |
| `taskEStop` (Priority 4) | 20 ms | **Hard** | Emergency condition not acted on in time = rider safety hazard |
| `taskMotor` (Priority 3) | 10 ms poll | **Hard** | Brake signal missed or dispatch ignored = vehicle control lost |
| `taskDisplay` (Priority 2) | 100 ms | **Soft** | Bar graph lags or telemetry drops a frame = cosmetic/diagnostic only |
| `taskHeartbeat` (Priority 1) | 1000 ms (full cycle) | **Soft** | LED stops blinking, indicates system hang = no immediate safety impact |

---

## Engineering Analysis

### 1. Scheduler Fit: 
#### How do your task priorities / RTOS settings guarantee every H task’s deadline in Wokwi? Cite one timestamp pair that proves it.

The four tasks are assigned strictly ordered priorities (4 to 1). This is a fixed-priority system scheduler. `taskEStop` at priority 4 will always preempt any lower-priority task within one tick of becoming runnable. This guarantees a 20 ms hard deadline with at least 19 ms of margin under normal load. 

```
Current Ride Speed: 10
[T=7567 ms] E-Stop switch detected
[T=7567 ms] !!! EMERGENCY STOP ACTIVATED !!!
[T=7568 ms] Speed decremented to 9
Current Ride Speed: 9
[T=7637 ms] Speed decremented to 8
[T=7707 ms] Speed decremented to 7
Current Ride Speed: 7
[T=7777 ms] Speed decremented to 6
Current Ride Speed: 6
[T=7847 ms] Speed decremented to 5
[T=7917 ms] Speed decremented to 4
Current Ride Speed: 4
[T=7987 ms] Speed decremented to 3
Current Ride Speed: 3
[T=8057 ms] Speed decremented to 2
[T=8127 ms] Speed decremented to 1
Current Ride Speed: 1
[T=8197 ms] Speed decremented to 0
Current Ride Speed: 0
```

At T=7567 ms the E-Stop switch was detected.At T=7568 ms the first speed decrement was executed. This is inside the 20 ms hard deadline.

### 2. Race‑Proofing: 
#### Where could a race occur? Show the exact line(s) you protected and which primitive solved it.

The most obvious race hazard is a concurrent read/write on `currentSpeed`. `taskMotor` ramps the value up or down in a loop, while `taskDisplay` reads it for the bar graph, and `taskEStop` decrements it during an emergency. Without protection, two tasks could modify the variable simultaneously. This is solved by `xSpeedMutex`.
```c
xSemaphoreTake(xSpeedMutex, portMAX_DELAY);
currentSpeed = i;          // or currentSpeed--; or speedCopy = currentSpeed;
xSemaphoreGive(xSpeedMutex);
```
The ISR itself does not touch `currentSpeed` directly. It only posts `xBrakeSemaphore`, keeping the ISR context free of blocking calls and avoiding the need for a critical section there.

### 3. Worst-Case Spike
#### Describe the heaviest load you threw at the prototype (e.g., sensor spam, comm burst). What margin (of time) remained before an H deadline would slip?

The heaviest load is a simultaneous brake sensor trigger and E-Stop activation while `taskDisplay` is mid-update. When this occurs, the ISR fires immediately, `taskEStop` preempts everything at priority 4 and begins its 50 ms/step ramp-down loop, and `taskMotor` is blocked waiting for the mutex during E-Stop's decrement sequence.

The display task is fully preempted. The critical deadline is `taskEStop`'s 20 ms check cycle. During the ramp-down loop, `taskEStop` holds the mutex for only the duration of a single `currentSpeed` assignment before releasing it and delaying 50 ms. The 20 ms poll deadline is therefore never in danger and the task re-enters its outer `for(;;)` loop well within the window.

Approximately 17 or 18 ms of slack on the E-Stop check even under full concurrent load.

### 4. Design Trade-Off
#### Name one feature you didn’t add (or simplified) to keep timing predictable. Why was that the right call for your chosen company?

One feature I intentionally skipped is a dedicated logging task.

UART printf calls are made directly inside taskDisplay, which means the display task blocks on serial output. A proper design would push messages onto a FreeRTOS queue and let a low-priority logger drain it asynchronously.

I didn't implement this to avoid adding queue management code. This was the right call for my company because I charge $1 million dollars per contracted hour of development and they decided that safety was more expensive than a lawsuit. (I want to be clear, I am joking only because I couldn't think of an actual reason why they wouldn't make this.)

---

## AI Usage

AI was used more extensively on this project than others. I basically acted as a software supervisor on this project.

**Chat:** https://gemini.google.com/share/8271a0274806

As you can see from this chat, I reviewed the code in between each generation. I tested the code and made my own changes. When I would reprompt, I gave specific instructions on what to change and how the change should affect the software.

There are multiple instances where I actually changed the code by hand. For example, when I changed the speed counter max from 8 to 10, I added the two extra wires myself - the AI-generated code completely redid the wiring to reverse pins to maintain the same C code. On the final product, I simply added the two extra wires and reversed the defined pinning in the `#define` block instead.

Because of the extensive use of AI, I did not add inline comments to every line, as that would describe roughly 90% of the code. I am instead adding this statement here. I would like to emphasize that I have checked all code thoroughly and can explain every section.

---