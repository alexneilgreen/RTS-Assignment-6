/* --------------------------------------------------------------
    Alexander Green
    Application: 06 - Rev1
    Release Type: Baseline Multitask Skeleton Starter Code 
    Class: Real Time Systems - Sp 2026
    AI Use: Commented inline

    Epic Universe - Ride Braking Proof of Concept
    Company: Universal Creative (Epic Universe)
    Developed with assistance from Gemini AI
---------------------------------------------------------------*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_timer.h"

// --- Pin Definitions ---
#define BTN_DISPATCH     4
#define SENSOR_BRAKE     18
#define SW_ESTOP         5
#define LED_BRAKE_ACTIVE 2
#define LED_HEARTBEAT    15

// Updated to 10 pins
const int BAR_PINS[] = {23, 22, 32, 33, 25, 26, 27, 14, 13, 12}; 

// --- Global Shared Variables (Protected) ---
volatile int currentSpeed = 1; 
volatile bool eStopActive = false;

// --- Synchronization Primitives ---
SemaphoreHandle_t xSpeedMutex;
SemaphoreHandle_t xBrakeSemaphore;

// --- ISR: Brake Sensor ---
static void IRAM_ATTR handleBrakeSensor(void* arg) {
    // Hard Real-Time: Signal the motor task immediately
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xBrakeSemaphore, &xHigherPriorityTaskWoken);
    
    // Force a context switch if a higher priority task was woken by the semaphore
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// --- Task: Emergency Monitor (Highest Priority) ---
void taskEStop(void *pvParameters) {
    bool eStopMessagePrinted = false; // Tracks if we've already printed the warning
    
    for (;;) {
        if (gpio_get_level(SW_ESTOP) == 1) {
          eStopActive = true;

          if (!eStopMessagePrinted) {
              int64_t t = esp_timer_get_time() / 1000; // convert to ms
              printf("[T=%lld ms] E-Stop switch detected\n", t);
              printf("[T=%lld ms] !!! EMERGENCY STOP ACTIVATED !!!\n", t);
              eStopMessagePrinted = true;
          }
            
          // Rapid ramp down logic
          while (currentSpeed > 0) {
            xSemaphoreTake(xSpeedMutex, portMAX_DELAY);
            currentSpeed--;
            xSemaphoreGive(xSpeedMutex);
            int64_t t2 = esp_timer_get_time() / 1000;
            printf("[T=%lld ms] Speed decremented to %d\n", t2, currentSpeed);
            vTaskDelay(pdMS_TO_TICKS(50));
            break;
          }
        } else {
          eStopActive = false;
          if (eStopMessagePrinted) {
              printf("Emergency Stop Cleared. Ready for dispatch.\n");
              eStopMessagePrinted = false;
          }
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // Hard Deadline: 20ms check
    }
}

// --- Task: Motor Control (Hard Real-Time) ---
void taskMotor(void *pvParameters) {
    for (;;) {
        // 1. Check for Dispatch (Now allows dispatch from Speed 0 or 1)
        if (gpio_get_level(BTN_DISPATCH) == 1 && (currentSpeed == 0 || currentSpeed == 1) && !eStopActive) {
            
            // If starting from 0 (after E-Stop), start our loop at 1. Otherwise start at 2.
            int startingSpeed = (currentSpeed == 0) ? 1 : 2;
            
            for (int i = startingSpeed; i <= 10; i++) { // Ramps to 10
                if (eStopActive) break;
                xSemaphoreTake(xSpeedMutex, portMAX_DELAY);
                currentSpeed = i;
                xSemaphoreGive(xSpeedMutex);

                // printf("[T=%lld ms] Motor ramping UP to %d\n", esp_timer_get_time() / 1000, i);

                vTaskDelay(pdMS_TO_TICKS(500)); // Smooth ramp up
            }
        }

        // 2. Check for Braking Sensor Signal
        if (xSemaphoreTake(xBrakeSemaphore, 0) == pdTRUE && !eStopActive) {
            gpio_set_level(LED_BRAKE_ACTIVE, 1);

            // printf("[T=%lld ms] Brake triggered! Ramping down.\n", esp_timer_get_time() / 1000);

            for (int i = 10; i >= 1; i--) { // Ramps down from 10
                if (eStopActive) break;
                xSemaphoreTake(xSpeedMutex, portMAX_DELAY);
                currentSpeed = i;
                xSemaphoreGive(xSpeedMutex);
                vTaskDelay(pdMS_TO_TICKS(300)); // Smooth ramp down
            }
            gpio_set_level(LED_BRAKE_ACTIVE, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// --- Task: Visual Feedback (Soft Real-Time) ---
void taskDisplay(void *pvParameters) {
    for (;;) {
        int speedCopy;
        xSemaphoreTake(xSpeedMutex, portMAX_DELAY);
        speedCopy = currentSpeed;
        xSemaphoreGive(xSpeedMutex);

        // Update Bar Graph (Loops through all 10)
        for (int i = 0; i < 10; i++) {
            gpio_set_level(BAR_PINS[i], (i < speedCopy) ? 1 : 0);
        }

        // Outward Link: UART Serial Terminal
        printf("Current Ride Speed: %d\n", speedCopy);

        vTaskDelay(pdMS_TO_TICKS(100)); // Soft Deadline: 100ms
    }
}

// --- Task: System Heartbeat (Soft Real-Time) ---
void taskHeartbeat(void *pvParameters) {
    int led_state = 0;
    for (;;) {
        led_state = !led_state; // Toggle state
        gpio_set_level(LED_HEARTBEAT, led_state);
        vTaskDelay(pdMS_TO_TICKS(500)); // Blinks every half second
    }
}

// --- Main Application Entry Point ---
void app_main(void) {
    printf("Starting Epic Universe Ride Control System...\n");

    // 1. Initialize GPIO Pins
    for (int i = 0; i < 10; i++) { // Initialize all 10 pins
        gpio_reset_pin(BAR_PINS[i]);
        gpio_set_direction(BAR_PINS[i], GPIO_MODE_OUTPUT);
    }
    
    gpio_reset_pin(LED_BRAKE_ACTIVE);
    gpio_set_direction(LED_BRAKE_ACTIVE, GPIO_MODE_OUTPUT);
    
    gpio_reset_pin(LED_HEARTBEAT);
    gpio_set_direction(LED_HEARTBEAT, GPIO_MODE_OUTPUT);

    gpio_reset_pin(BTN_DISPATCH);
    gpio_set_direction(BTN_DISPATCH, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_DISPATCH, GPIO_PULLDOWN_ONLY);

    gpio_reset_pin(SW_ESTOP);
    gpio_set_direction(SW_ESTOP, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SW_ESTOP, GPIO_PULLDOWN_ONLY);

    gpio_reset_pin(SENSOR_BRAKE);
    gpio_set_direction(SENSOR_BRAKE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SENSOR_BRAKE, GPIO_PULLUP_ONLY);

    // 2. Initialize Synchronization Primitives
    xSpeedMutex = xSemaphoreCreateMutex();
    xBrakeSemaphore = xSemaphoreCreateBinary();

    // 3. Configure and Attach Interrupt (ISR)
    gpio_install_isr_service(0);
    gpio_set_intr_type(SENSOR_BRAKE, GPIO_INTR_POSEDGE); 
    gpio_isr_handler_add(SENSOR_BRAKE, handleBrakeSensor, NULL);

    // 4. Create Tasks with Priorities
    xTaskCreate(taskEStop, "E-Stop", 2048, NULL, 4, NULL);     
    xTaskCreate(taskMotor, "MotorCtrl", 2048, NULL, 3, NULL);
    xTaskCreate(taskDisplay, "Display", 2048, NULL, 2, NULL);  
    xTaskCreate(taskHeartbeat, "Heartbeat", 1024, NULL, 1, NULL); 
}