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

#ifndef CHK_CONFIG_H_
#define CHK_CONFIG_H_

#if (defined CONNECTOR_TRANSPORT_TCP)

#if (defined CONNECTOR_VENDOR_ID)
#if (CONNECTOR_VENDOR_ID > CONNECTOR_MAX_VENDOR_ID_NUMBER) || (CONNECTOR_VENDOR_ID == 0)
    #error "Invalid CONNECTOR_VENDOR_ID in connector_config.h"
#endif
#endif

#if (defined CONNECTOR_TX_KEEPALIVE_IN_SECONDS)
#if (CONNECTOR_TX_KEEPALIVE_IN_SECONDS < MIN_TX_KEEPALIVE_INTERVAL_IN_SECONDS) || (CONNECTOR_TX_KEEPALIVE_IN_SECONDS > MAX_TX_KEEPALIVE_INTERVAL_IN_SECONDS)
    #error "Invalid CONNECTOR_TX_KEEPALIVE_IN_SECONDS value in connector_config.h."
#endif
#endif

#if (defined CONNECTOR_RX_KEEPALIVE_IN_SECONDS)
#if (CONNECTOR_RX_KEEPALIVE_IN_SECONDS < MIN_RX_KEEPALIVE_INTERVAL_IN_SECONDS) || (CONNECTOR_RX_KEEPALIVE_IN_SECONDS > MAX_RX_KEEPALIVE_INTERVAL_IN_SECONDS)
    #error "Invalid CONNECTOR_RX_KEEPALIVE_IN_SECONDS value in connector_config.h."
#endif
#endif

#if (defined CONNECTOR_WAIT_COUNT)
#if (CONNECTOR_WAIT_COUNT < WAIT_COUNT_MIN) || (CONNECTOR_WAIT_COUNT > WAIT_COUNT_MAX)
    #error "Invalid CONNECTOR_WAIT_COUNT value in connector_config.h."
#endif
#endif

#if (defined CONNECTOR_MSG_MAX_TRANSACTION)
#if (CONNECTOR_MSG_MAX_TRANSACTION < 0 || CONNECTOR_MSG_MAX_TRANSACTION > CONNECTOR_MAX_TRANSACTIONS_LIMIT)
    #error "Invalid CONNECTOR_MSG_MAX_TRANSACTION value in connector_config.h"
#endif
#endif

#endif

#if (CONNECTOR_VERSION < 0x02010000)
/* Translate macros that have been renamed */
#if (defined CONNECTOR_SM_MAX_SEGMENTS)
#define CONNECTOR_SM_MAX_RX_SEGMENTS            CONNECTOR_SM_MAX_SEGMENTS
#define CONNECTOR_SM_MAX_DATA_POINTS_SEGMENTS   CONNECTOR_SM_MAX_SEGMENTS
#endif
#endif

#if (defined CONNECTOR_RCI_MAXIMUM_CONTENT_LENGTH)
#if CONNECTOR_RCI_MAXIMUM_CONTENT_LENGTH > SIZE_MAX
    #error "Invalid CONNECTOR_RCI_MAXIMUM_CONTENT_LENGTH, it must be lower than SIZE_MAX"
#endif
#endif

#endif
