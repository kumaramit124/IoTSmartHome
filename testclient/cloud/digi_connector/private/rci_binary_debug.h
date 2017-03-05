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

#if !(defined RCI_DEBUG)

static void rci_debug_printf(char const * const format, ...)
{
    (void) format;
}

#define rci_input_state_t_as_string(value)      NULL
#define rci_output_state_t_as_string(value)     NULL
#define rci_traverse_state_t_as_string(value)     NULL

#else

#define rci_debug_printf    connector_debug_printf

#define enum_to_case(name)  case name:  result = #name;             break

static char const * rci_input_state_t_as_string(rci_input_state_t const value)
{
    char const * result=NULL;
    switch (value)
    {
        enum_to_case(rci_input_state_command_id);
        enum_to_case(rci_input_state_command_attribute);
        enum_to_case(rci_input_state_group_id);
        enum_to_case(rci_input_state_group_attribute);
        enum_to_case(rci_input_state_field_id);
        enum_to_case(rci_input_state_field_type);
        enum_to_case(rci_input_state_field_no_value);
        enum_to_case(rci_input_state_field_value);
        enum_to_case(rci_input_state_done);
    }
    return result;
}

static char const * rci_output_state_t_as_string(rci_output_state_t const value)
{
    char const * result=NULL;
    switch (value)
    {
        enum_to_case(rci_output_state_command_id);
        enum_to_case(rci_output_state_group_id);
        enum_to_case(rci_output_state_group_attribute);
        enum_to_case(rci_output_state_field_id);
        enum_to_case(rci_output_state_field_value);
        enum_to_case(rci_output_state_field_terminator);
        enum_to_case(rci_output_state_group_terminator);
        enum_to_case(rci_output_state_response_done);
        enum_to_case(rci_output_state_done);
    }
    return result;
}

static char const * rci_traverse_state_t_as_string(rci_traverse_state_t const value)
{
    char const * result=NULL;
    switch (value)
    {
        enum_to_case(rci_traverse_state_none);
        enum_to_case(rci_traverse_state_command_id);
        enum_to_case(rci_traverse_state_group_id);
        enum_to_case(rci_traverse_state_element_id);
        enum_to_case(rci_traverse_state_element_end);
        enum_to_case(rci_traverse_state_group_end);
        enum_to_case(rci_traverse_state_all_groups);
        enum_to_case(rci_traverse_state_all_group_instances);
        enum_to_case(rci_traverse_state_all_elements);
    }
    return result;
}

#endif
