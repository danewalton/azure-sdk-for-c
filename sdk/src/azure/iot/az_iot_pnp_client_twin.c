// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/internal/az_precondition_internal.h>

static const az_span iot_hub_twin_desired = AZ_SPAN_LITERAL_FROM_STR("desired");
static const az_span iot_hub_twin_desired_version = AZ_SPAN_LITERAL_FROM_STR("$version");
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
  AZ_RETURN_IF_FAILED(az_iot_hub_client_twin_parse_received_topic(
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

  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(json_writer, component_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(json_writer));
  AZ_RETURN_IF_FAILED(
      az_json_writer_append_property_name(json_writer, component_property_label_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(json_writer, component_property_label_value));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_pnp_client_twin_property_end_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_writer);
  _az_PRECONDITION_VALID_SPAN(component_name, 1, false);

  (void)client;
  (void)component_name;

  return az_json_writer_append_end_object(json_writer);
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
        IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

      return AZ_OK;
    }
    else if (jr->token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to skip child of complex object.");
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

AZ_NODISCARD az_result az_iot_pnp_client_twin_property_read(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    az_json_token* ref_component_name,
    az_json_token* out_property_name,
    az_json_reader* out_property_value)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(json_reader);
  _az_PRECONDITION_NOT_NULL(ref_component_name);
  _az_PRECONDITION_NOT_NULL(out_property_name);
  _az_PRECONDITION_NOT_NULL(out_property_value);

  az_json_reader jr;
  az_json_reader copy_jr;
  int32_t version;
  int32_t index;

  AZ_RETURN_IF_FAILED(az_json_reader_init(&jr, twin_message_span, NULL));
  AZ_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

  if (!is_partial && az_result_failed(json_child_token_move(&jr, iot_hub_twin_desired)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get desired property.");
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  copy_jr = jr;
  if (az_result_failed(json_child_token_move(&copy_jr, iot_hub_twin_desired_version))
      || az_result_failed(az_json_token_get_int32(&(copy_jr.token), (int32_t*)&version)))
  {
    IOT_SAMPLE_LOG_ERROR("Failed to get version.");
    return AZ_ERROR_UNEXPECTED_CHAR;
  }

  az_json_token property_name;

  while (az_result_succeeded(az_json_reader_next_token(&jr)))
  {
    if (jr.token.kind == AZ_JSON_TOKEN_PROPERTY_NAME)
    {
      if (az_json_token_is_text_equal(&(jr.token), iot_hub_twin_desired_version))
      {
        if (az_result_failed(az_json_reader_next_token(&jr)))
        {
          IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
        continue;
      }

      property_name = jr.token;

      if (az_result_failed(az_json_reader_next_token(&jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to get next token.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }

        if (az_result_failed(visit_component_properties(
                *components_ptr[index], &jr, version, property_callback, context_ptr)))
        {
          IOT_SAMPLE_LOG_ERROR("Failed to visit component properties.");
          return AZ_ERROR_UNEXPECTED_CHAR;
        }
      else
      {
        property_callback(AZ_SPAN_NULL, &property_name, jr, version);
      }
    }
    else if (jr.token.kind == AZ_JSON_TOKEN_BEGIN_OBJECT)
    {
      if (az_result_failed(az_json_reader_skip_children(&jr)))
      {
        IOT_SAMPLE_LOG_ERROR("Failed to skip children of object.");
        return AZ_ERROR_UNEXPECTED_CHAR;
      }
    }
    else if (jr.token.kind == AZ_JSON_TOKEN_END_OBJECT)
    {
      break;
    }
  }

  return AZ_OK;
}
