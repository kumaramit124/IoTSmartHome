/*
 * Copyright (c) 2013 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/*  This was moved here to correct a software engineering defect.
 * These defines need to be private
 */

#include <stddef.h>

#define CONNECTOR_CONST_PROTECTION

/*  WARNING: connector_api.h must be the first connector_* header file
 * to guarantee CONNECTOR_VERSION is properly applied to all files */
#include "connector_api.h"
#include "connector_debug.h"

#include "connector_info.h"
#include "connector_def.h"

#include "chk_config.h"
#include "bele.h"

static connector_status_t notify_error_status(connector_callback_t const callback, connector_class_id_t const class_number, connector_request_id_t const request_number, connector_status_t const status);
#include "os_intf.h"
#include "connector_global_config.h"

static connector_status_t connector_stop_callback(connector_data_t * const connector_ptr, connector_transport_t const transport, void * const user_context);
#if !(defined CONNECTOR_NETWORK_TCP_START) || (defined CONNECTOR_TRANSPORT_UDP) || defined (CONNECTOR_TRANSPORT_SMS)
static connector_status_t get_config_connect_status(connector_data_t * const connector_ptr, connector_request_id_config_t const request_id, connector_config_connect_type_t * const config_ptr);
#endif

#if (defined CONNECTOR_DATA_POINTS)
#include "connector_data_point.h"
#endif

#if (defined CONNECTOR_TRANSPORT_TCP)
#include "connector_edp.h"
#endif

#if (defined CONNECTOR_SHORT_MESSAGE)
#include "connector_sm.h"
#endif

#ifdef CONNECTOR_NO_MALLOC
#include "connector_static_buffer.h"
#endif

#define DEVICE_ID_LENGTH 16


static char const connector_signature[] = CONNECTOR_SW_VERSION;

#if !(defined CONNECTOR_NETWORK_TCP_START) || (defined CONNECTOR_TRANSPORT_UDP) || defined (CONNECTOR_TRANSPORT_SMS)
static connector_status_t get_config_connect_status(connector_data_t * const connector_ptr,
                                                        connector_request_id_config_t const config_request_id,
                                                        connector_config_connect_type_t * const config_connect)
{
    connector_status_t result = connector_working;

    ASSERT(config_connect != NULL);
    ASSERT((config_request_id == connector_request_id_config_network_tcp) ||
           (config_request_id == connector_request_id_config_network_udp) ||
           (config_request_id == connector_request_id_config_network_sms));

    config_connect->type = connector_connect_auto;

    {
        connector_callback_status_t status;
        connector_request_id_t request_id;

        request_id.config_request = config_request_id;
        status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, config_connect);

        switch (status)
        {
        case connector_callback_continue:
            switch (config_connect->type)
            {
            case connector_connect_auto:
            case connector_connect_manual:
                break;
            default:
                notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, connector_invalid_data_range);
                result = connector_abort;
                break;
            }
            break;

        case connector_callback_busy:
        case connector_callback_abort:
        case connector_callback_error:
            result = connector_abort;
            break;

        case connector_callback_unrecognized:
            break;
        }
    }
    return result;
}
#endif

static connector_status_t get_wan_device_id(connector_data_t * const connector_ptr,
                                            uint8_t * const device_id,
                                            connector_request_id_config_t config_request)
{

enum {
    imei_device_id_prefix = 1,
    esn_hex_device_id_prefix,
    esn_dec_device_id_prefix,
    meid_hex_device_id_prefix,
    meid_dec_device_id_prefix
};

    connector_status_t result;

    result = get_config_wan_id(connector_ptr, config_request);
    if (result == connector_working)
    {
        uint8_t * dst = device_id + (DEVICE_ID_LENGTH - connector_ptr->wan_id_length);

#if (defined CONNECTOR_DEBUG)
        {
            char * wan_string = NULL;

            switch (connector_ptr->wan_type)
            {
            case connector_wan_type_imei:
                wan_string = "IMEI";
                break;
            case connector_wan_type_esn:
                wan_string = "ESN";
                break;
            case connector_wan_type_meid:
                wan_string = "MEID";
                break;
            }
            connector_debug_hexvalue(wan_string, connector_ptr->wan_id, connector_ptr->wan_id_length);
        }
#endif
        memcpy(dst, connector_ptr->wan_id, connector_ptr->wan_id_length);

        switch (connector_ptr->wan_type)
        {
        case connector_wan_type_imei:
            device_id[1] = imei_device_id_prefix;
            break;
        case connector_wan_type_esn:
            device_id[1] = esn_hex_device_id_prefix;
            break;
        case connector_wan_type_meid:
            device_id[1] = meid_hex_device_id_prefix;
            break;
        }
    }

    return result;
}


static connector_status_t manage_device_id(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;

    connector_ptr->connector_got_device_id = connector_false;

    result = get_config_device_id_method(connector_ptr);
    COND_ELSE_GOTO(result == connector_working, error);

    switch (connector_ptr->device_id_method)
    {
        case connector_device_id_method_manual:
            result = get_config_device_id(connector_ptr);
            COND_ELSE_GOTO(result == connector_working, error);
            {
                uint8_t const null_device_id[DEVICE_ID_LENGTH] = {0x00};
                /* If the returned Device ID is zero, Cloud Connector will ask the Device Cloud for a Device ID. */
                if (memcmp(connector_ptr->device_id, null_device_id, sizeof null_device_id) == 0)
                    connector_ptr->connector_got_device_id = connector_false;
                else
                    connector_ptr->connector_got_device_id = connector_true;
            }
            break;

        case connector_device_id_method_auto:
        {
            result = get_config_connection_type(connector_ptr);
            COND_ELSE_GOTO(result == connector_working, error);

            switch (connector_ptr->connection_type)
            {
                case connector_connection_type_lan:
                {
                    result = get_config_mac_addr(connector_ptr);
                    COND_ELSE_GOTO(result == connector_working, error);

                    connector_ptr->device_id[8] = connector_ptr->mac_addr[0];
                    connector_ptr->device_id[9] = connector_ptr->mac_addr[1];
                    connector_ptr->device_id[10] = connector_ptr->mac_addr[2];
                    connector_ptr->device_id[11] = 0xFF;
                    connector_ptr->device_id[12] = 0xFF;
                    connector_ptr->device_id[13] = connector_ptr->mac_addr[3];
                    connector_ptr->device_id[14] = connector_ptr->mac_addr[4];
                    connector_ptr->device_id[15] = connector_ptr->mac_addr[5];
                }
                break;

                case connector_connection_type_wan:
                {
                    result = get_config_wan_type(connector_ptr);
                    COND_ELSE_GOTO(result == connector_working, error);

                    switch (connector_ptr->wan_type)
                    {
                    case connector_wan_type_imei:
                        result = get_wan_device_id(connector_ptr, connector_ptr->device_id, connector_request_id_config_imei_number);
                        break;
                    case connector_wan_type_esn:
                        result = get_wan_device_id(connector_ptr, connector_ptr->device_id, connector_request_id_config_esn);
                        break;
                    case connector_wan_type_meid:
                        result = get_wan_device_id(connector_ptr, connector_ptr->device_id, connector_request_id_config_meid);
                        break;
                    }
                    break;
                }
            }
            connector_ptr->connector_got_device_id = connector_true;
            break;
        }
    }
    COND_ELSE_GOTO(result == connector_working, error);

error:
    return result;
}

static connector_status_t connector_stop_callback(connector_data_t * const connector_ptr, connector_transport_t const transport, void * const user_context)
{
    connector_status_t result = connector_working;

    connector_initiate_stop_request_t stop_request;

    connector_request_id_t request_id;
    request_id.status_request = connector_request_id_status_stop_completed;

    stop_request.transport = transport;
    stop_request.user_context = user_context;

    {
        connector_callback_status_t const status =  connector_callback(connector_ptr->callback, connector_class_id_status, request_id, &stop_request);

        switch (status)
        {
        case connector_callback_continue:
        case connector_callback_unrecognized:
            break;
        case connector_callback_busy:
            result = connector_pending;
            break;
        default:
            result = connector_abort;
            break;
        }
    }

    connector_debug_printf("connector_stop_callback: %s\n", transport_to_string(transport));
    return result;
}


#define CONNECTOR_IS_STOP(state, value)    ((state) == (value))

static connector_bool_t is_connector_stopped(connector_data_t * const connector_ptr, connector_close_status_t const close_status)
{
    int count = 0;
    connector_transport_state_t wait_state = (close_status == connector_close_status_device_stopped) ? connector_transport_idle : connector_transport_terminate;

#if (defined CONNECTOR_TRANSPORT_UDP)
    if (!connector_bool(CONNECTOR_IS_STOP(connector_ptr->sm_udp.transport.state, wait_state))) count++;
#endif

#if (defined CONNECTOR_TRANSPORT_SMS)
    if (!connector_bool(CONNECTOR_IS_STOP(connector_ptr->sm_sms.transport.state, wait_state))) count++;
#endif

#if (defined CONNECTOR_TRANSPORT_TCP)
    if (!connector_bool(CONNECTOR_IS_STOP(edp_get_active_state(connector_ptr), wait_state))) count++;
#endif

    return connector_bool(count == 0);
}

static void abort_connector(connector_data_t * const connector_ptr)
{
    switch (connector_ptr->stop.state)
    {
        case connector_state_terminate_by_initiate_action:
        case connector_state_abort_by_callback:
            /* already shutting down - nothing to do here. */
            break;

        default:
        {
            connector_status_t status;

#if (defined CONNECTOR_TRANSPORT_UDP) ||(defined CONNECTOR_TRANSPORT_SMS)
            status = sm_initiate_action(connector_ptr, connector_initiate_terminate, NULL);
            if (status != connector_success)
                connector_debug_printf("abort_connector: sm_initiate_action returns error %d\n", status);

#if (defined CONNECTOR_TRANSPORT_UDP)
            connector_ptr->sm_udp.close.status = connector_close_status_abort;
#endif

#if (defined CONNECTOR_TRANSPORT_SMS)
            connector_ptr->sm_sms.close.status = connector_close_status_abort;
#endif
#endif


#if (defined CONNECTOR_TRANSPORT_TCP)
            status = edp_initiate_action(connector_ptr, connector_initiate_terminate, NULL);
            if (status != connector_success)
                connector_debug_printf("abort_connector: edp_initiate_action returns error %d\n", status);

            edp_set_close_status(connector_ptr, connector_close_status_abort);
#endif

            connector_ptr->stop.state = connector_state_abort_by_callback;
            break;
        }
    }
}

connector_handle_t connector_init(connector_callback_t const callback)
{

    connector_data_t * connector_handle = NULL;
    connector_status_t status;

#if (defined CONNECTOR_SW_DESCRIPTION)
    connector_debug_printf("Cloud Connector v%s %s\n", CONNECTOR_SW_VERSION, CONNECTOR_SW_DESCRIPTION);
#else
    connector_debug_printf("Cloud Connector v%s\n", CONNECTOR_SW_VERSION);
#endif

    {
        void * handle;

#if (defined CONNECTOR_NO_MALLOC)
        status = malloc_data_buffer(NULL, sizeof *connector_handle, named_buffer_id(connector_data), &handle);
#else
        status = malloc_cb(callback, sizeof *connector_handle, &handle);
#endif

        COND_ELSE_GOTO(status == connector_working, done);
        memset(handle, 0x00, sizeof *connector_handle); /* Init structure, all pointers to NULL */
        connector_handle = handle;
    }

    connector_handle->callback = callback;

    status = manage_device_id(connector_handle);
    COND_ELSE_GOTO(status == connector_working, error);

    /* make a copy of the cloud url */
#if (defined CONNECTOR_TRANSPORT_TCP) || (defined CONNECTOR_TRANSPORT_UDP)
#if (defined CONNECTOR_CLOUD_URL)
    {
        static char const connector_device_cloud_url[]= CONNECTOR_CLOUD_URL;
        connector_handle->device_cloud_url = (char *)connector_device_cloud_url;
        connector_handle->device_cloud_url_length = sizeof connector_device_cloud_url -1;
    }
#else
    status = get_config_device_cloud_url(connector_handle);
    COND_ELSE_GOTO(status == connector_working, error);
#endif
#endif /* (defined CONNECTOR_TRANSPORT_TCP) || (defined CONNECTOR_TRANSPORT_UDP) */

    /* make a copy of the cloud phone */
#if (defined CONNECTOR_TRANSPORT_SMS)
#if (defined CONNECTOR_CLOUD_PHONE)
    {
        static char const connector_device_cloud_phone[]= CONNECTOR_CLOUD_PHONE;
        connector_handle->device_cloud_phone = (char *)connector_device_cloud_phone;
        connector_handle->device_cloud_phone_length = sizeof connector_device_cloud_phone -1;
    }
#else
    status = get_config_device_cloud_phone(connector_handle);
    COND_ELSE_GOTO(status == connector_working, error);
#endif

#if (defined CONNECTOR_CLOUD_SERVICE_ID)
    {
        static char const connector_device_cloud_service_id[]= CONNECTOR_CLOUD_SERVICE_ID;
        connector_handle->device_cloud_service_id = (char *)connector_device_cloud_service_id;
        connector_handle->device_cloud_service_id_length = sizeof connector_device_cloud_service_id -1;
    }
#else
    status = get_config_device_cloud_service_id(connector_handle);
    COND_ELSE_GOTO(status == connector_working, error);
#endif
#endif /* (defined CONNECTOR_TRANSPORT_SMS) */

#if (defined CONNECTOR_TRANSPORT_TCP)
    status = connector_edp_init(connector_handle);
    COND_ELSE_GOTO(status == connector_working, error);
#endif

#if (defined CONNECTOR_TRANSPORT_UDP) || (defined CONNECTOR_TRANSPORT_SMS)
    status = connector_sm_init(connector_handle);
    COND_ELSE_GOTO(status == connector_working, error);
#endif

#if (defined CONNECTOR_TRANSPORT_COUNT > 1)
    connector_handle->first_running_network = (connector_network_type_t) 0;
#endif

    connector_handle->signature = connector_signature;
    goto done;

error:
    free_data_buffer(connector_handle, named_buffer_id(connector_data), connector_handle);
    connector_handle = NULL;

done:
    return connector_handle;
}


connector_status_t connector_step(connector_handle_t const handle)
{
    connector_status_t result = connector_init_error;
    connector_data_t * const connector_ptr = handle;

    ASSERT_GOTO(handle != NULL, done);
    if (connector_ptr->signature != connector_signature) goto done;

    switch (connector_ptr->stop.state)
    {
        case connector_state_running:
            break;
        case connector_state_stop_by_initiate_action:
            if (is_connector_stopped(connector_ptr, connector_close_status_device_stopped))
            {
                result = connector_stop_callback(connector_ptr, connector_transport_all, connector_ptr->stop.user_context);
                if (result == connector_abort)
                {
                    goto error;
                }
                else if (result == connector_working)
                {
                    connector_ptr->stop.state = connector_state_running;
                }
            }
            break;
        case connector_state_abort_by_callback:
        case connector_state_terminate_by_initiate_action:
            if (is_connector_stopped(connector_ptr, connector_close_status_device_terminated))
            {
                connector_ptr->connector_got_device_id = connector_false; /* TODO, Probably this should not be done with provisioning! */
                connector_ptr->signature = NULL;
                free_data_buffer(connector_ptr, named_buffer_id(connector_data), connector_ptr);
                connector_debug_printf("connector_step: free Cloud Connector\n");

                result = (connector_ptr->stop.state == connector_state_terminate_by_initiate_action) ? connector_device_terminated : connector_abort;
                goto done;
            }
            break;
        default:
            break;
    }

#if (CONNECTOR_TRANSPORT_COUNT == 1)
#if (defined CONNECTOR_TRANSPORT_TCP)
    result = connector_edp_step(connector_ptr);
#endif
#if (defined CONNECTOR_TRANSPORT_UDP)
    result = connector_udp_step(connector_ptr);
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
    result = connector_sms_step(connector_ptr);
#endif

#else
    {
#define next_network(current_network) (((current_network) == last_network_index) ? first_network_index : (connector_network_type_t) ((current_network) + 1))

        connector_network_type_t const first_network_index = (connector_network_type_t) 0;
        connector_network_type_t const last_network_index = (connector_network_type_t) (connector_network_count - 1);
        connector_network_type_t const first_network_checked = connector_ptr->first_running_network;
        connector_network_type_t current_network = first_network_checked;

        do
        {
            connector_status_t (*step_func)(connector_data_t * const connector_ptr);

            switch (current_network)
            {
#if (defined CONNECTOR_TRANSPORT_TCP)
            case connector_network_tcp: step_func = connector_edp_step; break;
#endif
#if (defined CONNECTOR_TRANSPORT_UDP)
            case connector_network_udp: step_func = connector_udp_step; break;
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
            case connector_network_sms: step_func = connector_sms_step; break;
#endif
            default:
                ASSERT(connector_false);
                result = connector_abort;
                goto error; break;
            }

            result = step_func(connector_ptr);

            current_network = next_network(current_network);
        } while ((current_network != first_network_checked) && (result == connector_idle));

        connector_ptr->first_running_network = (result == connector_idle) ? next_network(first_network_checked): current_network;

#undef next_network
    }
#endif

error:
    switch (result)
    {
    case connector_abort:
        abort_connector(connector_ptr);
        /* no break; */
    case connector_device_terminated:
        result = connector_working;
        break;
    default:
        break;
    }

done:
    return result;
}

connector_status_t connector_run(connector_handle_t const handle)
{
    connector_status_t rc;

    do {
        rc = connector_step(handle);

        if (rc == connector_idle || rc == connector_working || rc == connector_pending || rc == connector_active || rc == connector_success)
        {
            if (yield_process(handle, rc) != connector_working)
            {
                abort_connector(handle);
                rc = connector_success;
            }
        }

    } while (rc == connector_idle || rc == connector_working || rc == connector_pending || rc == connector_active || rc == connector_success);

    return rc;
}

connector_status_t connector_initiate_action(connector_handle_t const handle, connector_initiate_request_t const request, void const * const request_data)
{
    connector_status_t result = connector_init_error;
    connector_data_t * connector_ptr = (connector_data_t *)handle;

    ASSERT_GOTO(handle != NULL, done);

    if (connector_ptr->signature != connector_signature) goto done;

    switch (request)
    {
    case connector_initiate_terminate:

#if (defined CONNECTOR_TRANSPORT_TCP)
        result = edp_initiate_action(connector_ptr, request, request_data);
        COND_ELSE_GOTO(result == connector_success, done);
#endif

#if (defined CONNECTOR_SHORT_MESSAGE)
        result = sm_initiate_action(connector_ptr, request, request_data);
        COND_ELSE_GOTO(result == connector_success, done);
#endif

        connector_ptr->stop.state = connector_state_terminate_by_initiate_action;
        result = connector_success;
        break;

   default:
        if (connector_ptr->stop.state == connector_state_terminate_by_initiate_action)
        {
            result = connector_device_terminated;
            goto done;
        }

        {
            connector_transport_t const * const transport = request_data;

            result = connector_unavailable;

            if (transport == NULL)
            {
                result = connector_invalid_data;
                goto done;
            }

            switch (*transport)
            {
            case connector_transport_all:
                if (request != connector_initiate_transport_stop)
                {
                    result = connector_invalid_data;
                    goto done;
                }

                if (connector_ptr->stop.state != connector_state_running)
                {
                    /* already in close state */
                    result = (connector_ptr->stop.state == connector_state_terminate_by_initiate_action) ? connector_device_terminated: connector_service_busy;
                    goto done;
                }

                /* no break */

#if (defined CONNECTOR_SHORT_MESSAGE)
#if (defined CONNECTOR_TRANSPORT_UDP)
            case connector_transport_udp:
#endif
#if (defined CONNECTOR_TRANSPORT_SMS)
            case connector_transport_sms:
#endif
                result = sm_initiate_action(connector_ptr, request, request_data);

                if (*transport != connector_transport_all)  break;
                else if (result != connector_success) break;
                else if (request != connector_initiate_transport_stop) break;
                /* no break; */
#endif

#if (defined CONNECTOR_TRANSPORT_TCP)
            case connector_transport_tcp:
                result = edp_initiate_action(connector_ptr, request, request_data);

                if (*transport != connector_transport_all)  break;
                else if (result != connector_success) break;
                else if (request != connector_initiate_transport_stop) break;
#endif
                {
                    connector_initiate_stop_request_t const * const stop_request = request_data;
                    connector_ptr->stop.condition = stop_request->condition;
                    connector_ptr->stop.user_context = stop_request->user_context;
                    connector_ptr->stop.state = connector_state_stop_by_initiate_action;
                    result = connector_success;
                }
                break;

            default:
                result = connector_invalid_data;
                goto done;
            }
        }
    }
done:
    return result;
}

