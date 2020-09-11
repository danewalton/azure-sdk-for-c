// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/iot/az_iot_pnp_client.h>

az_iot_pnp_client pnp_client;

int32_t max_temp;
int32_t max_elevation;

static const az_span thermostat_1_name = AZ_SPAN_LITERAL_FROM_STR("thermostat1");
static const az_span thermostat_2_name = AZ_SPAN_LITERAL_FROM_STR("thermostat2");
static const az_span device_info_name = AZ_SPAN_LITERAL_FROM_STR("deviceInformation");
static const az_span* pnp_components[]
    = { &thermostat_1_name, &thermostat_2_name, &device_info_name };

//
// Functions
//
static void create_and_configure_mqtt_client(void);
static void connect_mqtt_client_to_iot_hub(void);
static void subscribe_mqtt_client_to_iot_hub_topics(void);
static void request_device_twin_document(void);
static void receive_messages(void);

int main(void)
{
  create_and_configure_mqtt_client();
  LOG_SUCCESS("Client created and configured.");

  connect_mqtt_client_to_iot_hub();
  LOG_SUCCESS("Client connected to IoT Hub.");

  subscribe_mqtt_client_to_iot_hub_topics();
  LOG_SUCCESS("Client subscribed to IoT Hub topics.");

  request_device_twin_document();
  LOG_SUCCESS("Client requested twin document.");

  receive_messages();
  LOG_SUCCESS("Client received messages.");

  return 0;
}

static void handle_device_twin_message(
    const az_span twin_message_span,
    const az_iot_hub_client_twin_response* twin_response)
{
  // Invoke appropriate action per response type (3 Types only).
  bool is_twin_get = false;

  switch (twin_response->response_type)
  {
    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET:
      LOG("Type: GET");
      is_twin_get = true;
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES:
      LOG("Type: Desired Properties");
      break;

    case AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES:
      LOG("Type: Reported Properties");
      break;
  }

  // For a GET response OR a Desired Properties response from the server:
  // 1. Parse for the desired temperature
  // 2. Update device temperature locally
  // 3. Report updated temperature to server
  if (twin_response->response_type == AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_GET
      || twin_response->response_type == AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES)
  {
    double desired_temp;
    int32_t version_num;

    az_json_reader jr;
    az_json_token component_name;
    az_json_token property_name;
    az_json_reader property_value;
    az_json_reader_init(&jr, twin_message_span, NULL);

    az_result result;
    while (az_result_succeeded(result = az_iot_pnp_client_twin_get_next_component(
          &pnp_client, &jr, !is_twin_get, &component_name, &version_num)))
    {
      if (result == AZ_OK)
      {
        az_iot_pnp_client_twin_get_next_component_property(
            &pnp_client, &jr, &property_name, &property_value);
        if (az_json_token_is_text_equal(&component_name, thermostat_1_name))
        {
          thermostat_process_property_update(component_name, property_name, &property_value);
        }
        else if (az_json_token_is_text_equal(&component_name, thermostat_2_name))
        {
          thermostat_process_property_update(component_name, property_name, &property_value);
        }
        else if (az_json_token_is_text_equal(&component_name, device_info_name))
        {
          // device_info_process_property_update(component_name, property_name, &property_value);
        }
      }
      else if (result == AZ_IOT_ITEM_NOT_COMPONENT)
      {
        az_iot_pnp_client_twin_get_next_component_property(
            &pnp_client, &jr, &property_name, &property_value);
      }
      else if (result == AZ_IOT_END_OF_COMPONENTS)
      {
        break;
      }
      else
      {
        return result;
      }
    }
  }
}

static void on_message_received(char* topic, int topic_len, char* payload, int payload_len)
{
  az_result rc;

  az_span topic_span = az_span_create((uint8_t*)topic, topic_len);
  az_span message_span = az_span_create((uint8_t*)payload, payload_len);

  az_iot_pnp_client_twin_response twin_response;
  az_iot_pnp_client_command_request command_request;

  // Parse the incoming message topic and check which feature it is for.
  if (az_succeeded(
          rc
          = az_iot_pnp_client_twin_parse_received_topic(&pnp_client, topic_span, &twin_response)))
  {
    handle_device_twin_message(message_span, &twin_response);
  }
  else if (az_succeeded(az_iot_pnp_client_methods_parse_received_topic(
               &pnp_client, topic_span, &command_request)))
  {
    handle_command_message(message_span, &command_request);
  }
  else
  {
    LOG_ERROR("Message from unknown topic: az_result return code 0x%04x.", rc);
    LOG_AZ_SPAN("Topic:", topic_span);
    exit(rc);
  }
}

static void receive_messages(void)
{
  bool received = false;
  char* topic;
  int topic_len;
  char* message;
  int message_len;

  while (1)
  {
    mqtt_receive();

    if (received)
    {
      on_message_received(topic, topic_len, message, message_len);
    }

    // Send a telemetry message
    send_telemetry_message();
  }
}
