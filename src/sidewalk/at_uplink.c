// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: MIT-0

#include <sid_api.h>
#include <sid_error.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(at_uplink, CONFIG_TRACKER_LOG_LEVEL);

#include <asset_tracker.h>
#include <sidewalk/at_uplink.h>


void at_send_uplink(at_ctx_t *context) 
{

        at_ctx_t *at_ctx = (at_ctx_t *)context;

        static struct sid_msg msg;
        uint8_t uptype = (uint8_t) at_ctx->uplink_type;
        sid_error_t sid_ret = SID_ERROR_NONE;
        uint8_t payload[MAX_PAYLOAD_SIZE];
        uint8_t payload_len = 0;
        struct sid_msg_desc desc = {
                .type = SID_MSG_TYPE_NOTIFY,
                .link_type = at_ctx->at_conf.sid_link_type,
                .link_mode = SID_LINK_MODE_CLOUD,
        };
        
        // Updated payload format for AWS IoT Core Device Location service
        // No manual fragmentation needed - service handles larger payloads natively
        switch (uptype) {
                case CONFIG_T:
                        //TODO
                        break;

                case NOLOC_T:
                        payload[0] = (uptype<<6);  //Upper two bits is message type.
                        payload[1] = at_ctx->sensors.batt;
                        payload[2] = (int8_t) at_ctx->sensors.temp;
                        payload[3] = (uint8_t) at_ctx->sensors.hum;
                        payload[4] = (uint8_t) at_ctx->motion<<7;
                        payload[4] |= (uint8_t) at_ctx->sensors.peak_accel;
                        payload_len = UPLINK_MSG_HDR_SIZE;

                        LOG_HEXDUMP_DBG(payload, payload_len, "noloc_uplink_buffer");

                        msg = (struct sid_msg){ .data = payload, .size = payload_len};
                        sid_ret = sid_put_msg(at_ctx->handle, &msg, &desc);

                        if (SID_ERROR_NONE != sid_ret) {
                                LOG_ERR("failed sending message err:%d", (int)sid_ret);
                                return;
                        }
                        LOG_INF("queued NOLOC uplink message id:%u", desc.id);
                        break;

                case WIFI_T:
                        // Simplified WiFi payload - send all APs in single message
                        // AWS IoT Core Device Location handles the data natively
                        uint8_t num_aps = at_ctx->wifi_scan_results.nb_results;
                        LOG_DBG("number of wifi results available: %d", num_aps);

                        // Limit to MAX_WIFI_NB to prevent buffer overflow
                        if (num_aps > MAX_WIFI_NB) {
                                num_aps = MAX_WIFI_NB;
                                LOG_WRN("Limiting WiFi APs to %d", MAX_WIFI_NB);
                        }

                        // Build simplified payload with header + all WiFi data
                        payload[0] = (uint8_t) (uptype<<6);  //Upper two bits is message type
                        payload[1] = at_ctx->sensors.batt;
                        payload[2] = (int8_t) at_ctx->sensors.temp;
                        payload[3] = (uint8_t) at_ctx->sensors.hum;
                        payload[4] = (uint8_t) at_ctx->motion<<7;
                        payload[4] |= (uint8_t) at_ctx->sensors.peak_accel;
                        payload[5] = num_aps; // Number of WiFi APs included
                        
                        payload_len = UPLINK_MSG_HDR_SIZE + 1; // Header + AP count
                        
                        // Pack all WiFi AP data: RSSI (1 byte) + MAC (6 bytes) per AP
                        for (uint8_t i = 0; i < num_aps; i++) {
                                if (payload_len + WIFI_AP_RSSI_SIZE + WIFI_AP_ADDRESS_SIZE > MAX_PAYLOAD_SIZE) {
                                        LOG_WRN("Payload size limit reached, sent %d APs", i);
                                        payload[5] = i; // Update actual count
                                        break;
                                }
                                memcpy(payload + payload_len, &at_ctx->wifi_scan_results.rssi[i], WIFI_AP_RSSI_SIZE);
                                payload_len += WIFI_AP_RSSI_SIZE;
                                memcpy(payload + payload_len, &at_ctx->wifi_scan_results.mac[i], WIFI_AP_ADDRESS_SIZE);
                                payload_len += WIFI_AP_ADDRESS_SIZE;
                        }
                        
                        LOG_HEXDUMP_DBG(payload, payload_len, "wifi_uplink_buffer");
                        
                        msg = (struct sid_msg){ .data = payload, .size = payload_len};
                        sid_ret = sid_put_msg(at_ctx->handle, &msg, &desc);
                        if (SID_ERROR_NONE != sid_ret) {
                                LOG_ERR("failed sending WIFI message err:%d", (int)sid_ret);
                                return;
                        }
                        LOG_INF("queued WIFI message (APs: %d) - id:%u", num_aps, desc.id);
                        break;

                case GNSS_T:
                        // Simplified GNSS payload - send complete data in single message
                        // AWS IoT Core Device Location handles GNSS NAV data natively
                        LOG_DBG("GNSS nav size: %d", at_ctx->gnss_scan_results.nav_msg_size);

                        // Check if payload fits in maximum size
                        uint16_t required_size = UPLINK_MSG_HDR_SIZE + 5 + at_ctx->gnss_scan_results.nav_msg_size;
                        if (required_size > MAX_PAYLOAD_SIZE) {
                                LOG_ERR("GNSS payload too large (%d bytes), max is %d", required_size, MAX_PAYLOAD_SIZE);
                                return;
                        }

                        // Build payload: header + GNSS metadata + NAV message
                        payload[0] = (uint8_t) (uptype<<6);  //Upper two bits is message type
                        payload[1] = at_ctx->sensors.batt;
                        payload[2] = (int8_t) at_ctx->sensors.temp;
                        payload[3] = (uint8_t) at_ctx->sensors.hum;
                        payload[4] = (uint8_t) at_ctx->motion<<7;
                        payload[4] |= (uint8_t) at_ctx->sensors.peak_accel;
                        payload[5] = at_ctx->gnss_scan_results.nav_msg_size;
                        payload[6] = (uint8_t) (at_ctx->gnss_scan_results.capture_time >> 24);
                        payload[7] = (uint8_t) (at_ctx->gnss_scan_results.capture_time >> 16);
                        payload[8] = (uint8_t) (at_ctx->gnss_scan_results.capture_time >> 8);
                        payload[9] = (uint8_t) (at_ctx->gnss_scan_results.capture_time & 0xFF);

                        // Copy complete NAV message
                        memcpy(payload + UPLINK_MSG_HDR_SIZE + 5, at_ctx->gnss_scan_results.nav_msg, at_ctx->gnss_scan_results.nav_msg_size);
                        payload_len = UPLINK_MSG_HDR_SIZE + 5 + at_ctx->gnss_scan_results.nav_msg_size;
                        
                        LOG_HEXDUMP_DBG(payload, payload_len, "gnss_uplink_buffer");
                        
                        msg = (struct sid_msg){ .data = payload, .size = payload_len};
                        sid_ret = sid_put_msg(at_ctx->handle, &msg, &desc);
                        if (SID_ERROR_NONE != sid_ret) {
                                LOG_ERR("failed sending gnss message err:%d", (int)sid_ret);
                                return;
                        }
                        LOG_INF("queued GNSS message - id:%u", desc.id);
                        break;

                default:
                        LOG_ERR("Invalid uplink type!");
        }
}

void at_msg_sent(at_ctx_t *context) {
        
        at_ctx_t *at_ctx = (at_ctx_t *)context;

        // Simplified message handling - no fragmentation tracking needed
        // AWS IoT Core Device Location handles payload delivery
        LOG_INF("Message sent successfully");
        at_event_send(EVENT_UPLINK_COMPLETE);
}

void at_send_error(at_ctx_t *context) {
        at_ctx_t *at_ctx = (at_ctx_t *)context;

        // Simplified error handling
        LOG_ERR("Error sending message");
        at_event_send(EVENT_UPLINK_COMPLETE);
}

