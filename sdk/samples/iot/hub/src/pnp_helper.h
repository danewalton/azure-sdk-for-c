// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PNP_HELPER_H
#define _az_PNP_HELPER_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

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
* @brief Build a reported property of type double
*
* @param[in] json_buffer The span into which the json payload will be placed.
* @param[in] component_name The name of the component for the reported_property.
* @param[in] property_name The name of the property to which to send an update.
* @param[in] property_value_double The value of the property as a double.
* @param[out] out_span The #az_span pointer to the output json payload.
*/
az_result pnp_helper_create_reported_property_double(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    double property_value_double);

/**
* @brief Build a reported property of type double
*
* @param[in] json_buffer The span into which the json payload will be placed.
* @param[in] component_name The name of the component for the reported_property.
* @param[in] property_name The name of the property to which to send an update.
* @param[in] property_value_string The value of the property as a string.
* @param[out] out_span The #az_span pointer to the output json payload.
*/
az_result pnp_helper_create_reported_property_string(
    az_span json_buffer,
    az_span component_name,
    az_span property_name,
    az_span property_value_string);

// az_result pnp_helper_
#endif // _az_PNP_HELPER_H
