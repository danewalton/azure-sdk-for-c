// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

static const az_span iot_hub_twin_desired = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span iot_hub_twin_reported = AZ_SPAN_LITERAL_FROM_STR("reported");
static const az_span iot_hub_twin_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
static const az_span property_response_value_name = AZ_SPAN_LITERAL_FROM_STR("value");
static const az_span property_ack_code_name = AZ_SPAN_LITERAL_FROM_STR("ac");
static const az_span property_ack_version_name = AZ_SPAN_LITERAL_FROM_STR("av");
static const az_span property_ack_description_name = AZ_SPAN_LITERAL_FROM_STR("ad");
static const az_span component_property_label_name = AZ_SPAN_LITERAL_FROM_STR("__t");
static const az_span component_property_label_value = AZ_SPAN_LITERAL_FROM_STR("c");

AZ_NODISCARD az_result az_iot_pnp_client_twin_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_twin_response* out_twin_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_VALID_SPAN(received_topic, 1, false);
  _az_PRECONDITION_NOT_NULL(out_twin_response);

  az_iot_hub_client_twin_response hub_twin_response;
  _az_RETURN_IF_FAILED(az_iot_hub_client_twin_parse_received_topic(
      &client->_internal.iot_hub_client, received_topic, &hub_twin_response));

  out_twin_response->request_id = hub_twin_response.request_id;
  out_twin_response->response_type = hub_twin_response.response_type;
  out_twin_response->status = hub_twin_response.status;
  out_twin_response->version = hub_twin_response.version;

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_property_begin_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION_VALID_SPAN(component_name, 1, false);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, component_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_writer, component_property_label_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, component_property_label_value));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_property_end_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);

  (void)client;

  return az_json_writer_append_end_object(json_writer);
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_begin_property_with_status(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name,
    az_span property_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION_VALID_SPAN(property_name, 1, false);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  if (az_span_size(component_name) != 0)
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, component_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
    _az_RETURN_IF_FAILED(
        az_json_writer_append_property_name(json_writer, component_property_label_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, component_property_label_value));
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, property_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_writer, property_response_value_name));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_end_property_with_status(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name,
    int32_t ack_code,
    int32_t ack_version,
    az_span ack_description)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);

  (void)client;

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, property_ack_code_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(json_writer, ack_code));
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, property_ack_version_name));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(json_writer, ack_version));

  if (az_span_size(ack_description) != 0)
  {
    _az_RETURN_IF_FAILED(
        az_json_writer_append_property_name(json_writer, property_ack_description_name));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, ack_description));
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));
  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));

  if (az_span_size(component_name) != 0)
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_end_object(json_writer));
  }

  return AZ_OK;
}

// Visit each valid property for the component
static az_result visit_component_properties(
    az_json_reader* jr,
    az_json_token* property_name,
    az_json_reader* property_value)
{
  do
  {
    if (jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      if (az_json_token_is_text_equal(&(jr->token), component_property_label_name)
          || az_json_token_is_text_equal(&(jr->token), iot_hub_twin_desired_version))
      {
        if (az_result_failed(az_json_reader_next_token(jr)))
        {
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        continue;
      }

      *property_name = jr->token;

      if (az_result_failed(az_json_reader_next_token(jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      *property_value = *jr;

      return AZ_OK;
    }

    if (jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      break;
    }
  } while (az_result_succeeded(az_json_reader_next_token(jr)));

  return AZ_OK;
}

// Move reader to the value of property name
static az_result json_child_token_move(az_json_reader* jr, az_span property_name)
{
  while (az_result_succeeded(az_json_reader_next_token(jr)))
  {
    if ((jr->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
        && az_json_token_is_text_equal(&(jr->token), property_name))
    {
      if (az_result_failed(az_json_reader_next_token(jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      return AZ_OK;
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(jr)))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

// Check if the component name is in the model
static az_result is_component_in_model(
    az_iot_pnp_client const* client,
    az_json_token* component_name)
{
  int32_t index = 0;

  while (index < client->_internal.options.component_names_size)
  {
    if (az_json_token_is_text_equal(component_name, *client->_internal.options.component_names[index]))
    {
      return AZ_OK;
    }

    index++;
  }

  return AZ_ERROR_UNEXPECTED_CHAR;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_get_next_component(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    bool is_partial,
    az_json_token* out_component_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_reader);
  _az_PRECONDITION_NOT_NULL(out_component_name);

  az_json_reader copy_json_reader;
  int32_t version;

  // End of components
  if (json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT)
  {
    return AZ_IOT_END_OF_COMPONENTS;
  }
  else if (json_reader->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    // Skip version if match
    if (az_json_token_is_text_equal(&json_reader->token, iot_hub_twin_desired_version))
    {
      if (az_result_failed(az_json_reader_next_token(json_reader))
          || (az_result_failed(az_json_reader_next_token(json_reader))))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    // At the end of get payload "desired" section
    if (!is_partial && az_json_token_is_text_equal(&(json_reader->token), iot_hub_twin_reported))
    {
      return AZ_IOT_END_OF_COMPONENTS;
    }

    // End of components
    if (json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      return AZ_IOT_END_OF_COMPONENTS;
    }

    if (az_result_succeeded(is_component_in_model(client, &json_reader->token)))
    {
      *out_component_name = json_reader->token;
      if (az_result_failed(az_json_reader_next_token(json_reader))
          || (json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
          || (az_result_failed(az_json_reader_next_token(json_reader))))
      {
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
      return AZ_OK;
    }
    else
    {
      return AZ_IOT_ITEM_NOT_COMPONENT;
    }
  }

  _az_RETURN_IF_FAILED(az_json_reader_next_token(json_reader));

  if (!is_partial && az_result_failed(json_child_token_move(json_reader, iot_hub_twin_desired)))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  copy_json_reader = *json_reader;
  if (az_result_failed(json_child_token_move(&copy_json_reader, iot_hub_twin_desired_version))
      || az_result_failed(az_json_token_get_int32(&(copy_json_reader.token), (int32_t*)&version)))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  while (az_result_succeeded(az_json_reader_next_token(json_reader)))
  {
    if (json_reader->token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      if (az_json_token_is_text_equal(&(json_reader->token), iot_hub_twin_desired_version))
      {
        if (az_result_failed(az_json_reader_next_token(json_reader)))
        {
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        continue;
      }

      if (az_result_succeeded(is_component_in_model(client, &json_reader->token)))
      {
        *out_component_name = json_reader->token;
        if (az_result_failed(az_json_reader_next_token(json_reader))
            || (json_reader->token.kind != AZ_JSON_TOKEN_BEGIN_OBJECT)
            || (az_result_failed(az_json_reader_next_token(json_reader))))
        {
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        return AZ_OK;
      }
    }
    else if (json_reader->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(json_reader)))
      {
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

AZ_NODISCARD az_result az_iot_pnp_client_twin_get_next_component_property(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    az_json_token* out_property_name,
    az_json_reader* out_property_value)
{
  (void)client;

  // At the end if is a closing object
  if (json_reader->token.kind == AZ_JSON_TOKEN_END_OBJECT)
  {
    if (az_result_failed(az_json_reader_next_token(json_reader)))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    return AZ_IOT_END_OF_PROPERTIES;
  }
  if (az_result_failed(
          visit_component_properties(json_reader, out_property_name, out_property_value)))
  {
    return AZ_ERROR_UNEXPECTED_CHAR;
  }
  else
  {
    // Advance to next property
    if (az_result_failed(az_json_reader_next_token(json_reader)))
    {
      return AZ_ERROR_UNEXPECTED_CHAR;
    }
    return AZ_OK;
  }
}
