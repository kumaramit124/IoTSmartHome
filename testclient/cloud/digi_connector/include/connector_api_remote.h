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

#ifndef _CONNECTOR_API_REMOTE_H
#define _CONNECTOR_API_REMOTE_H

#if (defined CONNECTOR_RCI_SERVICE)
#include "connector_types.h"
/**
* @defgroup connector_request_id_remote_config_t Remote Configuration Request IDs
* @{
*/
/**
* Remote Configuration Request ID passed to the application's callback to query or set remote configuration data.
* The class id for this connector_request_id_remote_config_t is connector_class_id_remote_config.
*/

typedef enum {
    connector_request_id_remote_config_session_start,  /**< inform callback to start remote configuration request */
    connector_request_id_remote_config_action_start,   /**< requesting callback to start query or set remote configuration data */
    connector_request_id_remote_config_group_start,    /**< requesting callback to start query or set an individual configuration group */
    connector_request_id_remote_config_group_process,  /**< requesting callback to query or set an element or field of a configuration group */
    connector_request_id_remote_config_group_end,      /**< requesting callback to end query or set an individual configuration group */
    connector_request_id_remote_config_action_end,     /**< requesting callback to end query or set remote configuration data */
    connector_request_id_remote_config_session_end,    /**< inform callback to end remote configuration request
                                                            Callback may start writing data into NVRAM for set remote configuration request.
                                                            Callback should end and release any resources used when it's done. */
    connector_request_id_remote_config_session_cancel  /**< Requesting callback to abort and cancel any query or set remote configuration request.
                                                            Callback should stop and release any resources used */
} connector_request_id_remote_config_t;
/**
* @}
*/


/**
 * @defgroup connector_remote_action_t Cloud Connector remote configuration action types
 * @{
 */
/**
 * Remote request action types
 */
typedef enum {
    connector_remote_action_set,    /**< Set remote configuration */
    connector_remote_action_query   /**< Query remote configuration */
} connector_remote_action_t;
/**
 * @}
 */

/**
 * @defgroup connector_remote_group_type_t Cloud Connector remote configuration types
 * @{
 */
/**
 * Remote request group types.
 */
typedef enum {
    connector_remote_group_setting, /**< Setting configuration */
    connector_remote_group_state    /**< State configuration */
} connector_remote_group_type_t;

/**
* @}
*/


/**
* @defgroup connector_element_access_t Cloud Connector remote configuration access types
* @{
*/
/**
* Remote Configuration Element Access types
*/
typedef enum {
    connector_element_access_read_only,     /**< Read only */
    connector_element_access_write_only,    /**< Write only */
    connector_element_access_read_write     /**< Read and write */
} connector_element_access_t;
/**
* @}
*/

#endif

#if !defined _CONNECTOR_API_H
#error  "Illegal inclusion of connector_api_remote.h. You should only include connector_api.h in user code."
#endif

#else
#error  "Illegal inclusion of connector_api_remote.h. You should only include connector_api.h in user code."
#endif
