// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include "sample_pnp_thermostat_component.h"

#define DOUBLE_DECIMAL_PLACE_DIGITS 2

// IoT Hub Commands Values
static const az_span report_command_name_span = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static const az_span report_max_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static const az_span report_min_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static const az_span report_avg_temp_name_span = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static const az_span report_start_time_name_span = AZ_SPAN_LITERAL_FROM_STR("startTime");
static const az_span report_end_time_name_span = AZ_SPAN_LITERAL_FROM_STR("endTime");
static const az_span report_error_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char end_time_buffer[32];
static char incoming_since_value[32];

// ISO8601 Time Format
static const char iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z";

static az_result build_command_response_payload(
    sample_pnp_thermostat_component* handle,
    az_json_writer* json_builder,
    az_span start_time_span,
    az_span end_time_span,
    az_span* response_payload)
{
  double avg_temp = handle->device_temperature_avg_total / handle->device_temperature_avg_count;
  // Build the command response payload
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_builder));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_max_temp_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(
      json_builder, handle->max_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_min_temp_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(
      json_builder, handle->min_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_avg_temp_name_span));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_double(json_builder, avg_temp, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_builder, report_start_time_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, start_time_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_builder, report_end_time_name_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_builder, end_time_span));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_builder));

  *response_payload = az_json_writer_get_json(json_builder);

  return AZ_OK;
}

// Invoke the command requested from the service. Here, it generates a report for max, min, and avg
// temperatures.
static az_result invoke_getMaxMinReport(
    sample_pnp_thermostat_component* handle,
    az_span payload,
    az_span response,
    az_span* out_response)
{
  // az_result result;
  // Parse the "since" field in the payload.
  az_span start_time_span = AZ_SPAN_NULL;
  az_json_reader jp;
  AZ_RETURN_IF_FAILED(az_json_reader_init(&jp, payload, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jp));
  int32_t incoming_since_value_len;
  AZ_RETURN_IF_FAILED(az_json_token_get_string(
      &jp.token, incoming_since_value, sizeof(incoming_since_value), &incoming_since_value_len));
  start_time_span = az_span_init((uint8_t*)incoming_since_value, incoming_since_value_len);

  // Set the response payload to error if the "since" field was not sent
  if (az_span_ptr(start_time_span) == NULL)
  {
    response = report_error_payload;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  // Get the current time as a string
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t len = strftime(end_time_buffer, sizeof(end_time_buffer), iso_spec_time_format, timeinfo);
  az_span end_time_span = az_span_init((uint8_t*)end_time_buffer, (int32_t)len);

  az_json_writer json_builder;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&json_builder, response, NULL));
  AZ_RETURN_IF_FAILED(build_command_response_payload(
      handle, &json_builder, start_time_span, end_time_span, out_response));

  return AZ_OK;
}

az_result sample_pnp_thermostat_init(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    double initial_temp)
{
  if (handle == NULL)
  {
    return AZ_ERROR_ARG;
  }

  handle->component_name = component_name;
  handle->current_temperature = initial_temp;
  handle->min_temperature = initial_temp;
  handle->max_temperature = initial_temp;
  handle->device_temperature_avg_count = 1;
  handle->device_temperature_avg_total = initial_temp;
  handle->avg_temperature = initial_temp;

  return AZ_OK;
}

az_result sample_pnp_thermostat_process_property_update(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    az_span property_name,
    az_json_token* property_value,
    int32_t version)
{

  (void)handle;
  (void)component_name;
  (void)property_name;
  (void)property_value;
  (void)version;

  return AZ_OK;
}

az_result sample_pnp_thermostat_process_command(
    az_iot_hub_client* client,
    sample_pnp_thermostat_component* handle,
    az_iot_hub_client_method_request* command_request,
    az_span component_name,
    az_span command_name,
    az_span command_payload,
    char* response_topic,
    size_t response_topic_length,
    size_t* out_response_topic_length,
    az_span response_payload_span,
    az_span* out_response_payload_span)
{
  az_result result;

  if (az_span_is_content_equal(handle->component_name, component_name)
      && az_span_is_content_equal(report_command_name_span, command_name))
  {
    // Invoke command
    uint16_t return_code;
    az_result response = invoke_getMaxMinReport(
        handle, command_payload, response_payload_span, out_response_payload_span);
    if (response != AZ_OK)
    {
      return_code = 400;
    }
    else
    {
      return_code = 200;
    }

    if (az_failed(
            result = az_iot_hub_client_methods_response_get_publish_topic(
                client,
                command_request->request_id,
                return_code,
                response_topic,
                response_topic_length,
                out_response_topic_length)))
    {
      printf("Unable to get twin document publish topic\n");
      return result;
    }
  }
  else
  {
    // Unsupported command
    printf(
        "Unsupported command received: %.*s.\n",
        az_span_size(command_request->name),
        az_span_ptr(command_request->name));

            if (az_failed(
            result = az_iot_hub_client_methods_response_get_publish_topic(
                client,
                command_request->request_id,
                404,
                response_topic,
                response_topic_length,
                out_response_topic_length)))
    {
      printf("Unable to get twin document publish topic\n");
      return result;
    }

    *out_response_payload_span = report_error_payload;

  }

  return result;
}
