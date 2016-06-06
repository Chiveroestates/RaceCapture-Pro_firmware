/*
 * Race Capture Firmware
 *
 * Copyright (C) 2016 Autosport Labs
 *
 * This file is part of the Race Capture firmware suite
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details. You should
 * have received a copy of the GNU General Public License along with
 * this code. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ESP8266_H_
#define _ESP8266_H_

#include "at.h"
#include "cpp_guard.h"
#include "dateTime.h"
#include "net/protocol.h"
#include "serial.h"
#include <stdbool.h>

CPP_GUARD_BEGIN

#define ESP8266_SSID_LEN_MAX	24
#define ESP8266_MAC_LEN_MAX	18
#define ESP8266_IPV4_LEN_MAX	16
#define ESP8266_PASSWD_LEN_MAX	24

void esp8266_do_loop(const size_t timeout);

/**
 * The various initialization states of the device.  Probably should
 * get put in a more generic file.  Here is good for now.
 */
enum dev_init_state {
        DEV_INIT_STATE_NOT_READY = 0,
        DEV_INIT_INITIALIZING,
        DEV_INIT_STATE_READY,
        DEV_INIT_STATE_FAILED,
};

/**
 * Called whenever the device informs us that the state of the
 * wifi client has changed.
 */
typedef void client_state_changed_cb_t(const char* msg);

/**
 * An action taken by the ESP8266 on a socket.
 */
enum socket_action {
        SOCKET_ACTION_UNKNOWN,
        SOCKET_ACTION_DISCONNECT,
        SOCKET_ACTION_CONNECT,
};

/**
 * Called whenever the device informs us that the state of a socket
 * has changed.
 */
typedef void socket_state_changed_cb_t(const size_t chan_id,
                                       const enum socket_action action);

/**
 * Called whenever we receive new data on a socket.
 */
typedef void data_received_cb_t(int chan_id, size_t len, const char* data);

struct esp8266_event_hooks {
        client_state_changed_cb_t *client_state_changed_cb;
        socket_state_changed_cb_t *socket_state_changed_cb;
        data_received_cb_t *data_received_cb;
};

bool esp8266_register_callbacks(const struct esp8266_event_hooks* hooks);

bool esp8266_init(struct Serial *s, const size_t max_cmd_len,
                  void (*cb)(enum dev_init_state));

enum dev_init_state esp8266_get_dev_init_state();

/**
 * These are the AT codes for the mode of the device represented in enum
 * form.  Do not change them unless you know what you are doing.
 */
enum esp8266_op_mode {
        ESP8266_OP_MODE_UNKNOWN = 0, /* Default value. */
        ESP8266_OP_MODE_CLIENT  = 1,
        ESP8266_OP_MODE_AP      = 2,
        ESP8266_OP_MODE_BOTH    = 3, /* Client & AP */
};

bool esp8266_set_op_mode(const enum esp8266_op_mode mode,
                         void (*cb)(bool status));

bool esp8266_get_op_mode(void (*cb)(bool, enum esp8266_op_mode));

struct esp8266_client_info {
        bool has_ap;
        char ssid[ESP8266_SSID_LEN_MAX];
        char mac[ESP8266_MAC_LEN_MAX];
};

void esp8266_log_client_info(const struct esp8266_client_info *info);

bool esp8266_join_ap(const char* ssid, const char* pass, void (*cb)(bool));

bool esp8266_get_client_ap(void (*cb)
                           (bool, const struct esp8266_client_info*));

struct esp8266_ipv4_info {
        char address[ESP8266_IPV4_LEN_MAX];
};

typedef void get_ip_info_cb_t(const bool status,
                              const struct esp8266_ipv4_info* client,
                              const struct esp8266_ipv4_info* station);

bool esp8266_get_ip_info(get_ip_info_cb_t callback);

enum esp8266_encryption {
        ESP8266_ENCRYPTION_INVALID = -1,
        ESP8266_ENCRYPTION_NONE = 0,
        ESP8266_ENCRYPTION_WEP = 1, /* Support deprecated */
        ESP8266_ENCRYPTION_WPA_PSK = 2,
        ESP8266_ENCRYPTION_WPA2_PSK = 3,
        ESP8266_ENCRYPTION_WPA_WPA2_PSK = 4,
        __ESP8266_ENCRYPTION_MAX, /* Always the last value */
};

struct esp8266_ap_info {
        char ssid[ESP8266_SSID_LEN_MAX];
        char password[ESP8266_PASSWD_LEN_MAX];
        size_t channel;
        enum esp8266_encryption encryption;
};

typedef void esp8266_get_ap_info_cb_t(const bool status,
                                      const struct esp8266_ap_info* info);

bool esp8266_get_ap_info(esp8266_get_ap_info_cb_t *cb);

typedef void esp8266_set_ap_info_cb_t(const bool status);

bool esp8266_set_ap_info(const struct esp8266_ap_info* info,
                         esp8266_set_ap_info_cb_t *cb);


bool esp8266_connect(const int chan_id, const enum protocol proto,
                     const char *ip_addr, const int dest_port,
                     void (*cb) (bool, const int));

bool esp8266_send_data(const int chan_id, struct Serial *data,
                       const size_t len, void (*cb)(int));

bool esp8266_send_serial(const int chan_id, struct Serial *serial,
                         const size_t len, void (*cb)(bool));

/**
 * These ENUM values match the AT values needed for the command.  Do not
 * change them unless you know what you are doing.
 */
enum esp8266_server_action {
        ESP8266_SERVER_ACTION_DELETE = 0,
        ESP8266_SERVER_ACTION_CREATE = 1,
};

bool esp8266_server_cmd(const enum esp8266_server_action action, int port,
                        void (*cb)(bool));

typedef void esp8266_soft_reset_cb_t(const bool status);

bool esp8266_soft_reset(esp8266_soft_reset_cb_t* cb);
CPP_GUARD_END

#endif /* _ESP8266_H_ */
