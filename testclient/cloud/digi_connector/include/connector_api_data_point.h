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
 /**
  * @file
  *  @brief Functions and prototypes for Connector Data Point related API
  *         public API
  *
  */
#ifndef _CONNECTOR_API_DATA_POINT_H
#define _CONNECTOR_API_DATA_POINT_H

#if (defined CONNECTOR_DATA_POINTS)

/**
* @defgroup connector_request_id_data_point_t Data Point Request IDs
* @{
*/
/**
 * Data Point request IDs are used to return Device Cloud response for a Data point request.
 * The status IDs are called to indicate the send complete in case the response is not requested
 * or if any error occurs before sending the data point message.
 */
typedef enum
{
    connector_request_id_data_point_binary_response,    /**< Cloud response to a binary data point request */
    connector_request_id_data_point_binary_status,      /**< reason to complete the binary data point session */
    connector_request_id_data_point_single_response,    /**< Cloud response to a single stream data point request */
    connector_request_id_data_point_single_status,      /**< reason to complete the single stream session */
    connector_request_id_data_point_multiple_response,  /**< Cloud response to a multiple stream data point request */
    connector_request_id_data_point_multiple_status     /**< reason to complete the multiple stream session */
} connector_request_id_data_point_t;
/**
* @}
*/

/**
* @defgroup connector_data_point_type_t data types for Data Points
* @{
*/
/**
* This lists the data types supported for Data Points for Devices.
*
* @see connector_data_point_request_t
*/
typedef enum
{
    connector_data_point_type_integer,   /**< a network (big endian) 32-bit two's complement integer */
    connector_data_point_type_long,      /**< a network (big endian) 64-bit two's complement integer */
    connector_data_point_type_float,     /**< a network (big endian) 32-bit IEEE754 floating point */
    connector_data_point_type_double,    /**< a network (big endian) 64-bit IEEE754 floating point */
    connector_data_point_type_string,    /**< an UTF-8 encoding (ASCII compatible) */
    connector_data_point_type_binary     /**< binary data */
} connector_data_point_type_t;
/**
* @}
*/

/**
* @defgroup connector_time_epoch_fractional_t  Structure to represent time in seconds and milliseconds.
* @{
*/
/**
* The data structure to represent seconds and milliseconds since the epoch (00:00:00 UTC on 1 January 1970).
* Devices on which the milliseconds are not available can use 0 for milliseconds.
*
* @see connector_data_point_t
*/
typedef struct
{
    uint32_t seconds;       /**< seconds since epoch */
    uint32_t milliseconds;  /**< milliseconds portion of epoch time */
} connector_time_epoch_fractional_t;
/**
* @}
*/

#if (defined CONNECTOR_HAS_64_BIT_INTEGERS)
/**
* @defgroup connector_time_epoch_whole_t  connector_time_epoch_whole_t
* @brief Structure to represent time in milliseconds.
* @{
*/
/**
* The data structure to represent milliseconds since the epoch (00:00:00 UTC on 1 January 1970).
*
* @see connector_data_point_t
*/
typedef struct
{
    uint64_t milliseconds; /**< milliseconds portion of epoch time */
} connector_time_epoch_whole_t;
/**
* @}
*/
#endif

/**
* @defgroup connector_data_point_t Data structure for non-binary data points.
* @{
*/
/**
* Data structure to make a linked list of data points to send it out in one transaction.
* All the data points in a list must belong to single stream.
* @see connector_request_data_point_single_t
* @see connector_data_stream_t
* @see connector_data_point_type_t
*/
typedef struct connector_data_point_t
{
    struct
    {
        enum
        {
            connector_data_type_native, /**< the data value is in native format */
            connector_data_type_text    /**< the data value is in ascii/text format */
        } type;     /**< data format type */

        union
        {
            union
            {
                int32_t int_value;    /**< 32-bit two's complement integer */
                #if (defined CONNECTOR_HAS_64_BIT_INTEGERS)
                int64_t long_value;   /**< 64-bit two's complement integer */
                #endif
                char * string_value;/**< a null-terminated utf-8 encoding string */
                #if (defined FLOATING_POINT_SUPPORTED)
                float float_value;  /**< 32-bit IEEE754 floating point */
                double double_value;/**< 64-bit IEEE754 floating point */
                #endif
            } native;               /**< to represent the data value in their native form */

            char * text;            /**< carries data in ascii format, a null-terminated string */
        } element;  /**< each data element */

    } data;  /**< data in each data point */

    struct
    {
        enum
        {
            connector_time_cloud,                  /**< The time is ignored and Device Cloud time is used instead. */
            connector_time_local_epoch_fractional,  /**< The time value is specified in Epoch sec/msec format. */
#if (defined CONNECTOR_HAS_64_BIT_INTEGERS)
            connector_time_local_epoch_whole,       /**< The time value is specified in Epoch milliseconds 64-bit format. */
#endif
            connector_time_local_iso8601            /**< The time value is specified in ISO 8601 string format. */
        } source;   /**< Time value format */

        union
        {
            connector_time_epoch_fractional_t since_epoch_fractional;   /**< Time since the Epoch time in seconds and milliseconds */
#if (defined CONNECTOR_HAS_64_BIT_INTEGERS)
            connector_time_epoch_whole_t since_epoch_whole;             /**< Time since the Epoch time in milliseconds */
#endif
            char * iso8601_string;                                      /**<
                                                                             A null-terminated local time in ISO 8601 format. Supported format is YYYY-MM-DDThh:mm:ss[.mmm]TZD (eg 2012-03-29T10:05:45-06:00) where:
                                                                             - YYYY = four-digit year.
                                                                             - MM = two-digit month (eg 03=March).
                                                                             - DD = two-digit day of the month (01 through 31).
                                                                             - T = a set character indicating the start of the time element.
                                                                             - hh = two digits of an hour (00 through 23, AM/PM not included).
                                                                             - mm = two digits of a minute (00 through 59).
                                                                             - ss = two digits of a second (00 through 59).
                                                                             - mmmm (optional) milliseconds (0 through 999).
                                                                             - TZD = time zone designator (Z or +hh:mm or -hh:mm), the + or - values indicate how far ahead or behind a time zone is from the UTC zone. US time zone values are as follows:
                                                                                 - EDT = -4:00
                                                                                 - EST/CDT = -5:00
                                                                                 - CST/MDT = -6:00
                                                                                 - MST/PDT = -7:00
                                                                                 - PST = -8:00
                                                                             - Some examples of valid representations:
                                                                                 - 2012-01-12T06:16:55.235Z
                                                                                 - 2012-12-22T03:16:44Z
                                                                                 - 2012-12-21T19:16:44-08:00 */
        } value;    /**< Time value data structure */

    } time;   /**< Time at the data point is captured */

    struct
    {
        enum
        {
            connector_location_type_ignore, /**< location is ignored */
            #if (defined FLOATING_POINT_SUPPORTED)
            connector_location_type_native, /**< location value is represented in its native format */
            #endif
            connector_location_type_text    /**< location value is represented in ascii */
        } type; /**< location format type */

        union
        {
            #if (defined FLOATING_POINT_SUPPORTED)
            struct
            {
                float latitude;     /**< latitude in degree */
                float longitude;    /**< longitude in degree */
                float elevation;    /**< elevation in meters */
            } native; /**< location in native format */
            #endif

            struct
            {
                char * latitude;   /**< latitude in degree (null-terminated string) */
                char * longitude;  /**< longitude in degree (null-terminated string) */
                char * elevation;  /**< elevation in meters (null-terminated string) */
            } text; /**< location in text format */

        } value; /**< location value */

    } location; /**< location where the data point is captured */

    struct
    {
        enum
        {
            connector_quality_type_ignore, /**< quality is ignored */
            connector_quality_type_native  /**< user specified data quality, an integer value */
        } type; /**< quality format type */

        int value; /**< data quality value */
    } quality; /**< user specified data point quality */

    char * description; /**< null terminated description string (optional field, set to NULL if not used) */

    struct connector_data_point_t * next; /**< Points to next data point, set to NULL if this is the last one. */
} connector_data_point_t;
/**
* @}
*/

/**
* @defgroup connector_data_stream_t Data structure for non-binary data streams.
* @{
*/
/**
* Data structure to make a linked list of data streams to send it out in one transaction.
* Each stream contains a linked list of data points
* @see connector_request_data_point_multiple_t
* @see connector_data_point_type_t
* @see connector_data_point_t
*/
typedef struct connector_data_stream_t
{
    char * stream_id;                       /**< data stream path name. On Device Cloud, the stream name will be 'device ID'/'stream_id' */
    char * unit;                            /**< null-terminated unit, optional field, set to NULL if not used */
    char * forward_to;                      /**< comma separated list of streams to replicate data points to (a null-terminated optional field, set to NULL if not used) */
    connector_data_point_type_t type;       /**< data point content type */
    connector_data_point_t * point;         /**< pointer to list of data points */
    struct connector_data_stream_t * next;  /**< Points to next data stream, set to NULL if this is the last one. */
} connector_data_stream_t;
/**
* @}
*/

/**
* @defgroup connector_request_data_point_binary_t  Binary data point of a stream.
* @{
*/
/**
* This data structure is used when the connector_initiate_action() API is called with connector_initiate_binary_point
* request id. This can be used on a device where foot print and or the network bandwidth is very limited.
*
* @see connector_request_id_data_point_t
* @see connector_initiate_action
* @see connector_initiate_data_point_binary
*/
typedef struct
{
    connector_transport_t transport;/**< transport method to use */
    void * user_context;            /**< user context to be passed back in response */

    uint32_t * request_id;          /**< pointer to where to store the session's Request ID. This value is saved by by Cloud Connector after a successful connector_initiate_action()
                                         and might be used for canceling the session. Only valid for SM protocol. Set to NULL if not desired. This field  connector_initiate_action().
                                         See @connector_initiate_session_cancel*/
    char * path;                    /**< data stream path name. On the cloud the stream name will be 'device ID'/'path' */
    void * point;                   /**< binary data of size specified in bytes_used */
    size_t bytes_used;              /**< number of bytes in the point buffer */
    connector_bool_t response_required;  /**< set to connector_true if response is needed */
} connector_request_data_point_binary_t;
/**
* @}
*/

/**
* @defgroup connector_request_data_point_single_t  Data points of a stream.
* @{
*/
/**
* This data structure is used when the connector_initiate_action() API is called with
* connector_initiate_data_point_single request id. This structure can be used to send
* multiple points which are belong to a single stream.
*
* @note If using a @ref shortmessaging transport, the number of Data Points that can be sent at once is limited by @ref CONNECTOR_SM_MAX_DATA_POINTS_SEGMENTS.
*
* @see connector_request_id_data_point_t
* @see connector_data_point_t
* @see connector_initiate_action
* @see connector_initiate_data_point_single
* @see connector_data_point_type_t
*/
typedef struct
{
    connector_transport_t transport;    /**< transport method to use */
    void * user_context;                /**< user context to be passed back in response */

    uint32_t * request_id;              /**< pointer to where to store the session's Request ID. This value is saved by by Cloud Connector after a successful connector_initiate_action()
                                             and might be used for canceling the session. Only valid for SM protocol. Set to NULL if not desired. This field  connector_initiate_action().
                                             See @connector_initiate_session_cancel*/
    char * path;                        /**< data stream path name. On the cloud the stream name will be 'device ID'/'path' */
    char * unit;                        /**< null-terminated unit, optional field, set to NULL if not used */
    char * forward_to;                  /**< comma separated list of streams to replicate data points to (a null-terminated optional field, set to NULL if not used) */
    connector_data_point_t * point;     /**< pointer to list of data points */
    connector_bool_t response_required; /**< set to connector_true if response is needed */
    connector_data_point_type_t type;   /**< data point content type */
} connector_request_data_point_single_t;
/**
* @}
*/

/**
* @defgroup connector_request_data_point_multiple_t  Data points of multiple streams.
* @{
*/
/**
* This data structure is used when the connector_initiate_action() API is called with
* connector_initiate_data_point_multiple request id. This structure can be used to send
* multiple points which are belong to multiple streams.
*
* @note If using a @ref shortmessaging transport, the number of Data Points that can be sent at once is limited by @ref CONNECTOR_SM_MAX_DATA_POINTS_SEGMENTS.
*
* @see connector_request_id_data_point_t
* @see connector_data_stream_t
* @see connector_initiate_action
* @see connector_initiate_data_point_multiple
*/
typedef struct
{
    connector_transport_t transport;    /**< transport method to use */
    void * user_context;                /**< user context to be passed back in response */

    uint32_t * request_id;              /**< pointer to where to store the session's Request ID. This value is saved by by Cloud Connector after a successful connector_initiate_action()
                                             and might be used for canceling the session. Only valid for SM protocol. Set to NULL if not desired. This field  connector_initiate_action().
                                             See @connector_initiate_session_cancel*/
    connector_data_stream_t * stream;   /**< pointer to list of data streams */
    connector_bool_t response_required; /**< set to connector_true if response is needed */
} connector_request_data_point_multiple_t;
/**
* @}
*/

/**
* @defgroup connector_data_point_response_t Carries Device Cloud response.
* @{
*/
/**
* The data in the callback with request id connector_request_id_data_point_binary_response and
* connector_request_id_data_point_single_response will point to this data structure. The callback
* is called when the Connector receives a response from Device Cloud.
*
* @see connector_request_id_data_point_t
* @see connector_initiate_data_point_binary
* @see connector_initiate_data_point_single
*/
typedef struct
{
    connector_transport_t CONST transport;  /**< transport method used */
    void * user_context;                    /**< user context passed in request */

    enum
    {
        connector_data_point_response_success,      /**< sent successfully */
        connector_data_point_response_bad_request,  /**< data point request was bad */
        connector_data_point_response_unavailable,  /**< service not available at this time, try later */
        connector_data_point_response_cloud_error   /**< Device Cloud encountered error while handling the request */
    } CONST response; /**< Device Cloud response code */

    char const * CONST hint;                /**< error hint returned from Device Cloud, NULL if success or hint not available */
} connector_data_point_response_t;
/**
* @}
*/

/**
* @defgroup connector_data_point_status_t Carries data point session status.
* @{
*/
/**
* The data in the callback with request id connector_request_id_data_point_binary_status,
* connector_request_id_data_point_single_status and connector_request_id_data_point_multiple_status
* will point to this data structure. The callback is called when session completes either due to error
* or the response is not requested.
*
* @see connector_request_id_data_point_t
* @see connector_initiate_data_point_binary
* @see connector_initiate_data_point_single
* @see connector_initiate_data_point_multiple
*/
typedef struct
{
    connector_transport_t CONST transport;    /**< transport method used */
    void * user_context;                      /**< user context passed in request */

    enum
    {
        connector_data_point_status_complete,      /**< response is not requested and session is complete successfully */
        connector_data_point_status_cancel,        /**< session is cancelled by the user */
        connector_data_point_status_timeout,       /**< session timed out */
        connector_data_point_status_invalid_data,  /**< the part of the data passed in initiate action is not valid */
        connector_data_point_status_session_error  /**< error from lower communication layer  */
    } CONST status;       /**< reason for end of session */

    connector_session_error_t CONST session_error; /**< lower communication layer error code */

} connector_data_point_status_t;
/**
* @}
*/
#endif

#if !defined _CONNECTOR_API_H
#error  "Illegal inclusion of connector_api_data_point.h. You should only include connector_api.h in user code."
#endif

#else
#error  "Illegal inclusion of connector_api_data_point.h. You should only include connector_api.h in user code."
#endif

