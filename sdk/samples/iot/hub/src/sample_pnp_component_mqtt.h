// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#ifndef SAMPLE_PNP_COMPONENT_MQTT_H
#define SAMPLE_PNP_COMPONENT_MQTT_H

#include <stdint.h>

#include <azure/core/az_span.h>

typedef struct sample_pnp_mqtt_message_tag
{
  char* topic;
  size_t topic_length;
  size_t* out_topic_length;
  az_span payload_span;
  az_span out_payload_span;
} sample_pnp_mqtt_message;

#endif // SAMPLE_PNP_COMPONENT_MQTT_H
