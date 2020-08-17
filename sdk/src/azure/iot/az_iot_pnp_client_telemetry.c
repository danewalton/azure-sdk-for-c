// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include <azure/core/az_precondition.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/_az_cfg.h>

// Property values
static char pnp_properties_buffer[64];
static const az_span component_telemetry_prop_span = AZ_SPAN_LITERAL_FROM_STR("$.sub");

AZ_NODISCARD az_result az_iot_pnp_client_telemetry_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span component_name,
    az_iot_hub_client_properties const* properties,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(component_name, 1, false);
  _az_PRECONDITION_NOT_NULL(mqtt_topic);
  _az_PRECONDITION(mqtt_topic_size > 0);

  az_iot_hub_client_properties pnp_properties;

  if (properties == NULL)
  {
    properties = &pnp_properties;

    AZ_RETURN_IF_FAILED(az_iot_hub_client_properties_init(
        properties, AZ_SPAN_FROM_BUFFER(pnp_properties_buffer), 0));
  }

  AZ_RETURN_IF_FAILED(az_iot_hub_client_properties_append(
      properties, component_telemetry_prop_span, component_name));

  AZ_RETURN_IF_FAILED(az_iot_hub_client_telemetry_get_publish_topic(
      client, properties, mqtt_topic, mqtt_topic_size, out_mqtt_topic_length));

  return AZ_OK;
}