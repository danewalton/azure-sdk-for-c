// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include "pnp_helper.h"

static char component_specifier_name[] = "__t";
static char component_specifier_value[] = "c";
static char command_separator[] = "*";

static az_result build_component_reported_property(
    az_span json_buffer,
    az_span component,
    az_span name,
    az_span value,
    az_span* out_span)
{
  az_json_writer json_writer;
  AZ_RETURN_IF_FAILED(
      az_json_writer_init(&json_writer, json_buffer, NULL));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_writer));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&json_writer, component));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&json_writer));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &json_writer, AZ_SPAN_FROM_BUFFER(component_specifier_name)));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_string(&json_writer, AZ_SPAN_FROM_BUFFER(component_specifier_value)));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &json_writer, name));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_string(&json_writer, value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_writer));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&json_writer));

  *out_span = az_json_writer_get_json(&json_writer);

  return AZ_OK;
}

az_result pnp_helper_parse_command_name(
    char* component_command,
    int32_t component_command_size,
    az_span* component_name,
    az_span* pnp_command_name,
    az_span* out_span)
{
  az_span component_command_span
      = az_span_init((uint8_t*)component_command, component_command_size);
  int32_t index = az_span_find(component_command_span, AZ_SPAN_FROM_BUFFER(command_separator));
  if (index > 0)
  {
    *component_name = az_span_slice(component_command_span, 0, index - 1);
    *pnp_command_name
        = az_span_slice(component_command_span, index, az_span_size(component_command_span));
  }
  else
  {
    *component_name = AZ_SPAN_NULL;
    *pnp_command_name = component_command_span;
  }
  return AZ_OK;
}

az_result pnp_helper_create_reported_property_double(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    double property_value_double,
    az_span* out_span)
{
  az_result result;

  // Temporary code section
  (void)property_value_double;
  char double_as_span_buffer[16];
  az_span double_as_span = az_span_init((uint8_t*)double_as_span_buffer, sizeof(double_as_span));
  result = az_span_dtoa(double_as_span, property_value_double, 2, &double_as_span);

  result = build_component_reported_property(
      json_buffer, component_name, property_name, double_as_span, &json_buffer);

  *out_span = json_buffer;

  return result;
}
