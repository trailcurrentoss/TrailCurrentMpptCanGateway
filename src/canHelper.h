#pragma once
#include "globals.h"
#include "driver/twai.h"
#define CAN_RX 13
#define CAN_TX 15
#define POLLING_RATE_MS 100
#define CAN_SEND_MESSAGE_IDENTIFIER 0x23;
static bool driver_installed = false;

namespace canHelper
{
  void canSetup()
  {
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NO_ACK);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS(); // Look in the api-reference for other speed sets.
    twai_filter_config_t f_config = {
      .acceptance_code = (0x99),
      .acceptance_mask = 0x1FFFFFFF,
      .single_filter = true};
    //twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
      debugln("Driver installed");
    }
    else
    {
      debugln("Failed to install driver");
      return;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
      debugln("Driver started");
    }
    else
    {
      debugln("Failed to start driver");
      return;
    }

    // Reconfigure alerts to detect frame receive, Bus-Off error and RX queue full states
    uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL;
    if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
    {
      debugln("CAN Alerts reconfigured");
    }
    else
    {
      debugln("Failed to reconfigure alerts");
      return;
    }

    // TWAI driver is now successfully installed and started
    driver_installed = true;
  }

  static void handle_rx_message(twai_message_t &message)
  {
    // Process received message
    if (message.extd)
    {
      // debugln("Message is in Extended Format");
    }
    else
    {
      // debugln("Message is in Standard Format");
    }
    // debugf("ID: %lx\nByte:", message.identifier);
    /* if (!(message.rtr))
    {      
      for (int i = 0; i < message.data_length_code; i++)
      {
        debugg(" %d = %02x,", i, message.data[i]);
      }
      debugln("");
    } */
  }

  void canLoop()
  {
    if (!driver_installed)
    {
      // Driver not installed
      delay(1000);
      return;
    }
    // Check if alert happened
    uint32_t alerts_triggered;
    twai_read_alerts(&alerts_triggered, pdMS_TO_TICKS(POLLING_RATE_MS));
    twai_status_info_t twaistatus;
    twai_get_status_info(&twaistatus);

    // Handle alerts
    if (alerts_triggered & TWAI_ALERT_ERR_PASS)
    {
      debugln("Alert: TWAI controller has become error passive.");
    }
    if (alerts_triggered & TWAI_ALERT_BUS_ERROR)
    {
      debugln("Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus.");
      debugf("Bus error count: %lu\n", twaistatus.bus_error_count);
    }
    if (alerts_triggered & TWAI_ALERT_RX_QUEUE_FULL)
    {
      // debugln("Alert: The RX queue is full causing a received frame to be lost.");
      // debugf("RX buffered: %lu\t", twaistatus.msgs_to_rx);
      // debugf("RX missed: %lu\t", twaistatus.rx_missed_count);
      // debugf("RX overrun %lu\n", twaistatus.rx_overrun_count);
    }

    // Check if message is received
    if (alerts_triggered & TWAI_ALERT_RX_DATA)
    {
      // One or more messages received. Handle all.
      twai_message_t message;
      while (twai_receive(&message, 0) == ESP_OK)
      {
        handle_rx_message(message);
      }
    }
  }
}