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

#include <stdarg.h>

#define BINARY_RCI_ATTRIBUTE_BIT  UINT32_C(0x40)  /* bit 6 */

#define BINARY_RCI_NO_VALUE        UINT32_C(0xE0)
#define BINARY_RCI_TERMINATOR      UINT32_C(0xE1)

#define BINARY_RCI_ERROR_INDICATOR_BIT  UINT32_C(0x1000) /* bit 12 */

#define BINARY_RCI_FIELD_TYPE_INDICATOR_BIT UINT32_C(0x40) /* bit 6 */
#define BINARY_RCI_FIELD_ATTRIBUTE_BIT      UINT32_C(0x400) /* bit 10 */
/* #define BINARY_RCI_FIELD_ASCENDING_INDICATOR_BIT    (0x1 << 6) */

/*
 *    7 6 5 4 3 2 1 0 bit
 *    0 X X X X X X X   (0 : 0x7F)
 *    1 0 0 X X X X X   + 1 byte followed (0: 0x1FFF)
 *    1 0 1 0 0 0 0 0   + 2 bytes followed (0: 0xFFFF)
 *    1 0 1 0 0 0 0 1   + 4 bytes followed (0: 0xFFFFFFFF)
 *    1 0 1 0 0 0 1 0   + 8 bytes followed (0: 0xFFFFFFFFFFFFFFFF)
 *    1 1 0 0 - - - -   Reserved
 *    1 1 1 0 0 0 0 0   NONUM (No Value)
 *    1 1 1 0 0 0 0 1   TRM (Terminator)
 */
#define BINARY_RCI_SIZE_ALTERNATE_FLAG          UINT32_C(0x80) /* bit 7 */
#define BINARY_RCI_SIZE_MODIFIER_MASK           UINT32_C(0x60) /* bits 6:5 */
#define BINARY_RCI_SIZE_MODIFIER(value)         (rci_size_modifier_t)((value & BINARY_RCI_SIZE_MODIFIER_MASK) >> UINT32_C(5))

typedef enum {
    binary_rci_one_follow_byte,
    binary_rci_multi_follow_byte,
    binary_rci_special_value    = 3
} rci_size_modifier_t;


#define BINARY_RCI_MULTI_FOLLOW_BYTE_MASK       UINT32_C(0x03)
#define BINARY_RCI_MULTI_FOLLOW_BYTES(value)    (value & BINARY_RCI_MULTI_FOLLOW_BYTE_MASK)
#define BINARY_RCI_SET_MULTI_FOLLOW_BYTES(value) (BINARY_RCI_SIZE_ALTERNATE_FLAG | (binary_rci_multi_follow_byte << UINT32_C(5)) |value)

enum {
    binary_rci_two_follow_byte,
    binary_rci_four_follow_byte,
    binary_rci_eight_follow_byte
};

#define RCI_FLAG_GET_ALL_INSTANCES     UINT32_C(0x01)

#define RCI_NO_HINT             NULL
#define INVALID_ID              UINT_MAX
#define INVALID_INDEX           UINT_MAX
#define INVALID_COUNT           ((size_t)-1)

#define ROUND_UP(value, interval)   ((value) + -(value) % (interval))

#define UNHANDLED_CASES_ARE_INVALID default: ASSERT(connector_false); break;

static char const nul = '\0';

enum rci_ber {
    field_define(rci_ber, value, uint8_t),
    record_end(rci_ber)
};

enum rci_ber_u8 {
    field_define(rci_ber_u8, opcode, uint8_t),
    field_define(rci_ber_u8, value, uint8_t),
    record_end(rci_ber_u8)
};

enum rci_ber_u16 {
    field_define(rci_ber_u16, opcode, uint8_t),
    field_define(rci_ber_u16, value, uint16_t),
    record_end(rci_ber_u16)
};

enum rci_ber_u32 {
    field_define(rci_ber_u32, opcode, uint8_t),
    field_define(rci_ber_u32, value, uint32_t),
    record_end(rci_ber_u32)
};


enum
{
    rci_field_type_none,
    rci_field_type_string,
    rci_field_type_multiline_string,
    rci_field_type_password,
    rci_field_type_int23,
    rci_field_type_uint32,
    rci_field_type_hex32,
    rci_field_type_0x_hex32,
    rci_field_type_float,
    rci_field_type_enum,
    rci_field_type_on_off = 0x0B,
    rci_field_type_boolean,
    rci_field_type_ip4,
    rci_field_type_fqdnv4,
    rci_field_type_fqdnv6,
    rci_field_type_datetime = 0x16
};

enum {
    rci_command_query_setting = 1,
    rci_command_set_setting,
    rci_command_query_state,
    rci_command_set_state
};

typedef enum
{
    rci_session_start,
    rci_session_active,
    rci_session_lost
} rci_session_t;

typedef enum
{
    rci_status_internal_error,  /* bad code path */
    rci_status_complete,        /* all done */
    rci_status_busy,            /* user callback returned busy */
    rci_status_more_input,      /* have output buffer space to process more input */
    rci_status_flush_output,    /* need more output space, so send out the current buffer */
    rci_status_error            /* error occurred, RCI service should inform messaging layer to cancel the session */
} rci_status_t;

typedef struct
{
    uint8_t * data;
    size_t bytes;
    unsigned int flags;
} rci_service_buffer_t;

typedef struct
{
    connector_data_t * connector_ptr;
    rci_service_buffer_t input;
    rci_service_buffer_t output;
} rci_service_data_t;

typedef struct
{
    uint8_t * start;
    uint8_t * end;
    uint8_t * current;
} rci_buffer_t;

typedef enum
{
    rci_parser_state_input,
    rci_parser_state_output,
    rci_parser_state_traverse,
    rci_parser_state_error
} rci_parser_state_t;


typedef enum
{
    rci_input_state_command_id,
    rci_input_state_command_attribute,
    rci_input_state_group_id,
    rci_input_state_group_attribute,
    rci_input_state_field_id,
    rci_input_state_field_type,
    rci_input_state_field_no_value,
    rci_input_state_field_value,
    rci_input_state_done
} rci_input_state_t;

typedef enum
{
    rci_output_state_command_id,
    rci_output_state_group_id,
    rci_output_state_group_attribute,
    rci_output_state_field_id,
    rci_output_state_field_value,
    rci_output_state_field_terminator,
    rci_output_state_group_terminator,
    rci_output_state_response_done,
    rci_output_state_done
} rci_output_state_t;

typedef enum
{
    rci_traverse_state_none,
    rci_traverse_state_command_id,
    rci_traverse_state_group_id,
    rci_traverse_state_element_id,
    rci_traverse_state_element_end,
    rci_traverse_state_group_end,
    rci_traverse_state_all_groups,
    rci_traverse_state_all_group_instances,
    rci_traverse_state_all_elements
} rci_traverse_state_t;

typedef enum
{
    rci_traverse_process_group,
    rci_traverse_process_element,
    rci_traverse_process_next_instance
} rci_traverse_process_state_t;

typedef enum
{
    rci_error_state_id,
    rci_error_state_description,
    rci_error_state_hint,
    rci_error_state_callback
} rci_error_state_t;

typedef struct
{
    uint8_t * data;
    size_t length;
} rcistr_t;

typedef struct
{
    rcistr_t name;
    rcistr_t value;
} rci_attribute_t;

typedef struct
{
    rci_service_data_t * service_data;
    rci_status_t status;
    struct {
        connector_request_id_t request;
        connector_callback_status_t status;
    } callback;

    struct {
        rci_buffer_t input;
        rci_buffer_t output;
    } buffer;

    struct {
        rci_parser_state_t state;
    } parser;

    struct {
        rci_traverse_state_t state;
        rci_traverse_process_state_t process_state;
    } traverse;

    struct {
        rci_input_state_t state;
        unsigned int flag;
        uint8_t * destination;
        uint8_t storage[CONNECTOR_RCI_MAXIMUM_CONTENT_LENGTH + sizeof nul + sizeof(uint32_t)];
    } input;

    struct {
        rcistr_t content;
        rci_output_state_t state;
    } output;

    struct {
        rci_error_state_t state;
        connector_bool_t command_error;
        char const * description;
    } error;

    struct {
        rcistr_t content;

        struct {
            unsigned int id;
            unsigned int index;
        } group;

        struct {
            unsigned int id;
        } element;

        connector_element_value_t value;
        size_t string_value_length;

        connector_remote_config_t callback_data;
    } shared;
} rci_t;

#define set_rci_input_flag(rci, value)     (rci)->input.flag |= (value)
#define is_rci_input_flag(rci, value)      (((rci)->input.flag & (value)) == (value))
#define clea_rci_input_flag(rci, value)    (rci)->input.flag &= ~(value)

#define set_rci_input_state(rci, value)     (rci)->input.state = (value)
#define get_rci_input_state(rci)            (rci)->input.state

#define set_rci_traverse_state(rci, value)     (rci)->traverse.state = (value)
#define get_rci_traverse_state(rci)            (rci)->traverse.state

#define set_rci_output_state(rci, value)     (rci)->output.state = (value)
#define get_rci_output_state(rci)            (rci)->output.state

#define set_rci_error_state(rci, value)     (rci)->error.state = (value);
#define get_rci_error_state(rci)            (rci)->error.state;

#define set_rci_command_error(rci)          (rci)->error.command_error = connector_true;
#define clear_rci_command_error(rci)        (rci)->error.command_error = connector_false;

