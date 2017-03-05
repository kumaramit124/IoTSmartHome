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

#include "rci_binary_support.h"
#include "rci_binary_debug.h"
#include "rci_binary_util.h"
#include "rci_binary_buffer.h"
#include "rci_binary_string.h"
#include "rci_binary_group.h"
#include "rci_binary_element.h"
#include "rci_binary_callback.h"
#include "rci_binary_output.h"
#include "rci_binary_input.h"
#include "rci_binary_traverse.h"
#include "rci_binary_error.h"

static connector_bool_t rci_action_session_start(rci_t * const rci, rci_service_data_t * service_data)
{
    ASSERT(rci->service_data == NULL);
    rci->service_data = service_data;
    ASSERT(rci->service_data != NULL);

    rci_set_buffer(&rci->buffer.input, &rci->service_data->input);
    rci_set_buffer(&rci->buffer.output, &rci->service_data->output);
#if defined RCI_DEBUG
    memset(rci->service_data->output.data, 0, rci->service_data->output.bytes);
#endif

    rci->input.destination = rci_buffer_position(&rci->buffer.input);
    reset_input_content(rci);

    invalidate_group_id(rci);
    invalidate_group_index(rci);
    invalidate_element_id(rci);

    rci->shared.callback_data.response.element_value = &rci->shared.value;

    rci->status = rci_status_busy;
    rci->error.command_error = connector_false;
    rci->input.flag = 0;

    trigger_rci_callback(rci, connector_request_id_remote_config_session_start);
    set_rci_input_state(rci, rci_input_state_command_id);
    state_call(rci, rci_parser_state_input);

    set_rci_error_state(rci, rci_error_state_id);

    return connector_true;
}

static connector_bool_t rci_action_session_active(rci_t * const rci)
{
    connector_bool_t success = connector_true;

    switch (rci->status)
    {
        case rci_status_error:
        case rci_status_complete:
        {
            rci->status = rci_status_internal_error;
            /* no break; */
        }

        case rci_status_internal_error:
        {
            success = connector_false;
            break;
        }

        case rci_status_busy:
        {
            break;
        }

        case rci_status_more_input:
        {
            connector_debug_hexvalue("rci_binary more request", rci->service_data->input.data, rci->service_data->input.bytes);
            rci_set_buffer(&rci->buffer.input, &rci->service_data->input);
            if (!destination_in_storage(rci))
            {
                rci->input.destination = rci_buffer_position(&rci->buffer.input);
            }
            rci->status = rci_status_busy;
            break;
        }

        case rci_status_flush_output:
        {
            rci_set_buffer(&rci->buffer.output, &rci->service_data->output);
#if defined RCI_DEBUG
            memset(rci->service_data->output.data, 0, rci->service_data->output.bytes);
#endif
            rci->status = rci_status_busy;
            break;
        }
    }

    return success;
}

static connector_bool_t rci_action_session_lost(rci_t * const rci)
{
    trigger_rci_callback(rci, connector_request_id_remote_config_session_cancel);
    {
        connector_bool_t const success = rci_callback(rci);
        ASSERT(success); UNUSED_VARIABLE(success);
    }

    rci->status = rci_status_complete;

    return connector_false;
}


static rci_status_t rci_binary(rci_session_t const action, ...)
{
    static rci_t rci;

    {
        connector_bool_t success;
        va_list ap;

        switch (action)
        {
        case rci_session_start:
            va_start(ap, action);
            success = rci_action_session_start(&rci, va_arg(ap, rci_service_data_t *));
            va_end(ap);
            break;

        case rci_session_active:
            success = rci_action_session_active(&rci);
            break;

        case rci_session_lost:
            success = rci_action_session_lost(&rci);
            break;

        default:
            success = connector_false;
            break;
        }

        if (!success) goto done;
    }

    if (pending_rci_callback(&rci))
    {
        connector_remote_config_t * const remote_config = &rci.shared.callback_data;

        if (!rci_callback(&rci))
            goto done;

        if (remote_config->error_id != connector_success)
        {
            rci_group_error(&rci, remote_config->error_id, remote_config->response.error_hint);
            goto done;
        }
    }

    switch (rci.parser.state)
    {
    case rci_parser_state_input:
        rci_parse_input(&rci);
        break;

    case rci_parser_state_output:
        rci_generate_output(&rci);
        break;

    case rci_parser_state_traverse:
        rci_traverse_data(&rci);
        break;

    case rci_parser_state_error:
        rci_generate_error(&rci);
        break;
    }

done:

    switch (rci.status)
    {
    case rci_status_busy:
        break;
    case rci_status_more_input:
        rci_debug_printf("Need more input\n");
        break;
    case rci_status_flush_output:
        rci.service_data->output.bytes = rci_buffer_used(&rci.buffer.output);
        break;
    case rci_status_complete:
        rci.service_data->output.bytes = rci_buffer_used(&rci.buffer.output);
        /* no break; */
    case rci_status_internal_error:
    case rci_status_error:
        rci.service_data = NULL;
        break;
    }

    return rci.status;
}

