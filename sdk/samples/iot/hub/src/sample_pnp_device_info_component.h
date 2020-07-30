// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

/**
 * @brief Get the payload to send for device info
 *
 * @param payload_span The span into which the payload will be deposited.
 * @param out_payload_span The output span which the span properties will be deposited.
 * @param topic The `char` pointer to place the topic to which to send the message.
 * @param topic_len The size of the buffer pointed to by `topic`.
 * @param out_topic_len The optional output topic length.
 */
az_result sample_pnp_device_info_get_report_data(
    az_iot_hub_client* client,
    az_span request_id,
    az_span payload_span,
    az_span* out_payload_span,
    char* topic,
    size_t topic_len,
    size_t* out_topic_len);
