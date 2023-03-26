/*
   MCM Soil Meter firmware

   @Author: Christopher Mendez | MCMCHRIS
   @Date: 03/25/2023 (mm/dd/yy)
   @Brief:
   This firmware runs on a nRF52840 (RAK4631) and using a Soil Sensor, measures the Soil humidity and temperature,
   send the data through a cellular link using a Notecard from Blues Wireless and is forwarded to Ubidots.

*/

#include "main.h"
#include <Notecard.h>
#include <Wire.h>
#include "RAK12035_SoilMoisture.h"

/** Semaphore used by events to wake up loop task */
SemaphoreHandle_t taskEvent = NULL;

/** Timer to wakeup task frequently and send message */
SoftwareTimer taskWakeupTimer;

// This is the unique Product Identifier for your device

#define PRODUCT_UID "com.hotmail.mcmchris:mcm_soil_meter"  // "com.my-company.my-name:my-project"

#define myProductID PRODUCT_UID

Notecard notecard;
/** Sensor */
RAK12035 sensor;

#define usbSerial Serial

/** Set these two values after the sensor calibration was done */
uint16_t zero_val = 555;
uint16_t hundred_val = 381;

/**
   @brief Flag for the event type
   -1 => no event
   1 => Timer wakeup
   ...
*/
uint8_t eventType = -1;

/**
   @brief Timer event that wakes up the loop task frequently

   @param unused
*/
void periodicWakeup(TimerHandle_t unused) {

  eventType = 1;
  // Give the semaphore, so the loop task will wake up
  xSemaphoreGiveFromISR(taskEvent, pdFALSE);
}

/**
   @brief Arduino setup function. Called once after power-up or reset

*/
void setup(void) {

  // Initialize the built in LED
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);

#ifdef usbSerial
  delay(2500);
  usbSerial.begin(115200);
  notecard.setDebugOutputStream(usbSerial);
#endif

  time_t serial_timeout = millis();
  // On nRF52840 the USB serial is not available immediately
  while (!usbSerial)
  {
    if ((millis() - serial_timeout) < 5000)
    {
      delay(100);
      digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
    }
    else
    {
      break;
    }
  }

  Wire.begin();

  // Initialize sensor
  sensor.begin();
  notecard.begin();

  // Get sensor firmware version
  uint8_t data = 0;
  sensor.get_sensor_version(&data);
  usbSerial.print("Sensor Firmware version: ");
  usbSerial.println(data, HEX);
  usbSerial.println();

  // Set the calibration values
  // Reading the saved calibration values from the sensor.
  sensor.get_dry_cal(&zero_val);
  sensor.get_wet_cal(&hundred_val);
  usbSerial.printf("Dry calibration value is %d\n", zero_val);
  usbSerial.printf("Wet calibration value is %d\n", hundred_val);

  J *req = notecard.newRequest("hub.set");
  if (myProductID[0]) {
    JAddStringToObject(req, "product", myProductID);
  }

  JAddStringToObject(req, "mode", "continuous");

  notecard.sendRequest(req);

  // Create the LoRaWan event semaphore
  taskEvent = xSemaphoreCreateBinary();
  // Initialize semaphore
  xSemaphoreGive(taskEvent);

  // Take the semaphore so the loop will go to sleep until an event happens
  xSemaphoreTake(taskEvent, 10);
  taskWakeupTimer.begin(SLEEP_TIME, periodicWakeup);
  taskWakeupTimer.start();
}

/**
   @brief Arduino loop task. Called in a loop from the FreeRTOS task handler

*/
void loop(void) {

  // Sleep until we are woken up by an event
  if (xSemaphoreTake(taskEvent, portMAX_DELAY) == pdTRUE) {

    // Check the wake up reason
    switch (eventType) {

      case 1:  // Wakeup reason is timer
        sending();
        break;

      default:
        break;
    }

    // Go back to sleep
    xSemaphoreTake(taskEvent, 10);
  }
}

void sending() {
  uint8_t moisture = 0;
  sensor.get_sensor_moisture(&moisture);
  usbSerial.print(moisture);
  usbSerial.println(" %");

  uint16_t temperature = 0;
  sensor.get_sensor_temperature(&temperature);
  temperature = temperature;
  usbSerial.print(temperature);
  usbSerial.println(" ºC");

  J *req = notecard.newRequest("note.add");
  if (req != NULL) {
    JAddStringToObject(req, "file", "sensors.qo");
    JAddBoolToObject(req, "sync", true);

    J *body = JCreateObject();
    if (body != NULL) {
      JAddNumberToObject(body, "temp", temperature);
      JAddNumberToObject(body, "moisture", moisture);
      JAddItemToObject(req, "body", body);
    }

    notecard.sendRequest(req);
  }
}
