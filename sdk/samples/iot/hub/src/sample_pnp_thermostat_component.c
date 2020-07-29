// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

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
  az_result result;

  return result;
}

az_result sample_pnp_thermostat_process_command(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    az_span pnp_command_name,
    az_span response_topic_span,
    az_span response_payload_span,
    az_span* out_response_topic_span,
    az_span* out_response_payload_span)
{
  az_result result;

  return result;
}

