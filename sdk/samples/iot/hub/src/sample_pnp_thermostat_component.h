// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_PNP_THERMOSTAT_COMPONENT_H
#define SAMPLE_PNP_THERMOSTAT_COMPONENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

typedef struct sample_pnp_thermostat_component_tag
{
  az_span component_name;
  double current_temperature;
  double min_temperature;
  double max_temperature;
  int32_t device_temperature_avg_count;
  double device_temperature_avg_total;
  double avg_temperature;
} sample_pnp_thermostat_component;

az_result sample_pnp_thermostat_init(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    double initial_temp);

az_result sample_pnp_thermostat_process_property_update(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    az_span property_name,
    az_json_token* property_value,
    int32_t version);

az_result sample_pnp_thermostat_process_command(
    sample_pnp_thermostat_component* handle,
    az_span component_name,
    az_span pnp_command_name,
    az_span response_topic_span,
    az_span response_payload_span,
    az_span* out_response_topic_span,
    az_span* out_response_payload_span);

#ifdef __cplusplus
}
#endif

#endif // SAMPLE_PNP_THERMOSTAT_COMPONENT_H

