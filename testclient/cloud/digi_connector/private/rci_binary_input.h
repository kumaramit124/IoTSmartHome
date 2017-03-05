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


#if defined RCI_PARSER_USES_ERROR_DESCRIPTIONS
static char const rci_set_empty_group_hint[] = "Empty group";
static char const rci_set_empty_element_hint[] = "empty element";
static char const rci_error_descriptor_mismatch_hint[] = "Mismatch configurations";
static char const rci_error_content_size_hint[] = "Maximum content size exceeded";
#else
#define rci_set_empty_group_hint            RCI_NO_HINT
#define rci_set_empty_element_hint          RCI_NO_HINT
#define rci_error_descriptor_mismatch_hint  RCI_NO_HINT
#define rci_error_content_size_hint         RCI_NO_HINT
#endif

static connector_bool_t destination_in_storage(rci_t const * const rci)
{
    uint8_t const * const storage_begin = rci->input.storage;
    uint8_t const * const storage_end = storage_begin + sizeof rci->input.storage;

    return ptr_in_range(rci->input.destination, storage_begin, storage_end);
}

static size_t get_bytes_to_follow(uint8_t opcode)
{
    size_t bytes_to_read = 0;

    if (opcode & BINARY_RCI_SIZE_ALTERNATE_FLAG)
    {
        rci_size_modifier_t size_mask = BINARY_RCI_SIZE_MODIFIER(opcode);
        switch (size_mask)
        {
        case binary_rci_one_follow_byte:
            bytes_to_read = 1;
            break;
        case binary_rci_multi_follow_byte:
            switch (BINARY_RCI_MULTI_FOLLOW_BYTES(opcode))
            {
                case binary_rci_two_follow_byte:
                    bytes_to_read = 2;
                    break;
                case binary_rci_four_follow_byte:
                    bytes_to_read = 4;
                    break;
                case binary_rci_eight_follow_byte:
                    bytes_to_read = 8;
                    break;
            }
            break;
        case binary_rci_special_value:
            bytes_to_read = 0;
            break;
        default:
            ASSERT(connector_false);
        }
    }
    return bytes_to_read;
}

static void reset_input_content(rci_t * const rci)
{
    rci->shared.content.data = rci->input.destination;
    rci->shared.content.length = 0;
}

#define BINARY_RCI_ONE_BYTE_LIMIT_MASK  UINT32_C(0x7F)
#define BINARY_RCI_HI_BYTE_MASK         UINT32_C(0x1F)

static size_t get_modifier_ber(rci_t * const rci, uint32_t * const value)
{

    uint8_t * const rci_ber = rci->shared.content.data;
    uint8_t const modifier_ber = message_load_u8(rci_ber, value);

    size_t bytes_read = ++rci->shared.content.length;

    {
        /* we have modifier BEF */
        size_t const bytes_to_follow = get_bytes_to_follow(modifier_ber) +1;

        bytes_read = rci->shared.content.length;

        if (bytes_read < bytes_to_follow)
        {
            bytes_read = 0;
            goto done;
        }

        switch (bytes_to_follow)
        {
            case record_bytes(rci_ber):
               *value =  (modifier_ber & BINARY_RCI_SIZE_ALTERNATE_FLAG) ? modifier_ber : (modifier_ber & BINARY_RCI_ONE_BYTE_LIMIT_MASK);
                break;
            case record_bytes(rci_ber_u8):
            {
                uint8_t * const rci_ber_u8 = rci_ber;
                /* mask of the 1st byte for data value */
                *value = (modifier_ber & BINARY_RCI_HI_BYTE_MASK) << 8;
                *value |= message_load_u8(rci_ber_u8, value);
                break;
            }
            case record_bytes(rci_ber_u16):
            {
                uint8_t * const rci_ber_u16 = rci_ber;
                *value = message_load_be16(rci_ber_u16, value);
                break;
            }
            case record_bytes(rci_ber_u32):
            {
                uint8_t * const rci_ber_u32 = rci_ber;
                *value = message_load_be32(rci_ber_u32, value);
                break;
            }
            default:
                ASSERT(bytes_to_follow == 1);
                /* NONUM or TRM value */
                *value = modifier_ber;
                break;
        }
        bytes_read = bytes_to_follow;
    }

done:
    return bytes_read;
}

static connector_bool_t get_uint32(rci_t * const rci, uint32_t * const value)
{

    size_t const bytes = get_modifier_ber(rci, value);

    if (bytes > 0)
        reset_input_content(rci);

    return connector_bool(bytes > 0);
}

#if defined RCI_PARSER_USES_FLOAT
static connector_bool_t get_float(rci_t * const rci, float * const value)
{

    connector_bool_t value_decoded = connector_false;
    uint8_t * rci_ber_u32 = rci->shared.content.data;
    uint8_t const opcode = message_load_u8(rci_ber_u32, opcode);

    rci->shared.content.length++;
    ASSERT(sizeof(uint32_t) == sizeof(float));

    {
        /* we have modifier BEF */
        size_t const bytes_to_follow = get_bytes_to_follow(opcode);
        size_t bytes_read = rci->shared.content.length;

        if (bytes_read < (bytes_to_follow + 1))
        {
            goto error;
        }

        /* Current, we only support single precision float. */
        ASSERT(bytes_read == record_bytes(rci_ber_u32));

        {
            uint32_t float_value = message_load_be32(rci_ber_u32, value);
            memcpy(value, &float_value, sizeof *value);
        }
    }

    value_decoded = connector_true;
    reset_input_content(rci);

error:
    return value_decoded;
}
#endif

static connector_bool_t get_string(rci_t * const rci, char const * * string, size_t * const length)
{
    connector_bool_t got_string = connector_false;
    uint32_t value;
    size_t const ber_bytes = get_modifier_ber(rci, &value);

    if (ber_bytes > 0)
    {
        size_t const bytes = rci->shared.content.length;
#if defined CONNECTOR_DEBUG
        size_t const size_max = SIZE_MAX;

        (void)size_max;
        ASSERT(value <= size_max);
#endif
        *length = value;
        if (*length > CONNECTOR_RCI_MAXIMUM_CONTENT_LENGTH)
        {
            connector_debug_printf("Maximum content size exceeded while getting  a string - wanted %u, had %u\n", *length, CONNECTOR_RCI_MAXIMUM_CONTENT_LENGTH);
            rci_set_output_error(rci, connector_rci_error_bad_descriptor, rci_error_content_size_hint, rci_output_state_field_id);
            goto done;
        }

        if (bytes == (ber_bytes + *length))
        {
            char * data = (char *)(rci->shared.content.data + ber_bytes);

            if (!destination_in_storage(rci))
            {
                memcpy(rci->input.storage, data, *length);
                data = (char *)rci->input.storage;
            }

            data[*length] = nul;
            *string =  data;

            reset_input_content(rci);
            got_string = connector_true;
        }
    }
done:
    return got_string;
}

#if defined RCI_PARSER_USES_IPV4
static connector_bool_t get_ip_address(rci_t * const rci, uint32_t * const ip_addr, size_t const length)
{
    connector_bool_t got_ip = connector_false;

    uint8_t * rci_ber_u32 = rci->shared.content.data;
    uint8_t const bytes_read = message_load_u8(rci_ber_u32, opcode);

    UNUSED_PARAMETER(length);

    rci->shared.content.length++;

    if (rci->shared.content.length == (size_t)(bytes_read + 1))
    {
        /* Current, we only support ipv4. */
        ASSERT(bytes_read == sizeof *ip_addr);
        ASSERT(length == bytes_read);

        *ip_addr = message_load_be32(rci_ber_u32, value);

        got_ip = connector_true;
        reset_input_content(rci);
    }

    return got_ip;
}
#endif

static connector_bool_t decode_attribute(rci_t * const rci, unsigned int * index)
{
#define BINARY_RCI_ATTRIBUTE_TYPE_MASK  0x60  /* attr type: [bit 6 and 5] */

#define BINARY_RCI_ATTRIBUTE_TYPE_NORMAL 0x00
#define BINARY_RCI_ATTRIBUTE_TYPE_INDEX  0x20
#define BINARY_RCI_ATTRIBUTE_TYPE_NAME   0x40

#define BINARY_RCI_ATTRIBUTE_INDEX_MASK 0x1F

    connector_bool_t got_attribute = connector_false;
    uint32_t attribute_value;

    if (get_uint32(rci, &attribute_value))
    {
        unsigned int type = attribute_value & BINARY_RCI_ATTRIBUTE_TYPE_MASK;
        switch (type)
        {
            case BINARY_RCI_ATTRIBUTE_TYPE_INDEX:
                *index =  attribute_value & BINARY_RCI_ATTRIBUTE_INDEX_MASK;
                rci_debug_printf("decode_attribute: index = %d\n", *index);
                got_attribute = connector_true;
                break;
            case BINARY_RCI_ATTRIBUTE_TYPE_NAME:
            case BINARY_RCI_ATTRIBUTE_TYPE_NORMAL:
                /* Tool doesn't support name and enum attribute */
                connector_debug_printf("decode_attribute: unsupported attribute type\n");
                rci->status = rci_status_internal_error;
                ASSERT(connector_false);
                break;
        }
    }

    return got_attribute;
}

static connector_bool_t has_rci_atribute(unsigned int data)
{
    return connector_bool((data & BINARY_RCI_ATTRIBUTE_BIT) == BINARY_RCI_ATTRIBUTE_BIT);
}

static connector_bool_t has_rci_error(rci_t * const rci, unsigned int data)
{

    connector_bool_t const hasError = connector_bool((data & BINARY_RCI_ERROR_INDICATOR_BIT) == BINARY_RCI_ERROR_INDICATOR_BIT);

    UNUSED_PARAMETER(rci);

    if (hasError)
    {
        connector_debug_printf("has_rci_error: unexpected error set.\n");
    }
    return hasError;
}

static connector_bool_t has_rci_terminated(unsigned int data)
{
    return connector_bool(data == BINARY_RCI_TERMINATOR);
}

static connector_bool_t has_rci_no_value(unsigned int data)
{
    return connector_bool(data == BINARY_RCI_NO_VALUE);
}

static void process_rci_command(rci_t * const rci)
{
#define BINARY_RCI_COMMAND_MASK   0x3F

    uint32_t command;

    {
        connector_remote_config_t const * const remote_config = &rci->shared.callback_data;

        if (remote_config->error_id != connector_success)
        {
            set_rci_output_state(rci, rci_output_state_field_id);
            state_call(rci, rci_parser_state_output);
            goto done;
        }
    }

    if (get_uint32(rci, &command))
    {

        connector_bool_t const has_attribute = has_rci_atribute(command);

        ASSERT_GOTO(!has_rci_error(rci, command), done);

        command &= BINARY_RCI_COMMAND_MASK;

        switch (command)
        {
            case rci_command_set_setting:
            case rci_command_query_setting:
                rci->shared.callback_data.group.type = connector_remote_group_setting;
                break;
            case rci_command_set_state:
            case rci_command_query_state:
                rci->shared.callback_data.group.type = connector_remote_group_state;
                break;
        }

        switch (command)
        {
            case rci_command_set_setting:
            case rci_command_set_state:
                rci->shared.callback_data.action = connector_remote_action_set;
                break;
            case rci_command_query_setting:
            case rci_command_query_state:
                rci->shared.callback_data.action = connector_remote_action_query;
                break;
            default:
                /* unsupported command.
                 * Just go to error state for returning error message.
                 */
                rci_global_error(rci, connector_rci_error_bad_command, RCI_NO_HINT);
                set_rci_command_error(rci);
                state_call(rci, rci_parser_state_error);
                goto done;
        }

        if (has_attribute)
        {
            set_rci_input_state(rci, rci_input_state_command_attribute);
        }
        else
        {
            set_rci_input_state(rci, rci_input_state_group_id);

            set_rci_traverse_state(rci, rci_traverse_state_command_id);
            state_call(rci, rci_parser_state_traverse);
        }

    }
done:
    return;
}

static void process_command_attribute(rci_t * const rci)
{
    unsigned int index;

    if (decode_attribute(rci, &index))
    {
        /* We don't support command attribute; so just ignore it. */
        set_rci_input_state(rci, rci_input_state_group_id);

        set_rci_traverse_state(rci, rci_traverse_state_command_id);
        state_call(rci, rci_parser_state_traverse);
    }

    return;
}


static void process_group_id(rci_t * const rci)
{
    uint32_t group_id;

    if (!get_uint32(rci, &group_id))
    {
        goto done;
    }

    ASSERT_GOTO(!has_rci_error(rci, group_id), done);

    if (has_rci_terminated(group_id))
    {
        if (have_group_id(rci))
        {
            /* not 1st group */
            set_rci_input_state(rci, rci_input_state_command_id);

            set_rci_traverse_state(rci, rci_traverse_state_group_end);
            state_call(rci, rci_parser_state_traverse);
            goto done;
        }
        else
        {
            /* Get all groups if no group has been requested before */
            switch (rci->shared.callback_data.action)
            {
                case connector_remote_action_query:
                    set_rci_traverse_state(rci, rci_traverse_state_all_groups);
                    state_call(rci, rci_parser_state_traverse);
                    break;
                case connector_remote_action_set:
                    connector_debug_printf("process_group_id: got set command with no group id specified\n");
                    rci_set_output_error(rci, connector_rci_error_bad_command, rci_set_empty_group_hint, rci_output_state_group_id);
                    break;
            }
            goto done;
        }
    }
    else
    {
        set_group_id(rci, decode_group_id(group_id));

        if (!have_group_id(rci))
        {
            connector_debug_printf("process_group_id: unrecognized group (mismatch of descriptors) group id = %d.\n", decode_group_id(group_id));
            rci_set_output_error(rci, connector_rci_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_group_id);
            ASSERT(connector_false);
            goto done;
        }

        if (has_rci_atribute(group_id))
        {
            invalidate_group_index(rci);
            set_rci_input_state(rci, rci_input_state_group_attribute);
            goto done;
        }

        {
            connector_group_t const * const group =  get_current_group(rci);
            if (group->instances > 1)
            {
                set_rci_input_flag(rci, RCI_FLAG_GET_ALL_INSTANCES);
            }
        }

        set_rci_input_state(rci, rci_input_state_field_id);
    }

    set_group_index(rci, 1);
    set_rci_traverse_state(rci, rci_traverse_state_group_id);
    state_call(rci, rci_parser_state_traverse);
    rci_debug_printf("process_group_id: group id = %d\n", get_group_id(rci));

done:
    return;
}

static void process_group_attribute(rci_t * const rci)
{
    if (decode_attribute(rci, &rci->shared.group.index))
    {
        set_rci_input_state(rci, rci_input_state_field_id);
        set_rci_traverse_state(rci, rci_traverse_state_group_id);
        state_call(rci, rci_parser_state_traverse);
    }

    return;
}

static void process_field_id(rci_t * const rci)
{
    unsigned int id = INVALID_ID;
    {
        uint32_t value;

        if (!get_uint32(rci, &value))
        {
            goto done;
        }

        /* Bit 6  - is Field type present indicator
         * Bit 10 - is attributes present indicator
         * Bit 12 - is Error indicator (Should not be set)
         */
        ASSERT_GOTO(!has_rci_error(rci, value), done);

        if (has_rci_terminated(value))
        {
            if (!have_element_id(rci))
            {
                switch (rci->shared.callback_data.action)
                {
                    case connector_remote_action_query:
                        /* all fields */
                        if (is_rci_input_flag(rci,RCI_FLAG_GET_ALL_INSTANCES))
                            set_rci_traverse_state(rci, rci_traverse_state_all_group_instances);
                        else
                            set_rci_traverse_state(rci, rci_traverse_state_all_elements);

                        set_rci_input_state(rci, rci_input_state_group_id);

                        state_call(rci, rci_parser_state_traverse);
                        break;
                    case connector_remote_action_set:
                        connector_debug_printf("process_field_id: got set command with no field id specified\n");
                        rci_set_output_error(rci, connector_rci_error_bad_command, rci_set_empty_element_hint, rci_output_state_field_id);
                        break;
                }
            }
            else
            {
                /* done with all fields */
                invalidate_element_id(rci);
                set_rci_input_state(rci, rci_input_state_group_id);

                set_rci_traverse_state(rci, rci_traverse_state_element_end);
                state_call(rci, rci_parser_state_traverse);
            }
            goto done;
        }

        if ((value & BINARY_RCI_FIELD_TYPE_INDICATOR_BIT) == BINARY_RCI_FIELD_TYPE_INDICATOR_BIT)
        {
            set_rci_input_state(rci, rci_input_state_field_type);
        }
        else
        {
            set_rci_input_state(rci, rci_input_state_field_no_value);
        }


        if ((value & BINARY_RCI_FIELD_ATTRIBUTE_BIT) == BINARY_RCI_FIELD_ATTRIBUTE_BIT)
        {
            connector_debug_printf("process_field_id: field attribute is not supported\n");
            rci_set_output_error(rci, connector_rci_error_bad_descriptor, RCI_NO_HINT, rci_output_state_field_id);
            goto done;
        }

        id = decode_element_id(value);

    }


    set_element_id(rci, id);

    if (!have_element_id(rci))
    {
        connector_debug_printf("process_field_id: unrecognized field id (mismatch of descriptors). element id = %d\n", id);
        rci_set_output_error(rci, connector_rci_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_field_id);
    }

done:
    return;
}

static void process_field_type(rci_t * const rci)
{
    connector_group_element_t const * const element = get_current_element(rci);
    connector_bool_t error = connector_false;
    uint32_t type;

    if (get_uint32(rci, &type))
    {
        ASSERT_GOTO(!has_rci_error(rci, type), done);

        switch (type)
        {
            case rci_field_type_none:
                break;

            default:
                if (element->type != type)
                {
                    connector_debug_printf("process_field_type: mismatch field type (type %d) != (actual %d)\n", type, element->type);
                    rci_set_output_error(rci, connector_rci_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_field_id);
                    error = connector_true;
                }
                break;
        }
    }

    if (!error)
    {
        set_rci_input_state(rci, rci_input_state_field_no_value);
    }

done:
    return;
}

static void process_field_value(rci_t * const rci)
{
    connector_group_element_t const * const element = get_current_element(rci);
    connector_element_value_type_t const type = element->type;

#if (defined RCI_PARSER_USES_ON_OFF) || (defined RCI_PARSER_USES_BOOLEAN)
    connector_bool_t error = connector_false;
#endif

    switch (type)
    {
#if defined RCI_PARSER_USES_STRINGS

#if defined RCI_PARSER_USES_STRING
    case connector_element_type_string:
#endif

#if defined RCI_PARSER_USES_MULTILINE_STRING
    case connector_element_type_multiline_string:
#endif

#if defined RCI_PARSER_USES_PASSWORD
    case connector_element_type_password:
#endif


#if defined RCI_PARSER_USES_FQDNV4
    case connector_element_type_fqdnv4:
#endif

#if defined RCI_PARSER_USES_FQDNV6
    case connector_element_type_fqdnv6:
#endif

#if defined RCI_PARSER_USES_DATETIME
    case connector_element_type_datetime:
#endif
        if (!get_string(rci, &rci->shared.value.string_value, &rci->shared.string_value_length))
        {
            goto done;
        }
        break;
#endif


#if defined RCI_PARSER_USES_INT32
    case connector_element_type_int32:
    {
        int32_t value;

        if (!get_uint32(rci, (uint32_t *)&value))
        {
            goto done;
        }
        rci->shared.value.signed_integer_value = value;
        break;
    }
#endif

#if defined RCI_PARSER_USES_IPV4
    case connector_element_type_ipv4:
    {
        uint32_t ip_addr;

        if (!get_ip_address(rci, &ip_addr, sizeof ip_addr))
        {
            goto done;
        }

        {
            #define MAX_IPV4_VALUE "255.255.255.255"

            char * const data = (char *)rci->input.storage;
            size_t const size_avail = sizeof MAX_IPV4_VALUE;
            int size_written;

            uint8_t ip1 = BYTE32_3(ip_addr);
            uint8_t ip2 = BYTE32_2(ip_addr);
            uint8_t ip3 = BYTE32_1(ip_addr);
            uint8_t ip4 = BYTE32_0(ip_addr);

            ASSERT(size_avail <= sizeof rci->input.storage);

            size_written = connector_snprintf(data, size_avail, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

            ASSERT(size_written < (int)size_avail);
            ASSERT_GOTO(size_written > 0, done);

            rci->shared.value.string_value = data;

            #undef MAX_IPV4_VALUE
        }
        break;
    }
#endif

#if (defined RCI_PARSER_USES_UNSIGNED_INTEGER)


#if defined RCI_PARSER_USES_UINT32
    case connector_element_type_uint32:
#endif

#if defined RCI_PARSER_USES_HEX32
    case connector_element_type_hex32:
#endif

#if defined RCI_PARSER_USES_0X_HEX32
    case connector_element_type_0x_hex32:
#endif
    {
        uint32_t value;

        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.unsigned_integer_value = value;
        break;
    }
#endif

#if defined RCI_PARSER_USES_FLOAT
    case connector_element_type_float:
    {
        if (!get_float(rci, &rci->shared.value.float_value))
        {
            goto done;
        }
        break;
    }
#endif

#if defined RCI_PARSER_USES_ENUM
    case connector_element_type_enum:
    {
        uint32_t value;
        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.enum_value = value;
        break;
    }
#endif

#if defined RCI_PARSER_USES_ON_OFF
    case connector_element_type_on_off:
    {
        uint32_t value;

        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.on_off_value = (value == 0)? connector_off : connector_on;
        error = connector_bool((value != 0) && (value != 1));
        break;
    }
#endif

#if defined RCI_PARSER_USES_BOOLEAN
    case connector_element_type_boolean:
    {
        uint32_t value;
        if (!get_uint32(rci, &value))
        {
            goto done;
        }
        rci->shared.value.boolean_value = (value == 0)? connector_false : connector_true;
        error = connector_bool((value != 0) && (value != 1));
        break;
    }
#endif
    }

#if (defined RCI_PARSER_USES_ON_OFF) || (defined RCI_PARSER_USES_BOOLEAN)
    if (error)
    {
        connector_debug_printf("process_field_value: range check error! Descriptor problem!");
        rci_set_output_error(rci, connector_rci_error_bad_descriptor, rci_error_descriptor_mismatch_hint, rci_output_state_field_id);
    }
    else
#endif
    {
        set_rci_traverse_state(rci, rci_traverse_state_element_id);
        state_call(rci, rci_parser_state_traverse);
    }
    set_rci_input_state(rci, rci_input_state_field_id);

done:
    return;
}

static void process_field_no_value(rci_t * const rci)
{
    uint8_t * const rci_ber = rci->shared.content.data;
    uint8_t const modifier_ber = message_load_u8(rci_ber, value);

    if (has_rci_no_value(modifier_ber))
    {
        /* this initializes element.value in case for set setting */
        connector_group_element_t const * const element = get_current_element(rci);
        connector_element_value_type_t const type = element->type;

        switch (type)
        {
#if defined RCI_PARSER_USES_STRINGS

    #if defined RCI_PARSER_USES_STRING
        case connector_element_type_string:
    #endif

    #if defined RCI_PARSER_USES_MULTILINE_STRING
        case connector_element_type_multiline_string:
    #endif

    #if defined RCI_PARSER_USES_PASSWORD
        case connector_element_type_password:
    #endif


    #if defined RCI_PARSER_USES_FQDNV4
        case connector_element_type_fqdnv4:
    #endif

    #if defined RCI_PARSER_USES_FQDNV6
        case connector_element_type_fqdnv6:
    #endif

    #if defined RCI_PARSER_USES_DATETIME
        case connector_element_type_datetime:
    #endif

    #if defined RCI_PARSER_USES_IPV4
        case connector_element_type_ipv4:
    #endif
            rci->shared.value.string_value = (char *)rci->input.storage;
            rci->input.storage[0] = (uint8_t)nul;
            break;
#endif

#if defined RCI_PARSER_USES_INT32
        case connector_element_type_int32:
            rci->shared.value.signed_integer_value = 0;
            break;
#endif

#if (defined RCI_PARSER_USES_UNSIGNED_INTEGER)


#if defined RCI_PARSER_USES_UINT32
        case connector_element_type_uint32:
#endif

#if defined RCI_PARSER_USES_HEX32
        case connector_element_type_hex32:
#endif

#if defined RCI_PARSER_USES_0X_HEX32
        case connector_element_type_0x_hex32:
#endif
            rci->shared.value.unsigned_integer_value = 0;
            break;
#endif

#if defined RCI_PARSER_USES_FLOAT
        case connector_element_type_float:
            rci->shared.value.float_value = 0;
            break;
#endif

#if defined RCI_PARSER_USES_ENUM
        case connector_element_type_enum:
            rci->shared.value.enum_value = 0;
            break;
#endif

#if defined RCI_PARSER_USES_ON_OFF
        case connector_element_type_on_off:
            rci->shared.value.on_off_value = connector_off;
            break;
#endif

#if defined RCI_PARSER_USES_BOOLEAN
        case connector_element_type_boolean:
            rci->shared.value.boolean_value = connector_false;
            break;
#endif
        }

        reset_input_content(rci);
        set_rci_traverse_state(rci, rci_traverse_state_element_id);
        state_call(rci, rci_parser_state_traverse);
        set_rci_input_state(rci, rci_input_state_field_id);
    }
    else
    {
        set_rci_input_state(rci, rci_input_state_field_value);
        process_field_value(rci);
    }

}

static void rci_parse_input(rci_t * const rci)
{
    rci_buffer_t * const input = &rci->buffer.input;

    while ((rci->parser.state == rci_parser_state_input) && (rci_buffer_remaining(input) != 0))
    {

        if (rci->input.destination != rci_buffer_position(&rci->buffer.input))
        {
            *(rci->input.destination) = rci_buffer_read(input);
        }
        rci->input.destination++;

        rci_debug_printf("input: %s\n", rci_input_state_t_as_string(rci->input.state));

        switch (rci->input.state)
        {
            case rci_input_state_command_id:
                process_rci_command(rci);
                break;
            case rci_input_state_command_attribute:
                process_command_attribute(rci);
                break;
            case rci_input_state_group_id:
                process_group_id(rci);
                break;
            case rci_input_state_group_attribute:
                process_group_attribute(rci);
                break;
            case rci_input_state_field_id:
                process_field_id(rci);
                break;
            case rci_input_state_field_type:
                process_field_type(rci);
                break;
            case rci_input_state_field_no_value:
                process_field_no_value(rci);
                break;
            case rci_input_state_field_value:
                process_field_value(rci);
                break;
            case rci_input_state_done:
                break;
        }

        {
            size_t const storage_bytes = sizeof rci->input.storage;
            uint8_t const * const storage_end = rci->input.storage + storage_bytes;

            if (rci->input.destination == storage_end)
            {
                connector_debug_printf("Maximum content size exceeded while parsing - wanted %u, had %u\n", storage_bytes + 1, storage_bytes);
                rci_set_output_error(rci, connector_rci_error_bad_descriptor, rci_error_content_size_hint, rci_output_state_field_id);
                goto done;
            }
        }

        rci_buffer_advance(input, 1);
    }

    if (rci_buffer_remaining(input) == 0)
    {
        if (MsgIsLastData(rci->service_data->input.flags))
        {
            set_rci_input_state(rci, rci_input_state_done);
            state_call(rci, rci_parser_state_traverse);
        }
        else
        {
            uint8_t const * const old_base = rcistr_data(&rci->shared.content);
            uint8_t * const new_base = destination_in_storage(rci) ? rci->input.destination : rci->input.storage;

            if (ptr_in_buffer(old_base, &rci->buffer.input))
            {
                size_t const storage_bytes = sizeof rci->input.storage;
                uint8_t const * const storage_end = rci->input.storage + storage_bytes;
                size_t const bytes_wanted = (size_t)(rci->buffer.input.end - old_base);
                size_t const bytes_have = (size_t)(storage_end - new_base);

                if (bytes_wanted >= bytes_have)
                {
                    connector_debug_printf("Maximum content size exceeded while storing - wanted %u, had %u\n", bytes_wanted, bytes_have);
                    rci_set_output_error(rci, connector_rci_error_bad_descriptor, rci_error_content_size_hint, rci_output_state_field_id);
                    goto done;
                }

                memcpy(new_base, old_base, bytes_wanted);

                adjust_rcistr(new_base, old_base, &rci->shared.content);
                rci->input.destination = new_base + bytes_wanted;
            }

            rci->status = rci_status_more_input;
        }
    }

done:
    return;
}

