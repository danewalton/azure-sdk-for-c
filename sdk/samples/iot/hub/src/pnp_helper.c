// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#include "pnp_helper.h"

#define JSON_DOUBLE_DIGITS 2

static const az_span desired_property_name = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span component_telemetry_prop_span = AZ_SPAN_LITERAL_FROM_STR("$.sub");
static const az_span desired_temp_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span desired_temp_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span desired_temp_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span desired_temp_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static char component_specifier_name[] = "__t";
static char component_specifier_value[] = "c";
static char command_separator[] = "*";

static void strip_quotes_from_span(az_span input_span, az_span* out_span)
{
  *out_span = az_span_slice(input_span, 1, az_span_size(input_span) - 1);
}

static az_result build_reported_property_with_status(
    az_json_writer* json_writer,
    az_span property_name,
    az_span property_val,
    int32_t ack_code_value,
    int32_t ack_version_value,
    az_span ack_description_value)
{
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_writer, desired_temp_response_value_name));

  if (*(char*)az_span_ptr(property_val) == '"')
  {
    az_span stripped_val;
    strip_quotes_from_span(property_val, &stripped_val);
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, stripped_val));
  }
  else
  {
    double value_as_double;
    AZ_RETURN_IF_FAILED(az_span_atod(property_val, &value_as_double));
    AZ_RETURN_IF_FAILED(
        az_json_writer_append_double(json_writer, value_as_double, JSON_DOUBLE_DIGITS));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, desired_temp_ack_code_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(json_writer, ack_code_value));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_writer, desired_temp_ack_version_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_int32(json_writer, ack_version_value));

  if (az_span_ptr(ack_description_value) != NULL)
  {
    AZ_RETURN_IF_FAILED(
        az_json_writer_append_property_name(json_writer, desired_temp_ack_description_name));
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, ack_description_value));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));

  return AZ_OK;
}

static az_result build_reported_property(
    az_json_writer* json_writer,
    az_span component,
    az_span name,
    az_span value,
    az_span* out_span)
{
  bool has_component = az_span_ptr(component) != NULL;

  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));

  if(has_component)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, component));
    AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
    AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(
        json_writer, AZ_SPAN_FROM_BUFFER(component_specifier_name)));
    AZ_RETURN_IF_FAILED(
        az_json_writer_append_string(json_writer, AZ_SPAN_FROM_BUFFER(component_specifier_value)));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, name));

  if (*(char*)az_span_ptr(value) == '"')
  {
    az_span stripped_val;
    strip_quotes_from_span(value, &stripped_val);
    AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, stripped_val));
  }
  else
  {
    double value_as_double;
    AZ_RETURN_IF_FAILED(az_span_atod(value, &value_as_double));
    AZ_RETURN_IF_FAILED(
        az_json_writer_append_double(json_writer, value_as_double, JSON_DOUBLE_DIGITS));
  }

  if(has_component)
  {
    AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));
  }

  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));

  *out_span = az_json_writer_get_json(json_writer);

  return AZ_OK;
}

az_result pnp_helper_get_telemetry_topic(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties* properties,
    az_span component_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  az_result result;

  if ((result = az_iot_hub_client_properties_append(
           properties, component_telemetry_prop_span, component_name))
      != AZ_OK)
  {
    return result;
  }
  else
  {
    result = az_iot_hub_client_telemetry_get_publish_topic(
        client, properties, mqtt_topic, mqtt_topic_size, out_mqtt_topic_length);
  }

  return result;
}

az_result pnp_helper_parse_command_name(
    char* component_command,
    int32_t component_command_size,
    az_span* component_name,
    az_span* pnp_command_name)
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

az_result pnp_helper_create_reported_property(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    az_span property_json_value,
    az_span* out_span)
{
  az_result result;

  az_json_writer json_writer;
  result = az_json_writer_init(&json_writer, json_buffer, NULL);
  result = build_reported_property(
      &json_writer, component_name, property_name, property_json_value, out_span);

  return result;
}

az_result pnp_helper_create_reported_property_with_status(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    az_span property_json_value,
    int32_t ack_value,
    int32_t ack_version,
    az_span ack_description)
{
  az_result result;

  az_json_writer json_writer;
  result = az_json_writer_init(&json_writer, json_buffer, NULL);
  if (az_span_ptr(component_name) != NULL)
  {
    result = AZ_OK;
  }
  else
  {
    result = build_reported_property_with_status(
        &json_writer, property_name, property_json_value, ack_value, ack_version, ack_description);
  }
  return result;
}

az_result pnp_helper_process_twin_data(
    az_span payload,
    pnp_helper_property_callback user_twin_callback,
    void* user_twin_callback_context)
{
  az_result result;

  int32_t prop_version;
  az_span component_name;
  az_json_reader json_reader;
  az_json_reader_init(&json_reader, payload, NULL);

  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));

  if (az_json_token_is_text_equal(&json_reader.token, desired_property_name))
  {
    if (json_reader.token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    az_span prop_name;
    az_json_token prop_value;
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));
    prop_name = json_reader.token.slice;
    strip_quotes_from_span(prop_name, &prop_name);
    AZ_RETURN_IF_FAILED(az_json_reader_next_token(&json_reader));

    user_twin_callback(component_name, prop_name, prop_value, prop_version, user_twin_callback_context);

    result = AZ_OK;
  }
  else
  {
    // Some error couldn't find the desired prop name
    result = AZ_ERROR_UNEXPECTED_CHAR;
  }

  return result;
}

