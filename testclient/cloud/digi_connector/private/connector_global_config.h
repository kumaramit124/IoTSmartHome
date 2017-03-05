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

static connector_status_t notify_error_status(connector_callback_t const callback, connector_class_id_t const class_number, connector_request_id_t const request_number, connector_status_t const status)
{
    connector_status_t result = connector_working;

#if (defined CONNECTOR_DEBUG)
    connector_config_error_status_t err_status;
    connector_request_id_t request_id;

    request_id.config_request = connector_request_id_config_error_status;
    err_status.class_id = class_number;
    err_status.request_id = request_number;
    err_status.status = status;

    {
        connector_callback_status_t const callback_status = connector_callback(callback, connector_class_id_config, request_id, &err_status);
        switch (callback_status)
        {
            case connector_callback_continue:
                break;
            default:
                result = connector_abort;
                break;
        }
    }

#else
    UNUSED_PARAMETER(callback);
    UNUSED_PARAMETER(class_number);
    UNUSED_PARAMETER(request_number);
    UNUSED_PARAMETER(status);
#endif
    return result;
}

static connector_status_t get_config_device_id(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;
    connector_callback_status_t callback_status;
    connector_request_id_t request_id;
    connector_config_pointer_data_t device_id;

    device_id.bytes_required = DEVICE_ID_LENGTH;

    request_id.config_request = connector_request_id_config_device_id;

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &device_id);
    switch (callback_status)
    {
    case connector_callback_continue:
        if (device_id.bytes_required != DEVICE_ID_LENGTH)
            result = connector_invalid_data_size;
        /* coverity[uninit_use] */
        else if (device_id.data == NULL)
            result = connector_invalid_data;
        else
            memcpy(connector_ptr->device_id, device_id.data, DEVICE_ID_LENGTH);
        break;
    case connector_callback_busy:
        result = connector_pending;
        break;
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        goto done;
    }

    if (result != connector_working)
    {
        notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
        result = connector_abort;
    }

done:
    return result;
}

#if !(defined CONNECTOR_CLOUD_URL)
static connector_status_t get_config_device_cloud_url(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;
    connector_callback_status_t callback_status;
    connector_config_pointer_string_t   cloud_url;
    connector_request_id_t request_id;

    request_id.config_request = connector_request_id_config_device_cloud_url;


    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &cloud_url);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* coverity[uninit_use] */
        if ((cloud_url.length == 0) || (cloud_url.length > CLOUD_URL_LENGTH-1))
        {
            result =  connector_invalid_data_size;
        }
        /* coverity[uninit_use] */
        else if (cloud_url.string == NULL)
        {
            result = connector_invalid_data;
        }
        else
        {
            connector_ptr->device_cloud_url_length = cloud_url.length;
            connector_ptr->device_cloud_url = cloud_url.string;
        }
        break;

    case connector_callback_busy:
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        goto done;
    }

    if (result != connector_working)
    {
        notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
        result = connector_abort;
    }

done:
    return result;
}
#endif

#if (defined CONNECTOR_TRANSPORT_SMS)
#if (CONNECTOR_VERSION >= 0x02010000)
#if !(defined CONNECTOR_CLOUD_PHONE)
static connector_status_t get_config_device_cloud_phone(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;
    connector_callback_status_t callback_status;
    connector_config_pointer_string_t   cloud_phone;
    connector_request_id_t request_id;

    request_id.config_request = connector_request_id_config_get_device_cloud_phone;

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &cloud_phone);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* coverity[uninit_use] */
        if (cloud_phone.length == 0)
        {
            result =  connector_invalid_data_size;
        }
        /* coverity[uninit_use] */
        else if (cloud_phone.string == NULL)
        {
            result = connector_invalid_data;
        }
        else
        {
            connector_ptr->device_cloud_phone_length = cloud_phone.length;
            connector_ptr->device_cloud_phone = cloud_phone.string;
        }
        break;

    case connector_callback_busy:
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        goto done;
    }

    if (result != connector_working)
    {
        notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
        result = connector_abort;
    }

done:
    return result;
}

/* This function will only be called under SMS transport when a 'provisioning' message is received */
static connector_status_t set_config_device_cloud_phone(connector_data_t * const connector_ptr, char * phone_number)
{
    connector_status_t result = connector_working;
    connector_callback_status_t callback_status;
    connector_config_pointer_string_t cloud_phone;
    connector_request_id_t request_id;

    request_id.config_request = connector_request_id_config_set_device_cloud_phone;

    cloud_phone.string = phone_number; 
    cloud_phone.length = strlen(phone_number);

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &cloud_phone);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* User may have changed the pointer during reconfiguration... So reload it again */
        get_config_device_cloud_phone(connector_ptr);
        break;

    case connector_callback_busy:
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        goto error;
    }
error:
    if (result != connector_working)
    {
        notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
        result = connector_abort;
    }

    return result;
}
#endif

#if !(defined CONNECTOR_CLOUD_SERVICE_ID)
static connector_status_t get_config_device_cloud_service_id(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;
    connector_callback_status_t callback_status;
    connector_config_pointer_string_t   cloud_service_id;
    connector_request_id_t request_id;

    request_id.config_request = connector_request_id_config_device_cloud_service_id;

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &cloud_service_id);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* coverity[uninit_use] */
        if (cloud_service_id.string == NULL)
        {
            result = connector_invalid_data;
        }
        else
        {
            connector_ptr->device_cloud_service_id_length = cloud_service_id.length;
            connector_ptr->device_cloud_service_id = cloud_service_id.string;
        }
        break;

    case connector_callback_busy:
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        goto done;
    }

    if (result != connector_working)
    {
        notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
        result = connector_abort;
    }

done:
    return result;
}
#endif
#endif
#endif

static connector_status_t get_config_connection_type(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;

#if (defined CONNECTOR_CONNECTION_TYPE)
    ASSERT((CONNECTOR_CONNECTION_TYPE == connector_connection_type_lan) || (CONNECTOR_CONNECTION_TYPE == connector_connection_type_wan));
    connector_ptr->connection_type = CONNECTOR_CONNECTION_TYPE;
#else
    connector_config_connection_type_t  config_connection;
    connector_callback_status_t callback_status;
    connector_request_id_t request_id;

    request_id.config_request = connector_request_id_config_connection_type;
    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &config_connection);

    switch (callback_status)
    {
    case connector_callback_continue:

        /* coverity[uninit_use] */
        switch (config_connection.type)
        {
        case connector_connection_type_lan:
        case connector_connection_type_wan:
            connector_ptr->connection_type = config_connection.type;
            break;

        default:
            notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, connector_invalid_data_range);
            result = connector_abort;
            break;
        }
        break;

    case connector_callback_busy:
        result = connector_pending;
        break;

    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        break;
    }
#endif

    return result;
}

static connector_status_t get_config_mac_addr(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;

    if (connector_ptr->mac_addr == NULL)
    {
        connector_callback_status_t callback_status;
        connector_request_id_t request_id;
        connector_config_pointer_data_t mac_addr;

        mac_addr.bytes_required = MAC_ADDR_LENGTH;

        request_id.config_request = connector_request_id_config_mac_addr;

        callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &mac_addr);
        switch (callback_status)
        {
        case connector_callback_continue:
            if (mac_addr.bytes_required != MAC_ADDR_LENGTH)
                result = connector_invalid_data_size;
            /* coverity[uninit_use] */
            else if (mac_addr.data == NULL)
                result = connector_invalid_data;
            else
                connector_ptr->mac_addr = mac_addr.data;
            break;

        case connector_callback_busy:
        case connector_callback_unrecognized:
        case connector_callback_abort:
        case connector_callback_error:
            result = connector_abort;
            goto done;
        }

        if (result != connector_working)
        {
            notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
            result = connector_abort;
        }
    }

done:
    return result;
}

#if !(defined CONNECTOR_WAN_LINK_SPEED_IN_BITS_PER_SECOND)
static connector_status_t get_config_link_speed(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;
    connector_callback_status_t callback_status;
    connector_request_id_t request_id;
    connector_config_link_speed_t config_link;

    request_id.config_request = connector_request_id_config_link_speed;

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &config_link);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* coverity[uninit_use] */
        connector_ptr->link_speed = config_link.speed;
        break;
    case connector_callback_busy:
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        break;
    }
    return result;
}
#endif

#if !(defined CONNECTOR_WAN_PHONE_NUMBER_DIALED)
static connector_status_t get_config_phone_number(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;
    if (connector_ptr->phone_dialed == NULL)
    {
        connector_callback_status_t callback_status;
        connector_request_id_t request_id;
        connector_config_pointer_string_t phone_number;

        request_id.config_request = connector_request_id_config_phone_number;

        callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &phone_number);
        switch (callback_status)
        {
        case connector_callback_continue:
            /* coverity[uninit_use] */
            if (phone_number.string == NULL)
            {
                result = connector_invalid_data;
            }
            /* coverity[uninit_use] */
            else if (phone_number.length == 0)
            {
                result = connector_invalid_data_size;
            }
            else
            {
                connector_ptr->phone_dialed_length = phone_number.length;
                connector_ptr->phone_dialed = phone_number.string;
            }
            break;
        case connector_callback_busy:
        case connector_callback_unrecognized:
        case connector_callback_abort:
        case connector_callback_error:
            result = connector_abort;
            goto done;
        }

        if (result != connector_working)
        {
            notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
            result = connector_abort;
        }
    }

done:
    return result;
}
#endif

static connector_status_t get_config_device_id_method(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;

#if (defined CONNECTOR_DEVICE_ID_METHOD)
    ASSERT((CONNECTOR_DEVICE_ID_METHOD == connector_device_id_method_auto) || (CONNECTOR_DEVICE_ID_METHOD == connector_device_id_method_manual));
    connector_ptr->device_id_method = CONNECTOR_DEVICE_ID_METHOD;
#else
    connector_callback_status_t callback_status;

    connector_request_id_t request_id;
    connector_config_device_id_method_t device_id;

    request_id.config_request = connector_request_id_config_device_id_method;
    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &device_id);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* coverity[uninit_use] */
        switch (device_id.method)
        {
        case connector_device_id_method_manual:
        case connector_device_id_method_auto:
            connector_ptr->device_id_method = device_id.method;
            break;

        default:
            notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, connector_invalid_data_range);
            result = connector_abort;
            break;
        }
        break;

    case connector_callback_unrecognized:
        connector_ptr->device_id_method = connector_device_id_method_manual;
        break;

    case connector_callback_busy:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        break;
    }
#endif

    return result;
}

#define MAX_DECIMAL_DIGIT       9
#define MAX_HEXADECIMAL_DIGIT    0xF

static connector_bool_t check_digit_array(uint8_t * const digits, size_t const length, uint8_t const max_digit)
{
    connector_bool_t isDigit = connector_true;
    size_t i;

    for (i=0; i < length; i++)
    {
        unsigned char const up_digit = (digits[i] >> 4) & 0xF;
        unsigned char const lo_digit = digits[i] & 0x0F;

        if (up_digit > max_digit || lo_digit > max_digit)
        {
            isDigit = connector_false;
            break;
        }
    }

    return isDigit;
}

static connector_status_t get_config_wan_id(connector_data_t * const connector_ptr,
                                            connector_request_id_config_t const config_request_id)
{
    connector_status_t result = connector_working;
    connector_callback_status_t callback_status;
    connector_config_pointer_data_t request_data;
    size_t  bytes_required;
    uint8_t max_digits;

    connector_request_id_t request_id;

    switch (config_request_id)
    {
        case connector_request_id_config_imei_number:
            bytes_required = CONNECTOR_GSM_IMEI_LENGTH;
            max_digits = MAX_DECIMAL_DIGIT;
            break;

        case connector_request_id_config_esn:
            bytes_required = CONNECTOR_ESN_HEX_LENGTH;
            max_digits = MAX_HEXADECIMAL_DIGIT;
            break;

        case connector_request_id_config_meid:
            bytes_required = CONNECTOR_MEID_HEX_LENGTH;
            max_digits = MAX_HEXADECIMAL_DIGIT;
            break;

        default:
            ASSERT(connector_false);
            result = connector_abort;
            goto done;
    }

    request_data.bytes_required = bytes_required;
    request_id.config_request = config_request_id;

    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &request_data);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* coverity[uninit_use] */
        if (request_data.data == NULL)
        {
            result = connector_invalid_data;
        }
        else if (request_data.bytes_required != bytes_required)
        {
            result = connector_invalid_data_size;
        }
        else if (check_digit_array(request_data.data, request_data.bytes_required, max_digits) != connector_true)
        {
            result = connector_invalid_data_range;
        }
        else
        {
            connector_ptr->wan_id = request_data.data;
            connector_ptr->wan_id_length = bytes_required;
        }
        break;
    case connector_callback_busy:
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        goto done;
    }

   if (result != connector_working)
    {
        notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, result);
        result = connector_abort;
    }

done:
    return result;
}

static connector_status_t get_config_wan_type(connector_data_t * const connector_ptr)
{
    connector_status_t result = connector_working;
#if (defined CONNECTOR_WAN_TYPE)
    ASSERT((CONNECTOR_WAN_TYPE == connector_wan_type_imei) || (CONNECTOR_WAN_TYPE == connector_wan_type_esn) || (CONNECTOR_WAN_TYPE == connector_wan_type_meid));
    connector_ptr->wan_type = CONNECTOR_WAN_TYPE;
#else
    connector_callback_status_t callback_status;
    connector_request_id_t request_id;
    connector_config_wan_type_t config_wan;


    request_id.config_request = connector_request_id_config_wan_type;
    callback_status = connector_callback(connector_ptr->callback, connector_class_id_config, request_id, &config_wan);
    switch (callback_status)
    {
    case connector_callback_continue:
        /* coverity[uninit_use] */
        switch (config_wan.type)
        {
        case connector_wan_type_imei:
        case connector_wan_type_esn:
        case connector_wan_type_meid:
            connector_ptr->wan_type = config_wan.type;
            break;
        default:
            notify_error_status(connector_ptr->callback, connector_class_id_config, request_id, connector_invalid_data_range);
            result = connector_abort;
            break;
        }
        break;
    case connector_callback_busy:
    case connector_callback_unrecognized:
    case connector_callback_abort:
    case connector_callback_error:
        result = connector_abort;
        break;
    }
#endif
    return result;
}


