// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4996: 'localtime': This function or variable may be unsafe. Consider using localtime_s
// instead.
#pragma warning(disable : 4996)
#endif

#include <azure/iot/az_iot_pnp_client.h>
#include <iot_sample_common.h>

#include "pnp_mqtt_message.h"

#include "pnp_thermostat_component.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define DEFAULT_START_TEMP_COUNT 1

static char const iso_spec_time_format[] = "%Y-%m-%dT%H:%M:%S%z"; // ISO8601 Time Format

// IoT Hub Device Twin Values
static az_span const twin_desired_temperature_property_name
    = AZ_SPAN_LITERAL_FROM_STR("targetTemperature");
static az_span const twin_reported_maximum_temperature_property_name
    = AZ_SPAN_LITERAL_FROM_STR("maxTempSinceLastReboot");
static az_span const twin_response_success = AZ_SPAN_LITERAL_FROM_STR("success");

// IoT Hub Commands Values
static az_span const command_getMaxMinReport_name = AZ_SPAN_LITERAL_FROM_STR("getMaxMinReport");
static az_span const command_max_temp_name = AZ_SPAN_LITERAL_FROM_STR("maxTemp");
static az_span const command_min_temp_name = AZ_SPAN_LITERAL_FROM_STR("minTemp");
static az_span const command_avg_temp_name = AZ_SPAN_LITERAL_FROM_STR("avgTemp");
static az_span const command_start_time_name = AZ_SPAN_LITERAL_FROM_STR("startTime");
static az_span const command_end_time_name = AZ_SPAN_LITERAL_FROM_STR("endTime");
static az_span const command_empty_response_payload = AZ_SPAN_LITERAL_FROM_STR("{}");
static char command_start_time_value_buffer[32];
static char command_end_time_value_buffer[32];

// IoT Hub Telemetry Values
static az_span const telemetry_temperature_name = AZ_SPAN_LITERAL_FROM_STR("temperature");

static az_result build_command_response_payload(
    pnp_thermostat_component const* thermostat_component,
    az_span start_time,
    az_span end_time,
    az_span payload,
    az_span* out_payload)
{
  az_json_writer jw;
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_init(&jw, payload, NULL));

  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, command_max_temp_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, thermostat_component->maximum_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, command_min_temp_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, thermostat_component->minimum_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, command_avg_temp_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, thermostat_component->average_temperature, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, command_start_time_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_string(&jw, start_time));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, command_end_time_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_string(&jw, end_time));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

static az_result invoke_getMaxMinReport(
    const pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span response,
    az_span* out_response)
{
  int32_t incoming_since_value_len = 0;
  az_json_reader jr;

  // Parse the "since" field in the payload.
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_reader_init(&jr, payload, NULL));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_token_get_string(
      &jr.token,
      command_start_time_value_buffer,
      sizeof(command_start_time_value_buffer),
      &incoming_since_value_len));

  // Set the response payload to error if the "since" field was empty.
  if (incoming_since_value_len == 0)
  {
    *out_response = command_empty_response_payload;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  az_span start_time_span
      = az_span_create((uint8_t*)command_start_time_value_buffer, incoming_since_value_len);

  IOT_SAMPLE_LOG_AZ_SPAN("Start time:", start_time_span);

  // Get the current time as a string.
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  size_t length = strftime(
      command_end_time_value_buffer,
      sizeof(command_end_time_value_buffer),
      iso_spec_time_format,
      timeinfo);
  az_span end_time_span = az_span_create((uint8_t*)command_end_time_value_buffer, (int32_t)length);

  IOT_SAMPLE_LOG_AZ_SPAN("End Time:", end_time_span);

  // Build command response message.
  IOT_SAMPLE_RETURN_IF_FAILED(build_command_response_payload(
      thermostat_component, start_time_span, end_time_span, response, out_response));

  return AZ_OK;
}

az_result pnp_thermostat_init(
    pnp_thermostat_component* out_thermostat_component,
    az_span component_name,
    double initial_temperature)
{
  if (out_thermostat_component == NULL)
  {
    return AZ_ERROR_ARG;
  }

  out_thermostat_component->component_name = component_name;
  out_thermostat_component->average_temperature = initial_temperature;
  out_thermostat_component->current_temperature = initial_temperature;
  out_thermostat_component->maximum_temperature = initial_temperature;
  out_thermostat_component->minimum_temperature = initial_temperature;
  out_thermostat_component->temperature_count = DEFAULT_START_TEMP_COUNT;
  out_thermostat_component->temperature_summation = initial_temperature;
  out_thermostat_component->send_maximum_temperature_property = true;

  return AZ_OK;
}

void pnp_thermostat_build_telemetry_message(
    pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload)
{
  az_result rc;

  az_json_writer jw;

  if (az_result_failed(rc = az_json_writer_init(&jw, payload, NULL))
      || az_result_failed(rc = az_json_writer_append_begin_object(&jw))
      || az_result_failed(rc = az_json_writer_append_property_name(&jw, telemetry_temperature_name))
      || az_result_failed(
          rc = az_json_writer_append_double(
              &jw, thermostat_component->current_temperature, DOUBLE_DECIMAL_PLACE_DIGITS))
      || az_result_failed(rc = az_json_writer_append_end_object(&jw)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to build reported property payload");
    exit(rc);
  }

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

void pnp_thermostat_build_maximum_temperature_reported_property(
    const az_iot_pnp_client* pnp_client,
    pnp_thermostat_component* thermostat_component,
    az_span payload,
    az_span* out_payload,
    az_span* out_property_name)
{
  az_result rc;

  *out_property_name = twin_reported_maximum_temperature_property_name;

  az_json_writer jw;

  if (az_result_failed(rc = az_json_writer_init(&jw, payload, NULL))
      || (az_result_failed(az_json_writer_append_begin_object(&jw)))
      || az_result_failed(
          rc = az_iot_pnp_client_twin_property_begin_component(
              pnp_client, &jw, thermostat_component->component_name))
      || az_result_failed(rc = az_json_writer_append_property_name(&jw, telemetry_temperature_name))
      || az_result_failed(
          rc = az_json_writer_append_double(
              &jw, thermostat_component->current_temperature, DOUBLE_DECIMAL_PLACE_DIGITS))
      || az_result_failed(rc = az_iot_pnp_client_twin_property_end_component(pnp_client, &jw))
      || az_result_failed(rc = az_json_writer_append_end_object(&jw)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to build reported property payload");
    exit(rc);
  }

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}

az_result pnp_thermostat_process_property_update(
    az_iot_pnp_client* pnp_client,
    pnp_thermostat_component* ref_thermostat_component,
    az_json_token const* property_name,
    az_json_reader const* property_value,
    int32_t version,
    az_span payload,
    az_span* out_payload)
{
  double parsed_property_value = 0;

  if (!az_json_token_is_text_equal(property_name, twin_desired_temperature_property_name))
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  else
  {
    IOT_SAMPLE_RETURN_IF_FAILED(
        az_json_token_get_double(&property_value->token, &parsed_property_value));

    // Update variables locally.
    ref_thermostat_component->current_temperature = parsed_property_value;
    if (ref_thermostat_component->current_temperature
        > ref_thermostat_component->maximum_temperature)
    {
      ref_thermostat_component->maximum_temperature = ref_thermostat_component->current_temperature;
      ref_thermostat_component->send_maximum_temperature_property = true;
    }

    if (ref_thermostat_component->current_temperature
        < ref_thermostat_component->minimum_temperature)
    {
      ref_thermostat_component->minimum_temperature = ref_thermostat_component->current_temperature;
    }

    // Calculate and update the new average temperature.
    ref_thermostat_component->temperature_count++;
    ref_thermostat_component->temperature_summation
        += ref_thermostat_component->current_temperature;
    ref_thermostat_component->average_temperature = ref_thermostat_component->temperature_summation
        / ref_thermostat_component->temperature_count;

    IOT_SAMPLE_LOG_SUCCESS("Client updated desired temperature variables locally.");
    IOT_SAMPLE_LOG("Current Temperature: %2f", ref_thermostat_component->current_temperature);
    IOT_SAMPLE_LOG("Maximum Temperature: %2f", ref_thermostat_component->maximum_temperature);
    IOT_SAMPLE_LOG("Minimum Temperature: %2f", ref_thermostat_component->minimum_temperature);
    IOT_SAMPLE_LOG("Average Temperature: %2f", ref_thermostat_component->average_temperature);

    az_json_writer jw;
    az_result rc = az_json_writer_init(&jw, payload, NULL);

    if ((rc = az_json_writer_append_begin_object(&jw))
        || (rc = az_iot_pnp_client_twin_begin_property_with_status(
                pnp_client,
                &jw,
                ref_thermostat_component->component_name,
                property_name->slice,
                (int32_t)AZ_IOT_STATUS_OK,
                version,
                twin_response_success))
        || (rc
            = az_json_writer_append_double(&jw, parsed_property_value, DOUBLE_DECIMAL_PLACE_DIGITS))
        || (rc = az_iot_pnp_client_twin_end_property_with_status(
                pnp_client, &jw, ref_thermostat_component->component_name))
        || (rc = az_json_writer_append_end_object(&jw)))
    {
      IOT_SAMPLE_LOG_ERROR(
          "Failed to create property with status payload, az_result return code 0x%08x.", rc);
      exit(rc);
    }

    if (az_result_failed(rc))
    {
      IOT_SAMPLE_LOG_ERROR(
          "Failed to get reported property payload with status: az_result return code 0x%08x.", rc);
      exit(rc);
    }

    *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

    // Build reported property message with status.
    // if (az_result_failed(
    //         rc = pnp_build_reported_property_with_status(
    //             payload,
    //             ref_thermostat_component->component_name,
    //             property_name->slice,
    //             append_double_callback,
    //             (void*)&parsed_property_value,
    //             (int32_t)AZ_IOT_STATUS_OK,
    //             version,
    //             twin_response_success,
    //             out_payload)))
    // {
    //   IOT_SAMPLE_LOG_ERROR(
    //       "Failed to get reported property payload with status: az_result return code 0x%08x.", rc);
    //   exit(rc);
    // }
  }

  return AZ_OK;
}

az_result pnp_thermostat_process_command_request(
    pnp_thermostat_component const* thermostat_component,
    az_span command_name,
    az_span command_received_payload,
    az_span payload,
    az_span* out_payload,
    az_iot_status* out_status)
{
  az_result rc;

  if (az_span_is_content_equal(command_getMaxMinReport_name, command_name))
  {
    // Invoke command.
    rc = invoke_getMaxMinReport(
        thermostat_component, command_received_payload, payload, out_payload);

    if (az_result_failed(rc))
    {
      *out_payload = command_empty_response_payload;
      *out_status = AZ_IOT_STATUS_BAD_REQUEST;

      IOT_SAMPLE_LOG_AZ_SPAN(
          "Bad request when invoking command on Thermostat Sensor component:", command_name);
    }
    else
    {
      *out_status = AZ_IOT_STATUS_OK;
    }
  }
  else
  {
    *out_payload = command_empty_response_payload;
    *out_status = AZ_IOT_STATUS_NOT_FOUND;

    IOT_SAMPLE_LOG_AZ_SPAN("Command not supported on Thermostat Sensor component:", command_name);
    rc = AZ_ERROR_ITEM_NOT_FOUND; // Unsupported command
  }

  return rc;
}
