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
#include "connector_sm_utils.h"
#include "connector_sm_cmd.h"
#include "connector_sm_session.h"
#include "connector_sm_send.h"
#include "connector_sm_recv.h"

static connector_sm_data_t * get_sm_data(connector_data_t * const connector_ptr, connector_transport_t const transport)
{
    connector_sm_data_t * sm_ptr = NULL;

    switch (transport)
    {
        #if (defined CONNECTOR_TRANSPORT_UDP)
        case connector_transport_udp:
            sm_ptr = &connector_ptr->sm_udp;
            break;
        #endif

        #if (defined CONNECTOR_TRANSPORT_SMS)
        case connector_transport_sms:
            sm_ptr = &connector_ptr->sm_sms;
            break;
        #endif

        default:
            ASSERT(connector_false);
            break;
    }

    return sm_ptr;
}

static connector_status_t sm_initialize(connector_data_t * const connector_ptr, connector_transport_t const transport)
{
    connector_status_t result = connector_init_error;
    connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, transport);

    ASSERT_GOTO(sm_ptr != NULL, error);
    switch (transport)
    {
        #if (defined CONNECTOR_TRANSPORT_UDP)
        case connector_transport_udp:
        {
            size_t const sm_udp_version_length = 1;

            sm_ptr->transport.id_type = connector_sm_id_type_device_id;
            sm_ptr->transport.id = connector_ptr->device_id;
            sm_ptr->transport.id_length = DEVICE_ID_LENGTH;

            sm_ptr->network.class_id = connector_class_id_network_udp;
            sm_ptr->network.transport = connector_transport_udp;
            sm_ptr->transport.mtu = SM_PACKET_SIZE_UDP;
            sm_ptr->transport.sm_mtu_tx = sm_ptr->transport.mtu - (sm_ptr->transport.id_length + sm_udp_version_length);
            sm_ptr->transport.sm_mtu_rx = sm_ptr->transport.sm_mtu_tx;
            break;
        }
        #endif

        #if (defined CONNECTOR_TRANSPORT_SMS)
        case connector_transport_sms:
        {
            #if (defined CONNECTOR_CLOUD_SERVICE_ID)
            static uint8_t service_id[] = CONNECTOR_CLOUD_SERVICE_ID;
            size_t const service_id_length = sizeof service_id -1;
            #else
            size_t service_id_length = connector_ptr->device_cloud_service_id_length;
            char * service_id = connector_ptr->device_cloud_service_id;
            #endif

            if (service_id_length)
            {
                sm_ptr->transport.id_type = connector_sm_id_type_service_id;
                sm_ptr->transport.id = (uint8_t *)service_id;
                sm_ptr->transport.id_length = service_id_length;
            }
            else
            {
                /* No shared codes used */
                sm_ptr->transport.id_type = connector_sm_id_type_none;
                sm_ptr->transport.id_length = 0;
            }

            sm_ptr->network.class_id = connector_class_id_network_sms;
            sm_ptr->network.transport = connector_transport_sms;
            sm_ptr->transport.mtu = SM_PACKET_SIZE_SMS_ENCODED;
            {
                if ((sm_ptr->transport.id != NULL) && (sm_ptr->transport.id_length > 0))
                {
                    /* Preamble is NOT encoded85, so for a service-id like 'idgp': */
                    /*
                       For Tx: 'idgp '
                               There is room for 160-5=155 not encoded85 characters. After encoding, that will lead to a max payload of 155*4/5=124 bytes.
                     */
                    sm_ptr->transport.sm_mtu_tx = (((sm_ptr->transport.mtu - (sm_ptr->transport.id_length + SMS_SERVICEID_WRAPPER_TX_SIZE))*4) / 5);
                    /*
                       For Rx: '(idgp):'
                               There is room for 160-7=153 not encoded85 characters. After encoding, that will lead to a max payload of 153*4/5=122 bytes.
                     */

                    sm_ptr->transport.sm_mtu_rx = (((sm_ptr->transport.mtu - (sm_ptr->transport.id_length + SMS_SERVICEID_WRAPPER_RX_SIZE))*4) / 5);
                }
                else
                {
                    sm_ptr->transport.sm_mtu_tx = ((sm_ptr->transport.mtu *4) / 5);
                    sm_ptr->transport.sm_mtu_rx = sm_ptr->transport.sm_mtu_tx;
                }
            }
            break;
        }
        #endif

        default:
            ASSERT_GOTO(connector_false, error);
            break;
    }

    sm_ptr->transport.state = connector_transport_idle;
    sm_ptr->pending.data = NULL;
    sm_ptr->session.head = NULL;
    sm_ptr->session.tail = NULL;
    sm_ptr->session.current = NULL;
    #if (defined CONNECTOR_SM_MAX_SESSIONS)
    sm_ptr->session.max_sessions = CONNECTOR_SM_MAX_SESSIONS;
    #else
    sm_ptr->session.max_sessions = 2;
    #endif
    sm_ptr->session.active_client_sessions = 0;
    sm_ptr->session.active_cloud_sessions = 0;

    #if (defined CONNECTOR_SM_MAX_RX_SEGMENTS) && (CONNECTOR_SM_MAX_RX_SEGMENTS > 1) && (!defined CONNECTOR_SM_MULTIPART)
    #error "You must define CONNECTOR_SM_MULTIPART in order to set CONNECTOR_SM_MAX_RX_SEGMENTS bigger than 1"
    #endif

    #if (defined CONNECTOR_SM_MAX_DATA_POINTS_SEGMENTS) && (CONNECTOR_SM_MAX_DATA_POINTS_SEGMENTS > 1) && (!defined CONNECTOR_SM_MULTIPART)
    #error "You must define CONNECTOR_SM_MULTIPART in order to set CONNECTOR_SM_MAX_DATA_POINTS_SEGMENTS bigger than 1"
    #endif

    #if (defined CONNECTOR_SM_MAX_RX_SEGMENTS)
    sm_ptr->session.max_segments = CONNECTOR_SM_MAX_RX_SEGMENTS;
    #else
    sm_ptr->session.max_segments = 1;
    #endif
    #if (defined CONNECTOR_SM_TIMEOUT)
    sm_ptr->timeout_in_seconds = CONNECTOR_SM_TIMEOUT;
    #else
    sm_ptr->timeout_in_seconds = SM_WAIT_FOREVER;
    #endif
    sm_ptr->network.handle = NULL;
    sm_ptr->close.status = connector_close_status_device_error;
    sm_ptr->close.callback_needed = connector_true;
    sm_ptr->close.stop_condition = connector_stop_immediately;

    switch (transport)
    {
        #if (defined CONNECTOR_TRANSPORT_UDP)
        case connector_transport_udp:
        {
#if !(defined CONNECTOR_NETWORK_UDP_START)
            {
                connector_config_connect_type_t config_connect;

                result = get_config_connect_status(connector_ptr, connector_request_id_config_network_udp, &config_connect);
                ASSERT_GOTO(result == connector_working, error);

                sm_ptr->transport.connect_type = config_connect.type;
            }
#else
            ASSERT((CONNECTOR_NETWORK_UDP_START == connector_connect_auto) || (CONNECTOR_NETWORK_UDP_START == connector_connect_manual));
            sm_ptr->transport.connect_type = CONNECTOR_NETWORK_UDP_START;
            result = connector_working;
#endif
            break;
        }
        #endif

        #if (defined CONNECTOR_TRANSPORT_SMS)
        case connector_transport_sms:
        {
#if !(defined CONNECTOR_NETWORK_SMS_START)
            {
                connector_config_connect_type_t config_connect;

                result = get_config_connect_status(connector_ptr, connector_request_id_config_network_sms, &config_connect);
                ASSERT_GOTO(result == connector_working, error);

                sm_ptr->transport.connect_type = config_connect.type;
            }
#else
            ASSERT((CONNECTOR_NETWORK_SMS_START == connector_connect_auto) || (CONNECTOR_NETWORK_SMS_START == connector_connect_manual));
            sm_ptr->transport.connect_type = CONNECTOR_NETWORK_SMS_START;
            result = connector_working;
#endif
            break;
        }
        #endif

        default:
            ASSERT_GOTO(connector_false, error);
            break;
    }

error:
    return result;
}

static connector_status_t connector_sm_init(connector_data_t * const connector_ptr)
{
    connector_status_t status;

    connector_ptr->last_request_id = SM_DEFAULT_REQUEST_ID;

    #if (defined CONNECTOR_TRANSPORT_UDP)
    status = sm_initialize(connector_ptr, connector_transport_udp);
    ASSERT_GOTO(status == connector_working, error);
    #endif

    #if (defined CONNECTOR_TRANSPORT_SMS)
    status = sm_initialize(connector_ptr, connector_transport_sms);
    ASSERT_GOTO(status == connector_working, error);
    #endif

error:
    return status;
}

#if (CONNECTOR_VERSION >= 0x02010000)
/* Return request_data's request_id field. This varies depending on the request. If this can't be done, set request_id to NULL */
static uint32_t * get_request_id_ptr(connector_initiate_request_t const request, void const * const request_data)
{
    uint32_t * request_id;
    switch (request)
    {
#if (defined CONNECTOR_DATA_POINTS)
        case connector_initiate_data_point_single:
        {
            connector_request_data_point_single_t const * const data = request_data;

            request_id = data->request_id;
            break;
        }
#if (CONNECTOR_VERSION >= 0x02020000)
        case connector_initiate_data_point_multiple:
        {
            connector_request_data_point_multiple_t const * const data = request_data;

            request_id = data->request_id;
            break;
        }
#endif
        case connector_initiate_data_point_binary:
        {
            connector_request_data_point_binary_t const * const data = request_data;

            request_id = data->request_id;
            break;
        }
#endif
#if (defined CONNECTOR_DATA_SERVICE)
        case connector_initiate_send_data:
        {
            connector_request_data_service_send_t const * const data = request_data;

            request_id = data->request_id;
            break;
         }
#endif
        case connector_initiate_ping_request:
        {
            connector_sm_send_ping_request_t const * const data = request_data;

            request_id = data->request_id;
            break;
        }
        default:
            request_id = NULL;
            break;
    }

    return request_id;
}
#endif

static connector_status_t sm_initiate_action(connector_handle_t const handle, connector_initiate_request_t const request, void const * const request_data)
{
    connector_status_t result = connector_service_busy;
    connector_data_t * const connector_ptr = (connector_data_t *)handle;
    connector_transport_t const * const transport_ptr = request_data;

    ASSERT_GOTO(handle != NULL, error);
    ASSERT_GOTO((request_data != NULL) || (request == connector_initiate_terminate), error);

    switch (request)
    {
        case connector_initiate_transport_start:
        {
            connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, *transport_ptr);

            ASSERT_GOTO(sm_ptr != NULL, error);
            switch (sm_ptr->transport.state)
            {
                case connector_transport_idle:
                case connector_transport_close:
                    break;

                case connector_transport_terminate:
                    result = connector_device_terminated;
                    goto error;

                default:
                    goto done;
            }

            if (sm_ptr->pending.data != NULL) goto error;
            sm_ptr->pending.data = &sm_ptr->close.stop_condition; /* dummy */
            sm_ptr->pending.request = request;
            break;
        }

        case connector_initiate_terminate:
            if (transport_ptr == NULL)
            {
                connector_transport_t transport;

                #if (defined CONNECTOR_TRANSPORT_UDP)
                transport = connector_transport_udp;
                result = sm_initiate_action(handle, request, &transport); /* intended recursive */
                if (result != connector_success) goto error;
                #endif

                #if (defined CONNECTOR_TRANSPORT_SMS)
                transport = connector_transport_sms;
                result = sm_initiate_action(handle, request, &transport); /* intended recursive */
                if (result != connector_success) goto error;
                #endif
            }
            else
            {
                connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, *transport_ptr);

                ASSERT_GOTO(sm_ptr != NULL, error);
                if (sm_ptr->transport.state != connector_transport_terminate)
                {
                    sm_ptr->transport.state = connector_transport_close;
                    sm_ptr->close.callback_needed = connector_false;
                    sm_ptr->close.status = connector_close_status_device_terminated;
                }
            }
            break;

        case connector_initiate_transport_stop:
            if (*transport_ptr == connector_transport_all)
            {
                connector_initiate_stop_request_t stop_request;

                memcpy(&stop_request, request_data, sizeof stop_request);
                #if (defined CONNECTOR_TRANSPORT_UDP)
                stop_request.transport = connector_transport_udp;
                result = sm_initiate_action(handle, request, &stop_request); /* intended recursive */
                if (result == connector_success)
                {
                    connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, connector_transport_udp);

                    sm_ptr->close.callback_needed = connector_false;
                }
                else
                    goto error;
                #endif

                #if (defined CONNECTOR_TRANSPORT_SMS)
                stop_request.transport = connector_transport_sms;
                result = sm_initiate_action(handle, request, &stop_request); /* intended recursive */
                if (result == connector_success)
                {
                    connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, connector_transport_sms);

                    sm_ptr->close.callback_needed = connector_false;
                }
                else
                    goto error;
                #endif
            }
            else
            {
                connector_initiate_stop_request_t const * const stop_ptr = request_data;
                connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, stop_ptr->transport);

                ASSERT_GOTO(sm_ptr != NULL, error);
                switch (sm_ptr->transport.state)
                {
                    case connector_transport_terminate:
                        result = connector_device_terminated;
                        goto error;

                    case connector_transport_close:
                        result = connector_service_busy;
                        goto error;

                    default:
                    {
                        connector_initiate_stop_request_t const * const stop_request = request_data;

                        sm_ptr->close.status = connector_close_status_device_stopped;
                        sm_ptr->close.user_context = stop_ptr->user_context;
                        sm_ptr->close.stop_condition = stop_request->condition;
                        if ((stop_request->condition == connector_stop_immediately) || (sm_ptr->session.head == NULL))
                            sm_ptr->transport.state = connector_transport_close;
                        break;
                    }
                }
            }

            break;

        case connector_initiate_session_cancel:
#if (CONNECTOR_VERSION >= 0x02010000)
        case connector_initiate_session_cancel_all:
#endif
        {
            connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, *transport_ptr);
            
            ASSERT_GOTO(sm_ptr != NULL, error);

            if (sm_ptr->close.stop_condition == connector_wait_sessions_complete)
            {
                result = connector_unavailable;
                goto error;
            }

            if (sm_ptr->pending.data != NULL)
                goto error;

            sm_ptr->pending.data = request_data;
            sm_ptr->pending.request = request;
            sm_ptr->pending.pending_internal = connector_false;
            break;
        }

        case connector_initiate_ping_request:
#if (defined CONNECTOR_DATA_SERVICE)
        case connector_initiate_send_data:
#endif
#if (defined CONNECTOR_DATA_POINTS)
        case connector_initiate_data_point_binary:
        case connector_initiate_data_point_single:
#if (CONNECTOR_VERSION >= 0x02020000)
        case connector_initiate_data_point_multiple:
#endif
#endif
        {
            connector_sm_data_t * const sm_ptr = get_sm_data(connector_ptr, *transport_ptr);

            ASSERT_GOTO(sm_ptr != NULL, error);

            if (sm_ptr->close.stop_condition == connector_wait_sessions_complete)
            {
                result = connector_unavailable;
                goto error;
            }

            switch (sm_ptr->transport.state)
            {
                case connector_transport_idle:
                case connector_transport_open:
                case connector_transport_close:
                case connector_transport_terminate:
                case connector_transport_wait_for_reconnect:
                    result = connector_unavailable;
                    goto error;
                case connector_transport_send:
                case connector_transport_receive:
                case connector_transport_redirect:
                {
#if (CONNECTOR_VERSION >= 0x02010000)
                    uint32_t * request_id = NULL;
#endif
                    if (sm_ptr->close.stop_condition == connector_wait_sessions_complete)
                    {
                        result = connector_unavailable;
                        goto error;
                    }
#if (CONNECTOR_VERSION >= 0x02010000)
                    request_id = get_request_id_ptr(request, request_data);
                    /* dp_initiate_data_point_single/multiple/binary() convert a connector_initiate_data_point_single/multiple/binary
                     * to a connector_initiate_send_data, but we want that that "hidden" connector_initiate_send_data
                     * use the same request_id, which has been already set by dp_send_message().
                     * */
                    if (sm_ptr->pending.pending_internal)
                    {
                        sm_ptr->pending.pending_internal = connector_false;
                        if (request_id != NULL)
                        {
                            /* Use the same request_id */
                            sm_ptr->pending.request_id = *request_id;
                        }
                        else
                        {
                            /* Update request_id */
                            result = sm_get_request_id(connector_ptr, sm_ptr);
                            ASSERT_GOTO(result == connector_working, error);
                            sm_ptr->pending.request_id = connector_ptr->last_request_id;
                        }
                    }
                    else
                    {
                        /* Update request_id */
                        result = sm_get_request_id(connector_ptr, sm_ptr);
                        ASSERT_GOTO(result == connector_working, error);
                        sm_ptr->pending.request_id = connector_ptr->last_request_id;
                        if (request_id != NULL)
                            *request_id = sm_ptr->pending.request_id;
                    }
#endif

#if (defined CONNECTOR_DATA_POINTS)
                    switch (request)
                    {
                        case connector_initiate_data_point_single:
                        {
                            sm_ptr->pending.pending_internal = connector_true;
                            result = dp_initiate_data_point_single(request_data);
                            goto done_datapoints;
                        }
#if (CONNECTOR_VERSION >= 0x02020000)
                        case connector_initiate_data_point_multiple:
                        {
                            sm_ptr->pending.pending_internal = connector_true;
                            result = dp_initiate_data_point_multiple(request_data);
                            goto done_datapoints;
                        }
#endif
                        case connector_initiate_data_point_binary:
                        {
                            sm_ptr->pending.pending_internal = connector_true;
                            result = dp_initiate_data_point_binary(request_data);
                            goto done_datapoints;
                        }

                        default:
                            break;
                    }
#endif
                    if (sm_ptr->pending.data != NULL)
                    {
                        result = connector_service_busy;
                        goto error;
                    }
                    sm_ptr->pending.data = request_data;
                    sm_ptr->pending.request = request;
                    break;
                }
                default:
                    ASSERT(connector_false);
                    break;
            }
            break;
        }
        default:
            ASSERT(connector_false);
            break;
    }

done:
    result = connector_success;

#if (defined CONNECTOR_DATA_POINTS)
done_datapoints:
#endif
error:
    return result;
}

static void sm_init_network_packet(connector_sm_packet_t * const packet, void * const ptr)
{
    packet->data = ptr;
    packet->total_bytes = 0;
    packet->processed_bytes = 0;
    packet->pending_session = NULL;
}

static connector_status_t sm_open_transport(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr)
{
    connector_status_t result;

    {
        connector_callback_status_t status;
        connector_network_open_t open_data;
        connector_request_id_t request_id;

        switch(sm_ptr->network.class_id)
        {
            case connector_class_id_network_sms:
                open_data.device_cloud_url = connector_ptr->device_cloud_phone;
                break;

            default:
                open_data.device_cloud_url = connector_ptr->device_cloud_url;
                break;
        }
        open_data.handle = NULL;

        request_id.network_request = connector_request_id_network_open;
        status = connector_callback(connector_ptr->callback, sm_ptr->network.class_id, request_id, &open_data);
        ASSERT(status != connector_callback_unrecognized);
        switch (status)
        {
            case connector_callback_continue:
                result = connector_working;
                sm_ptr->network.handle = open_data.handle;
                break;

            case  connector_callback_abort:
                result = connector_abort;
                goto error;

            case connector_callback_unrecognized:
                result = connector_unavailable;
                goto error;

            case connector_callback_error:
                result = connector_open_error;
                goto error;

            case connector_callback_busy:
                result = connector_pending;
                goto error;
        }
    }

    {
        void * data_ptr;
        size_t const data_size = 2 * sm_ptr->transport.mtu;

        result = malloc_data_buffer(connector_ptr, data_size, named_buffer_id(sm_packet), &data_ptr);
        ASSERT_GOTO(result == connector_working, error);

        {
            uint8_t * const send_data_ptr = data_ptr;
            uint8_t * const recv_data_ptr = send_data_ptr + sm_ptr->transport.mtu;

            sm_init_network_packet(&sm_ptr->network.send_packet, send_data_ptr);
            sm_init_network_packet(&sm_ptr->network.recv_packet, recv_data_ptr);
        }
    }

error:
    return result;
}

static connector_status_t sm_close_transport(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr)
{
    connector_status_t result = sm_cancel_session(connector_ptr, sm_ptr, NULL);

    if (result == connector_abort)
        sm_ptr->close.status = connector_close_status_abort;

    if (sm_ptr->network.handle != NULL)
    {
        connector_callback_status_t callback_status;
        connector_request_id_t request_id;
        connector_network_close_t close_data;

        close_data.handle = sm_ptr->network.handle;
        close_data.status = sm_ptr->close.status;

        connector_debug_printf("sm_close_transport: status %d\n", sm_ptr->close.status);
        request_id.network_request = connector_request_id_network_close;
        callback_status = connector_callback(connector_ptr->callback, sm_ptr->network.class_id, request_id, &close_data);
        ASSERT(callback_status != connector_callback_unrecognized);
        switch (callback_status)
        {
            case connector_callback_busy:
                result = connector_pending;
                goto done;

            case connector_callback_continue:
                result = connector_working;
                break;

            default:
                sm_ptr->close.status = connector_close_status_abort;
                break;
        }

        sm_ptr->network.handle = NULL;
    }

    if (sm_ptr->network.send_packet.data != NULL)
    {
        if (free_data_buffer(connector_ptr, named_buffer_id(sm_packet), sm_ptr->network.send_packet.data) == connector_abort)
            sm_ptr->close.status = connector_close_status_abort;
        sm_ptr->network.send_packet.data = NULL;
    }

    if (sm_ptr->close.callback_needed)
    {
        connector_transport_t const transport = sm_ptr->network.transport;
        connector_status_t const stop_status = connector_stop_callback(connector_ptr, transport, sm_ptr->close.user_context);

        switch (stop_status)
        {
            case connector_abort:
                sm_ptr->close.status = connector_close_status_abort;
                break;

            case connector_pending:
                result = connector_pending;
                goto done;

            default:
                break;
        }
    }

    switch (sm_ptr->close.status)
    {
        case connector_close_status_abort:
        case connector_close_status_device_terminated:
            sm_ptr->transport.state = connector_transport_terminate;
            result = (sm_ptr->close.status == connector_close_status_device_terminated) ? connector_device_terminated : connector_abort;
            break;

        default:
            sm_ptr->transport.state = connector_transport_idle;
            sm_ptr->close.stop_condition = connector_stop_immediately;
            sm_ptr->close.callback_needed = connector_true;
            break;
    }

done:
    return result;
}

static connector_status_t sm_state_machine(connector_data_t * const connector_ptr, connector_sm_data_t * const sm_ptr)
{
    connector_status_t result = connector_idle;
    size_t iterations = 2;

    ASSERT_GOTO(sm_ptr != NULL, error);

    if (sm_ptr->transport.state == connector_transport_terminate)
    {
        result = connector_device_terminated;
        goto done;
    }
    result = sm_process_pending_data(connector_ptr, sm_ptr);
    if (result != connector_idle && result != connector_pending)
        goto done;

#if (defined CONNECTOR_DATA_POINTS)
    result = dp_process_request(connector_ptr, sm_ptr->network.transport);
    if ((result != connector_idle) && (result != connector_working))
        goto error;
#endif

    while (iterations > 0)
    {
        switch (sm_ptr->transport.state)
        {
            case connector_transport_idle:
                if ((sm_ptr->transport.connect_type == connector_connect_auto) && (sm_ptr->close.status == connector_close_status_device_error))
                    sm_ptr->transport.state = connector_transport_open;
                goto done;

            case connector_transport_open:
                result = sm_open_transport(connector_ptr, sm_ptr);
                switch(result)
                {
                    case connector_working:
                        sm_ptr->transport.state = connector_transport_receive;
                        break;
                    case connector_pending:
                        sm_ptr->transport.state = connector_transport_open;
                        break;
                    case connector_open_error:
                    {
                        sm_ptr->transport.connect_at = 0;
                        sm_ptr->transport.state = connector_transport_wait_for_reconnect;
                        break;
                    }
                    default:
                        sm_ptr->transport.state = connector_transport_idle;
                        break;
                }
                goto done;

            case connector_transport_receive:
                sm_ptr->transport.state = connector_transport_send;
                if (sm_ptr->network.handle != NULL)    /* Give a chance to dynamic reconfiguration of the network when provisioning message arrives */
                    result = sm_receive_data(connector_ptr, sm_ptr); /* NOTE: sm_receive_data has precedence over sm_process_recv_path just to keep the receive buffer ready */
                if ((result != connector_idle) && (result != connector_pending))
                    goto done;
                else
                {
                    connector_sm_session_t * session = (sm_ptr->session.current == NULL) ? sm_ptr->session.head : sm_ptr->session.current;

                    if (session == NULL) goto done;

                    do
                    {
                        if (session->sm_state >= connector_sm_state_receive_data)
                        {
                            result = sm_process_recv_path(connector_ptr, sm_ptr, session);
                            switch (result)
                            {
                                case connector_working:
                                case connector_pending:
                                    sm_ptr->session.current = session->next;
                                    goto done;

                                case connector_idle:
                                    break;

                                default:
                                    ASSERT_GOTO(connector_false, error);
                                    break;
                            }
                        }

                        session = session->next;
                        sm_ptr->session.current = session;

                    } while (session != NULL);
                }

                iterations--;
                break;

            case connector_transport_send:
            {
                connector_sm_session_t * session = (sm_ptr->session.current == NULL) ? sm_ptr->session.head : sm_ptr->session.current;

                sm_ptr->transport.state = connector_transport_receive;
                if (session == NULL) goto done;

                do
                {
                    if (session->sm_state <= connector_sm_state_send_data)
                    {
                        result = sm_process_send_path(connector_ptr, sm_ptr, session);
                        switch (result)
                        {
                            case connector_working:
                            case connector_pending:
                                sm_ptr->session.current = session->next;
                                goto done;

                            case connector_idle:
                                break;

                            default:
                                ASSERT_GOTO(connector_false, error);
                                break;
                        }
                    }

                    session = session->next;
                    sm_ptr->session.current = session;

                } while (session != NULL);

                iterations--;
                break;
            }

            case connector_transport_close:
                result = sm_close_transport(connector_ptr, sm_ptr);
                goto done;

            case connector_transport_terminate:
                break;

            case connector_transport_wait_for_reconnect:
            {
                if (sm_ptr->transport.connect_at == 0)
                {
#if (defined CONNECTOR_TRANSPORT_UDP) && defined CONNECTOR_TRANSPORT_SMS
                    connector_debug_printf("Waiting %d second before reconnecting SM transport %s\n",
                                 CONNECTOR_TRANSPORT_RECONNECT_AFTER, sm_ptr->network.transport == connector_transport_udp ? "UDP" : "SMS");
#elif defined CONNECTOR_TRANSPORT_UDP
                    connector_debug_printf("Waiting %d second before reconnecting SM transport UDP\n", CONNECTOR_TRANSPORT_RECONNECT_AFTER);
#else
                    connector_debug_printf("Waiting %d second before reconnecting SM transport SMS\n", CONNECTOR_TRANSPORT_RECONNECT_AFTER);
#endif
                    result = get_system_time(connector_ptr, &sm_ptr->transport.connect_at);
                    if (result != connector_working)
                        goto done;
                    sm_ptr->transport.connect_at += CONNECTOR_TRANSPORT_RECONNECT_AFTER;
                } else {
                    unsigned long int uptime;

                    result = get_system_time(connector_ptr, &uptime);
                    if (result != connector_working)
                        goto done;
                    if (uptime >= sm_ptr->transport.connect_at)
                        sm_ptr->transport.state = connector_transport_open;
                }
                break;
            }
            default:
                ASSERT_GOTO(connector_false, error);
                break;
        }
    }

error:
done:
    return result;
}

#if (defined CONNECTOR_TRANSPORT_UDP)
static connector_status_t connector_udp_step(connector_data_t * const connector_ptr)
{
    return sm_state_machine(connector_ptr, &connector_ptr->sm_udp);
}
#endif

#if (defined CONNECTOR_TRANSPORT_SMS)
static connector_status_t connector_sms_step(connector_data_t * const connector_ptr)
{
    return sm_state_machine(connector_ptr, &connector_ptr->sm_sms);
}
#endif
