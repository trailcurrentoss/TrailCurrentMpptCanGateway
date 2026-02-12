#include <Arduino.h>
#include "globals.h"
#include "canHelper.h"

#define CAN_SEND_MESSAGE_SOLAR01_IDENTIFIER 0x2C;
#define CAN_SEND_MESSAGE_SOLAR02_IDENTIFIER 0x2D;
volatile int panelVoltageWholeNumber;
volatile int panelVotlageDecimal;
volatile byte solarWattageMsb;
volatile byte solarWattageLsb;
volatile int batteryVoltageWholeNumber;
volatile int batteryVotlageDecimal;
volatile int isPanelCurrentNegative;
volatile int panelCurrentWholeNumber;
volatile int panelCurrentDecimal;
volatile int solarStatus;

unsigned long canStartMillis;
unsigned long canCurrentMillis;
const unsigned long canStatusPeriod = 33;

void setup()
{
  Serial.begin(115200); // Used for debugging and logging
  while (!Serial)
  {
    ;
    ;
  }
  Serial2.begin(19200); // RX on pin 16, TX on pin 17
  canHelper::canSetup();
}

static void send_mppt_message()
{
  // Configure message to transmit
  twai_message_t message;
  message.identifier = CAN_SEND_MESSAGE_SOLAR01_IDENTIFIER;
  message.extd = false;
  message.rtr = false;
  message.data_length_code = 7;
  message.data[0] = panelVoltageWholeNumber;
  message.data[1] = panelVotlageDecimal;
  message.data[2] = solarWattageMsb;
  message.data[3] = solarWattageLsb;
  message.data[4] = batteryVoltageWholeNumber;
  message.data[5] = batteryVotlageDecimal;
  message.data[6] = solarStatus;
  // Queue message for transmission
  if (twai_transmit(&message, pdMS_TO_TICKS(10)) == ESP_OK)
  {
    debugln("Message queued for transmission");
  }
  else
  {
    debugln("Failed to queue message for transmission");
  }
  return;
}

static void send_mppt_message2()
{
  // Configure message to transmit
  twai_message_t message;
  message.identifier = CAN_SEND_MESSAGE_SOLAR02_IDENTIFIER;
  message.extd = false;
  message.rtr = false;
  message.data_length_code = 3;
  message.data[0] = isPanelCurrentNegative;
  message.data[1] = panelCurrentWholeNumber;
  message.data[2] = panelCurrentDecimal;
  // Queue message for transmission
  if (twai_transmit(&message, pdMS_TO_TICKS(10)) == ESP_OK)
  {
    debugln("Message queued for transmission");
  }
  else
  {
    debugln("Failed to queue message for transmission");
  }
  return;
}

void loop()
{
  canCurrentMillis = millis();
  if (canCurrentMillis - canStartMillis >= canStatusPeriod)
  {
    Serial.println("Running");
    while (Serial2.available())
    {
      String input = Serial2.readStringUntil('\n');
      Serial.println(input);
      // Split the input string based on the delimiter "]t"
      int startIndex = 0;
      int delimiterIndex = 0;

      while ((delimiterIndex = input.indexOf("\t", startIndex)) != -1)
      {
        // Extract the key (substring from startIndex to delimiterIndex)
        String key = input.substring(startIndex, delimiterIndex);
        startIndex = delimiterIndex + 1; // Move past the "]t" delimiter

        // Extract the value (substring from startIndex to the next delimiter or end of string)
        int nextDelimiterIndex = input.indexOf("\t", startIndex);
        String value;
        if (nextDelimiterIndex != -1)
        {
          value = input.substring(startIndex, nextDelimiterIndex);
          startIndex = nextDelimiterIndex + 2; // Move past the "]t" delimiter
        }
        else
        {
          value = input.substring(startIndex); // Handle last key-value pair (no delimiter after value)
        }
        // Serial.print("Key ");
        // Serial.println(key);
        // Serial.print("Value: ");
        // Serial.println(value);
        if (key == "V")
        {
          float voltageValue = (value.toInt() * 0.001);
          batteryVoltageWholeNumber = (int)voltageValue;                                   // First we get the whole number voltage.
          batteryVotlageDecimal = (int)((voltageValue - batteryVoltageWholeNumber) * 100); // For 2 decimal places          
        }
        else if (key == "VPV")
        {
          float panelVoltageValue = (value.toInt() * 0.001);
          panelVoltageWholeNumber = (int)panelVoltageValue;                                 // First we get the whole number voltage.
          panelVotlageDecimal = (int)((panelVoltageValue - panelVoltageWholeNumber) * 100); // For 2 decimal places
        }
        else if (key == "PPV") // This is the panel power
        {
          int solarWattage = value.toInt();
          // Break the absolute value into 2 bytes
          byte lowByte = solarWattage & 0xFF;         // Least significant byte
          byte highByte = (solarWattage >> 8) & 0xFF; // Most significant byte          
          solarWattageMsb = highByte;
          solarWattageLsb = lowByte;
        }
        else if (key == "I")
        {
          float panelCurrentValue = (value.toInt() * 0.001);
          panelCurrentWholeNumber = (int)panelCurrentValue;
          panelCurrentDecimal = (int)((panelCurrentValue - panelCurrentWholeNumber) * 100);
          if (panelCurrentValue < 0)
          {
            isPanelCurrentNegative = 1;
          }
          else
          {
            isPanelCurrentNegative = 0;
          }
        }
        else if (key == "LOAD")
        {
          Serial.print("Panel Load: "); // ON or OFF in string
          Serial.println(value);
        }
        else if (key == "H19")
        {
          float yieldTotal = (value.toInt() * 0.01);
          Serial.print("Yield Total: ");
          Serial.print(yieldTotal);
          Serial.println(" Kw");
        }
        else if (key == "H20")
        {
          float yieldToday = (value.toInt() * 0.01);
          Serial.print("Yield Today: ");
          Serial.print(yieldToday);
          Serial.println(" Kw");
        }
        else if (key == "H21")
        {
          float maxPowerToday = value.toInt();
          Serial.print("Maximum Power Today: ");
          Serial.print(maxPowerToday);
          Serial.println(" W");
        }
        else if (key == "H22")
        {
          float yieldYesterday = (value.toInt() * 0.01);
          Serial.print("Yield Yesterday: ");
          Serial.print(yieldYesterday);
          Serial.println(" Kw");
        }
        else if (key == "H23")
        {
          float maxPowerYesterday = value.toInt();
          Serial.print("Maximum Power Yesterday: ");
          Serial.print(maxPowerYesterday);
          Serial.println(" W");
        }
        else if (key == "ERR")
        {
          Serial.print("Error: "); // Lookup values in ERR of documentation
          Serial.println(value);
        }
        else if (key == "CS")
        {
          // Indicates enum values of charging, etc...
          solarStatus = value.toInt();
        }
        else if (key == "FW")
        {
          Serial.print("Firmware Version: ");
          Serial.println(value);
        }
        else if (key == "PID")
        {
          Serial.print("Product ID: ");
          Serial.println(value);
        }
        else if (key == "SER#")
        {
          Serial.print("Serial Number: ");
          Serial.println(value);
        }
        else if (key == "HSDS")
        {
          Serial.print("Day Number: ");
          Serial.println(value);
        }
        else if (key == "MPPT")
        {
          Serial.print("Tracker Mode: "); // See documentation for ENUMS
          Serial.println(value);
        }
      }
    }
    canHelper::canLoop();
    send_mppt_message();
    send_mppt_message2();
    canStartMillis = canCurrentMillis;
  }
}
