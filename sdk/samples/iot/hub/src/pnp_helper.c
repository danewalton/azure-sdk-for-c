// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#include "pnp_helper.h"

#define JSON_DOUBLE_DIGITS 2

static const az_span component_telemetry_prop_span = AZ_SPAN_LITERAL_FROM_STR("$.sub");
static const az_span desired_temp_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span desired_temp_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span desired_temp_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span desired_temp_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span component_specifier_name = AZ_SPAN_LITERAL_FROM_STR("__t");
static const az_span component_specifier_value = AZ_SPAN_LITERAL_FROM_STR("c");
static const az_span command_separator = AZ_SPAN_LITERAL_FROM_STR("*");

/* Device twin keys */
static const az_span sample_iot_hub_twin_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span sample_iot_hub_twin_desired = AZ_SPAN_LITERAL_FROM_STR("desired");

static void strip_quotes_from_span(az_span input_span, az_span* out_span)
{
  *out_span = az_span_slice(input_span, 1, az_span_size(input_span) - 1);
}

static int32_t visit_component_properties(
    az_span component_name,
    az_json_reader* json_reader,
    int32_t version,
    char* scratch_buf,
    int32_t scratch_buf_len,
    pnp_helper_property_callback property_callback,
    void* context_ptr)
{
  int32_t len;

  while (az_succeeded(az_json_reader_next_token(json_reader)))
  {
    if (json_reader->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      if (az_failed(az_json_token_get_string(
              &(json_reader->token), (char*)scratch_buf, (int32_t)scratch_buf_len, (int32_t*)&len)))
      {
        printf("Failed to get string property value\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      if (az_failed(az_json_reader_next_token(json_reader)))
      {
        printf("Failed to get next token\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      if (memcmp(
              (void*)scratch_buf,
              (void*)az_span_ptr(component_specifier_name),
              (size_t)az_span_size(component_specifier_name))
          == 0)
      {
        continue;
      }

      if ((memcmp(
               (void*)scratch_buf,
               (void*)az_span_ptr(sample_iot_hub_twin_desired_version),
               (size_t)az_span_size(sample_iot_hub_twin_desired_version))
           == 0))
      {
        continue;
      }

      property_callback(
          component_name,
          az_span_init((uint8_t*)scratch_buf, len),
          &(json_reader->token),
          version,
          context_ptr);
    }

    if (json_reader->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_failed(az_json_reader_skip_children(json_reader)))
      {
        printf("Failed to skip children of object\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      break;
    }
  }

  return AZ_OK;
}

/* Move reader to the value of property name */
static az_result sample_json_child_token_move(az_json_reader* json_reader, az_span property_name)
{
  while (az_succeeded(az_json_reader_next_token(json_reader)))
  {
    if ((json_reader->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
        && az_json_token_is_text_equal(&(json_reader->token), property_name))
    {
      if (az_failed(az_json_reader_next_token(json_reader)))
      {
        printf("Failed to read next token\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      return AZ_OK;
    }
    else if (json_reader->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_failed(az_json_reader_skip_children(json_reader)))
      {
        printf("Failed to skip child of complex object\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

static az_result is_component_in_model(
    az_span component_name,
    az_span** sample_components_ptr,
    int32_t sample_components_num,
    int32_t* out_index)
{
  int32_t index = 0;

  if (az_span_ptr(component_name) == NULL || az_span_size(component_name) == 0)
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  while (index < sample_components_num)
  {
    if ((az_span_size(component_name) == az_span_size(*sample_components_ptr[index]))
        && (memcmp(
                (void*)az_span_ptr(component_name),
                (void*)az_span_ptr(*sample_components_ptr[index]),
                (size_t)az_span_size(component_name))
            == 0))
    {
      *out_index = index;
      return AZ_OK;
    }

    index++;
  }

  return AZ_ERROR_UNEXPECTED_CHAR;
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
        json_writer, component_specifier_name));
    AZ_RETURN_IF_FAILED(
        az_json_writer_append_string(json_writer, component_specifier_value));
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
  int32_t index = az_span_find(component_command_span, command_separator);
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
    az_json_reader json_reader,
    int32_t is_partial,
    az_span** sample_components_ptr,
    int32_t sample_components_num,
    char* scratch_buf,
    int32_t scratch_buf_len,
    pnp_helper_property_callback property_callback,
    void* context_ptr)
{
  az_json_reader copy_json_reader;
  int32_t version;
  int32_t len;
  int32_t index;

  if (!is_partial && sample_json_child_token_move(&json_reader, sample_iot_hub_twin_desired))
  {
    printf("Failed to get desired property\r\n");
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  copy_json_reader = json_reader;
  if (sample_json_child_token_move(&copy_json_reader, sample_iot_hub_twin_desired_version)
      || az_failed(az_json_token_get_int32(&(copy_json_reader.token), (int32_t*)&version)))
  {
    printf("Failed to get version\r\n");
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  while (az_succeeded(az_json_reader_next_token(&json_reader)))
  {
    if (json_reader.token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      if (az_failed(az_json_token_get_string(
              &(json_reader.token), (char*)scratch_buf, (int32_t)scratch_buf_len, (int32_t*)&len)))
      {
        printf("Failed to string value for property name\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      if (az_failed(az_json_reader_next_token(&json_reader)))
      {
        printf("Failed to next token\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      if ((len == (int32_t)az_span_size(sample_iot_hub_twin_desired_version))
          && (memcmp(
                  (void*)az_span_ptr(sample_iot_hub_twin_desired_version),
                  (void*)scratch_buf,
                  (size_t)az_span_size(sample_iot_hub_twin_desired_version))
              == 0))
      {
        continue;
      }

      if (json_reader.token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT && sample_components_ptr != NULL
          && (is_component_in_model(
                  az_span_init((uint8_t*)scratch_buf, len), sample_components_ptr, sample_components_num, &index)
              == AZ_OK))
      {
        if (visit_component_properties(
                *sample_components_ptr[index],
                &json_reader,
                version,
                scratch_buf,
                scratch_buf_len,
                property_callback,
                context_ptr))
        {
          printf("Failed to visit component properties\r\n");
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
      }
      else
      {
        property_callback(
            AZ_SPAN_NULL,
            az_span_init((uint8_t*)scratch_buf, (int32_t)len),
            &(json_reader.token),
            version,
            context_ptr);
      }
    }
    else if (json_reader.token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_failed(az_json_reader_skip_children(&json_reader)))
      {
        printf("Failed to skip children of object\r\n");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (json_reader.token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      break;
    }
  }

  return AZ_OK;
}

