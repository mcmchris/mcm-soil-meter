/**
 * @file main.h
 * @brief Includes, definitions and global declarations for SoilMeter code
 * 
 */
#include <Arduino.h>
#include <SPI.h>

// Comment the next line if you want DEBUG output. But the power savings are not as good then!!!!!!!
//#define MAX_SAVE

/* Time the device is sleeping in milliseconds = 1 minutes * 60 seconds * 1000 milliseconds */
#define SLEEP_TIME 60 * 60 * 1000 //1 minuto para esta prueba

extern SemaphoreHandle_t loraEvent;

// Main loop stuff
void periodicWakeup(TimerHandle_t unused);
extern SemaphoreHandle_t taskEvent;
extern uint8_t eventType;
extern SoftwareTimer taskWakeupTimer;
