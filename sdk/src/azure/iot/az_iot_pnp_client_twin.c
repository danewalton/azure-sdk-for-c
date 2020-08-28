// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/internal/az_precondition_internal.h>

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

AZ_NODISCARD az_result az_iot_pnp_twin_property_read(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    az_json_token* ref_component_name,
    az_json_token* out_property_name,
    az_json_reader* out_property_value)
{

  return AZ_OK;
}
