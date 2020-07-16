// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PNP_HELPER_H
#define _az_PNP_HELPER_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define PNP_STATUS_SUCCESS 200
#define PNP_STATUS_BAD_FORMAT 400
#define PNP_STATUS_NOT_FOUND  404
#define PNP_STATUS_INTERNAL_ERROR 500

/**
 * @brief Gets the MQTT topic that must be used for device to cloud telemetry messages.
 * @remark Telemetry MQTT Publish messages must have QoS At least once (1).
 * @remark This topic can also be used to set the MQTT Will message in the Connect message.
 *
 * @param[in] client The #az_iot_hub_client to use for this call.
 * @param[in] properties An optional #az_iot_hub_client_properties object (can be NULL).
 * @param[in] component_name An optional component name if the telemetry is being sent from a
 *                           sub-component.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
az_result pnp_helper_get_telemetry_topic(
    az_iot_hub_client const* client,
    az_iot_hub_client_properties* properties,
    az_span component_name,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/**
 * @brief Parse a JSON payload for a PnP component command
 *
 * @param[in] json_payload Input JSON payload containing the details for the component command.
 * @param[out]
 */
az_result pnp_helper_parse_command_name(
    char* component_command,
    int32_t component_command_size,
    az_span* component_name,
    az_span* pnp_command_name);

/**
 * @brief Build a reported property
 *
 * @param[in] json_buffer The span into which the json payload will be placed.
 * @param[in] component_name The name of the component for the reported_property.
 * @param[in] property_name The name of the property to which to send an update.
 * @param[in] property_json_value The value of the property as valid JSON.
 * @param[out] out_span The #az_span pointer to the output json payload.
 */
az_result pnp_helper_create_reported_property(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    az_span property_json_value,
    az_span* out_span);

/**
 * @brief Build a reported property
 *
 * @param[in] json_buffer The span into which the json payload will be placed.
 * @param[in] component_name The name of the component for the reported_property.
 * @param[in] property_name The name of the property to which to send an update.
 * @param[in] property_json_value The value of the property as valid JSON.
 * @param[in] ack_value The return value for the reported property.
 * @param[in] ack_version The ack version for the reported property.
 * @param[in] ack_description The optional description for the reported property.
 * @param[out] out_span The #az_span pointer to the output json payload.
 */
az_result pnp_helper_create_reported_property_with_status(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    az_span property_json_value,
    int32_t ack_value,
    int32_t ack_version,
    az_span ack_description);


#endif // _az_PNP_HELPER_H
