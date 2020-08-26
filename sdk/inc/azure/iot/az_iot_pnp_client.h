// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_pnp_client.h
 *
 * @brief Definition for the Azure IoT PnP device SDK.
 */

#ifndef _az_IOT_PNP_CLIENT_H
#define _az_IOT_PNP_CLIENT_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/iot/az_iot_hub_client.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Azure IoT PnP Client options.
 *
 */
typedef struct
{
  az_span module_id; /**< The module name (if a module identity is used). */
  az_span user_agent; /**< The user-agent is a formatted string that will be used for Azure IoT
                         usage statistics. */
  az_span model_id; /**< The model id used to identify the capabilities of a device based on the
                       Digital Twin document */
} az_iot_pnp_client_options;

/**
 * @brief Azure IoT PnP Client.
 *
 */
typedef struct
{
  struct
  {
    az_iot_hub_client iot_hub_client;
    az_iot_pnp_client_options options;
  } _internal;
} az_iot_pnp_client;

/**
 * @brief Gets the default Azure IoT PnP Client options.
 * @details Call this to obtain an initialized #az_iot_pnp_client_options structure that can be
 *          afterwards modified and passed to #az_iot_pnp_client_init.
 *
 * @return #az_iot_pnp_client_options.
 */
AZ_NODISCARD az_iot_pnp_client_options az_iot_pnp_client_options_default();

/**
 * @brief Initializes an Azure IoT PnP Client.
 *
 * @param[out] client The #az_iot_pnp_client to use for this call.
 * @param[in] iot_hub_hostname The IoT Hub Hostname.
 * @param[in] device_id The Device ID.
 * @param[in] model_id The root interface of the #az_iot_pnp_client.
 * @param[in] options A reference to an #az_iot_pnp_client_options structure. Can be NULL.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_init(
    az_iot_pnp_client* client,
    az_span iot_hub_hostname,
    az_span device_id,
    az_span model_id,
    az_iot_pnp_client_options const* options);

/**
 * @brief The HTTP URI Path necessary when connecting to IoT Hub using WebSockets.
 */
#define AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH "/$iothub/websocket"

/**
 * @brief The HTTP URI Path necessary when connecting to IoT Hub using WebSockets without an X509
 * client certificate.
 * @remark Most devices should use #AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH. This option is available for
 * devices not using X509 client certificates that fail to connect to IoT Hub.
 */
#define AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH_NO_X509_CLIENT_CERT \
  AZ_IOT_PNP_CLIENT_WEB_SOCKET_PATH "?iothub-no-client-cert=true"

/**
 * @brief Gets the MQTT user name.
 *
 * The user name will be of the following format:
 * {iothubhostname}/{device_id}/?api-version=2018-06-30&{user_agent}&digital-twin-model-id={model_id}
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[out] mqtt_user_name A buffer with sufficient capacity to hold the MQTT user name.
 *                            If successful, contains a null-terminated string with the user name
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_user_name_size The size, in bytes of \p mqtt_user_name.
 * @param[out] out_mqtt_user_name_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      \p mqtt_user_name. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_user_name(
    az_iot_pnp_client const* client,
    char* mqtt_user_name,
    size_t mqtt_user_name_size,
    size_t* out_mqtt_user_name_length)
{
  return az_iot_hub_client_get_user_name(
      &(client->_internal.iot_hub_client),
      mqtt_user_name,
      mqtt_user_name_size,
      out_mqtt_user_name_length);
}

/**
 * @brief Gets the MQTT client id.
 *
 * The client id will be of the following format:
 * {device_id}
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[out] mqtt_client_id A buffer with sufficient capacity to hold the MQTT client id.
 *                            If successful, contains a null-terminated string with the client id
 *                            that needs to be passed to the MQTT client.
 * @param[in] mqtt_client_id_size The size, in bytes of \p mqtt_client_id.
 * @param[out] out_mqtt_client_id_length __[nullable]__ Contains the string length, in bytes, of
 *                                                      of \p mqtt_client_id. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_client_id(
    az_iot_pnp_client const* client,
    char* mqtt_client_id,
    size_t mqtt_client_id_size,
    size_t* out_mqtt_client_id_length)
{
  return az_iot_hub_client_get_client_id(
      &client->_internal.iot_hub_client,
      mqtt_client_id,
      mqtt_client_id_size,
      out_mqtt_client_id_length);
}

/*
 *
 * SAS Token APIs
 *
 *   Use the following APIs when the Shared Access Key is available to the application or stored
 *   within a Hardware Security Module. The APIs are not necessary if X509 Client Certificate
 *   Authentication is used.
 */

/**
 * @brief Gets the Shared Access clear-text signature.
 * @details The application must obtain a valid clear-text signature using this API, sign it using
 *          HMAC-SHA256 using the Shared Access Key as password then Base64 encode the result.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] signature An empty #az_span with sufficient capacity to hold the SAS signature.
 * @param[out] out_signature The output #az_span containing the SAS signature.
 * @return #az_result
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_sas_signature(
    az_iot_pnp_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature)
{
  return az_iot_hub_client_sas_get_signature(
      &client->_internal.iot_hub_client, token_expiration_epoch_time, signature, out_signature);
}

/**
 * @brief Gets the MQTT password.
 * @remark The MQTT password must be an empty string if X509 Client certificates are used. Use this
 *         API only when authenticating with SAS tokens.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] base64_hmac_sha256_signature The Base64 encoded value of the HMAC-SHA256(signature,
 *                                         SharedAccessKey). The signature is obtained by using
 *                                         #az_iot_pnp_client_sas_signature_get.
 * @param[in] token_expiration_epoch_time The time, in seconds, from 1/1/1970.
 * @param[in] key_name The Shared Access Key Name (Policy Name). This is optional. For security
 *                     reasons we recommend using one key per device instead of using a global
 *                     policy key.
 * @param[out] mqtt_password A buffer with sufficient capacity to hold the MQTT password.
 *                           If successful, contains a null-terminated string with the password
 *                           that needs to be passed to the MQTT client.
 * @param[in] mqtt_password_size The size, in bytes of \p mqtt_password.
 * @param[out] out_mqtt_password_length __[nullable]__ Contains the string length, in bytes, of
 *                                                     \p mqtt_password. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_get_sas_password(
    az_iot_pnp_client const* client,
    az_span base64_hmac_sha256_signature,
    uint32_t token_expiration_epoch_time,
    az_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length)
{
  return az_iot_hub_client_sas_get_password(
      &client->_internal.iot_hub_client,
      base64_hmac_sha256_signature,
      token_expiration_epoch_time,
      key_name,
      mqtt_password,
      mqtt_password_size,
      out_mqtt_password_length);
}

/*
 *
 * PnP Telemetry APIs
 *
 */

/**
 * @brief Gets the MQTT topic that must be used for device to cloud telemetry messages.
 * @remark Telemetry MQTT Publish messages must have QoS At Least Once (1).
 * @remark This topic can also be used to set the MQTT Will message in the Connect message.
 *
 * Should the user want a null terminated topic string, they may allocate a buffer large enough
 * to fit the topic plus a null terminator. They must set the last byte themselves or zero
 * initialize the buffer.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] component_name An #az_span specifying the component name to publish telemetry on.
 * @param[in] properties Properties to attach to append to the topic.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_client_telemetry_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span component_name,
    az_iot_message_properties* properties,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length);

/*
 *
 * PnP Command APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to command requests.
 * @remark Commands MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_PNP_CLIENT_COMMANDS_SUBSCRIBE_TOPIC "$iothub/methods/POST/#"

/**
 * @brief A command request received from IoT Hub.
 *
 */
typedef struct
{
  az_span
      request_id; /**< The request id.
                   * @note The application must match the command request and command response. */
  az_span component; /**< The name of the component which the command was invoked for.
                      * @note Can be `AZ_SPAN_NULL` if for the root component */
  az_span name; /**< The command name. */
} az_iot_pnp_client_command_request;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_request If the message is a command request, this will contain the
 *                         #az_iot_pnp_client_command_request.
 * @return #az_result
 *         - `AZ_ERROR_IOT_TOPIC_NO_MATCH` if the topic is not matching the expected format.
 */
AZ_NODISCARD az_result az_iot_pnp_client_commands_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_command_request* out_request);

/**
 * @brief Gets the MQTT topic that must be used to respond to command requests.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] request_id The request id. Must match a received #az_iot_pnp_client_command_request
 *                       request_id.
 * @param[in] status A code that indicates the result of the command, as defined by the user.
 * @param[out] mqtt_topic A buffer with sufficient capacity to hold the MQTT topic. If
 *                        successful, contains a null-terminated string with the topic that
 *                        needs to be passed to the MQTT client.
 * @param[in] mqtt_topic_size The size, in bytes of \p mqtt_topic.
 * @param[out] out_mqtt_topic_length __[nullable]__ Contains the string length, in bytes, of
 *                                                  \p mqtt_topic. Can be `NULL`.
 * @return #az_result
 */
AZ_NODISCARD AZ_INLINE az_result az_iot_pnp_client_commands_response_get_publish_topic(
    az_iot_pnp_client const* client,
    az_span request_id,
    uint16_t status,
    char* mqtt_topic,
    size_t mqtt_topic_size,
    size_t* out_mqtt_topic_length)
{
  return az_iot_hub_client_methods_response_get_publish_topic(
      &client->_internal.iot_hub_client,
      request_id,
      status,
      mqtt_topic,
      mqtt_topic_size,
      out_mqtt_topic_length);
}

/**
 *
 * Twin APIs
 *
 */

/**
 * @brief The MQTT topic filter to subscribe to twin operation responses.
 * @remark Twin MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_HUB_CLIENT_TWIN_RESPONSE_SUBSCRIBE_TOPIC "$iothub/twin/res/#"

/**
 * @brief Gets the MQTT topic filter to subscribe to twin desired property changes.
 * @remark Twin MQTT Publish messages will have QoS At most once (0).
 */
#define AZ_IOT_HUB_CLIENT_TWIN_PATCH_SUBSCRIBE_TOPIC "$iothub/twin/PATCH/properties/desired/#"

/**
 * @brief Twin response type.
 *
 */
typedef enum
{
  AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_GET = 1,
  AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES = 2,
  AZ_IOT_PNP_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES = 3,
} az_iot_pnp_client_twin_response_type;

/**
 * @brief Twin response.
 *
 */
typedef struct
{
  az_iot_pnp_client_twin_response_type response_type; /**< Twin response type. */
  az_iot_status status; /**< The operation status. */
  az_span
      request_id; /**< Request ID matches the ID specified when issuing a Get or Patch command. */
  az_span version; /**< The Twin object version.
                    * @remark This is only returned when
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_DESIRED_PROPERTIES
                    * or
                    * response_type==AZ_IOT_CLIENT_TWIN_RESPONSE_TYPE_REPORTED_PROPERTIES. */
} az_iot_pnp_client_twin_response;

/**
 * @brief Attempts to parse a received message's topic.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] received_topic An #az_span containing the received topic.
 * @param[out] out_twin_response If the message is twin-operation related, this will contain the
 *                         #az_iot_pnp_client_twin_response.
 * @return #az_result
 *         - `AZ_ERROR_IOT_TOPIC_NO_MATCH` if the topic is not matching the expected format.
 */
AZ_NODISCARD az_result az_iot_pnp_client_twin_parse_received_topic(
    az_iot_pnp_client const* client,
    az_span received_topic,
    az_iot_pnp_client_twin_response* out_twin_response);

/**
 * @brief Append the necessary characters to a JSON payload to begin a twin component.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] The #az_json_writer to append the necessary characters for an IoT Plug and Play
 * component.
 * @param [in] component_name The component name to begin.
 *
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_twin_property_begin_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name);

/**
 * @brief Append the necessary characters to a JSON payload to end a twin component.
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in,out] The #az_json_writer to append the necessary characters for an IoT Plug and Play
 * component.
 * @param [in] component_name The component name to end.
 *
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_twin_property_end_component(
    az_iot_pnp_client const* client,
    az_json_writer* json_writer,
    az_span component_name);

/**
 * @brief Read the IoT Plug and Play twin properties component by component
 *
 * @param[in] client The #az_iot_pnp_client to use for this call.
 * @param[in] json_reader The #az_json_reader to parse through.
 * @param[in/out] ref_component_name The #az_json_token* which is changed only if a new component name is
 * found.
 * @param[out] out_property_value The #az_json_reader* representing the value of the property.
 *
 * @return #az_result
 */
AZ_NODISCARD az_result az_iot_pnp_twin_property_read(
    az_iot_pnp_client const* client,
    az_json_reader* json_reader,
    az_json_token* ref_component_name,
    az_json_token* out_property_name,
    az_json_reader* out_property_value);
// This could function like the one for the Azure SDK for C if the ref_component_name is effectively
// a ref_component_name. The user would have to pass an empty span pointer at which point the first
// component would populate the span. The user would have to pass back the pointer to us and we
// would change it only if the component changes. The ref_component_name would be set each time it
// hits an "end object" since each sub-object would be passed to the user as a json_reader. IE, in
// our land, level one would be a component name and level two is any properties and their values.
// Therefore is we kick out of a level,

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_PNP_CLIENT_H
