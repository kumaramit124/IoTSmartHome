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

#define state_call(rci, value)  ((rci)->parser.state = (value))

static connector_bool_t is_set_command(connector_remote_action_t const action)
{
    return connector_bool(action == connector_remote_action_set);

}

static void rci_error(rci_t * const rci, unsigned int const id, char const * const description, char const * const hint)
{
    rci->shared.callback_data.error_id = id;
    rci->shared.callback_data.response.error_hint = hint;

    rci->error.description = description;
}

#if defined RCI_PARSER_USES_ERROR_DESCRIPTIONS
#define get_rci_global_error(rci, id)   connector_rci_config_data.error_table[(id) - connector_rci_error_OFFSET]
static char const * get_rci_group_error(rci_t * const rci, unsigned int const id)
{
    connector_group_t const * const group = get_current_group(rci);
    unsigned int const index = (id - connector_rci_config_data.global_error_count);

    ASSERT(id >= connector_rci_config_data.global_error_count);
    ASSERT(index < group->errors.count);

    return group->errors.description[index];
}
#else
#define get_rci_global_error(rci, id)   ((void) (rci), (void) (id), NULL)
#define get_rci_group_error(rci, id)    ((void) (rci), (void) (id), NULL)
#endif

static void rci_global_error(rci_t * const rci, unsigned int const id, char const * const hint)
{
    char const * const description = get_rci_global_error(rci, id);

    rci_error(rci, id, description, hint);
}

static void rci_group_error(rci_t * const rci, unsigned int const id, char const * const hint)
{
    if (id < connector_rci_config_data.global_error_count)
    {
        rci_global_error(rci, id, hint);
    }
    else
    {
        char const * const description = get_rci_group_error(rci, id);

        rci_error(rci, id, description, hint);
    }
}

static connector_bool_t pending_rci_callback(rci_t * const rci)
{
    connector_bool_t const pending = connector_bool(rci->callback.status == connector_callback_busy);

    return pending;
}

static void trigger_rci_callback(rci_t * const rci, connector_request_id_remote_config_t const remote_config_request)
{
    switch (remote_config_request)
    {
    case connector_request_id_remote_config_session_cancel:
        break;

    case connector_request_id_remote_config_session_start:
    case connector_request_id_remote_config_session_end:
    case connector_request_id_remote_config_action_start:
    case connector_request_id_remote_config_action_end:
        break;

    case connector_request_id_remote_config_group_start:
        ASSERT(have_group_id(rci));
        ASSERT(have_group_index(rci));

        rci->shared.callback_data.group.id = get_group_id(rci);
        rci->shared.callback_data.group.index = get_group_index(rci);
        break;

    case connector_request_id_remote_config_group_end:
        ASSERT(have_group_id(rci));
        ASSERT(have_group_index(rci));
        break;

    case connector_request_id_remote_config_group_process:
        ASSERT(have_group_id(rci));
        ASSERT(have_group_index(rci));
        ASSERT(have_element_id(rci));

        rci->shared.callback_data.element.id = get_element_id(rci);
        {
            connector_group_element_t const * const element = get_current_element(rci);

            rci->shared.callback_data.element.type = element->type;
        }

        rci->shared.callback_data.element.value = is_set_command(rci->shared.callback_data.action) ? &rci->shared.value : NULL;
        break;
    }

    rci->callback.request.remote_config_request = remote_config_request;
    rci->callback.status = connector_callback_busy;
}

static connector_bool_t rci_callback(rci_t * const rci)
{
    connector_bool_t callback_complete;
    connector_remote_config_t * remote_config = &rci->shared.callback_data;
    connector_remote_config_cancel_t remote_cancel;
    void * callback_data = NULL;

    switch (rci->callback.request.remote_config_request)
    {
    case connector_request_id_remote_config_session_start:
    case connector_request_id_remote_config_session_end:
    case connector_request_id_remote_config_action_start:
    case connector_request_id_remote_config_action_end:
    case connector_request_id_remote_config_group_start:
    case connector_request_id_remote_config_group_end:
    case connector_request_id_remote_config_group_process:
    {
        remote_config->error_id = connector_success;
        callback_data = remote_config;
        break;
    }

    case connector_request_id_remote_config_session_cancel:
    {
        remote_cancel.user_context = remote_config->user_context;
        callback_data =  &remote_cancel;
        break;
    }
    }

    rci->callback.status = connector_callback(rci->service_data->connector_ptr->callback, connector_class_id_remote_config, rci->callback.request, callback_data);

    switch (rci->callback.status)
    {
    case connector_callback_abort:
        callback_complete = connector_false;
        rci->status = rci_status_error;
        break;

    case connector_callback_continue:
        callback_complete = connector_true;
#if 0
        if (response_data->error_id != connector_success)
        {
            switch (rci->callback.request.remote_config_request)
            {
            case connector_request_id_remote_config_session_end:
                callback_complete = connector_false;
                rci->status = rci_status_internal_error;
                break;
            default:
                break;
            }
        }
#endif
        break;

    case connector_callback_busy:
        callback_complete = connector_false;
        break;

    default:
        callback_complete = connector_false;
        rci->status = rci_status_error;
        break;
    }

    return callback_complete;
}


