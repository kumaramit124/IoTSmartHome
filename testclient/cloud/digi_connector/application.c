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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "connector_api.h"
#include "platform.h"
#include "remote_config.h"

connector_handle_t app_handle = NULL;

extern connector_status_t app_send_put_request(connector_handle_t handle, char const *buffer, char file_mode);

extern connector_callback_status_t app_data_service_handler(connector_request_id_data_service_t const request_id, void * const data);

extern connector_callback_status_t app_firmware_handler(connector_request_id_firmware_t const request_id,
    void * const data);
extern connector_callback_status_t app_remote_config_handler(connector_request_id_remote_config_t const request_id,
    void * const data);

connector_bool_t app_connector_reconnect(connector_class_id_t const class_id, connector_close_status_t const status)
{
  connector_bool_t type;

  UNUSED_ARGUMENT(class_id);

  switch (status)
  {
    /* if either Device Cloud or our application cuts the connection, don't reconnect */
    case connector_close_status_device_terminated:
    case connector_close_status_device_stopped:
    case connector_close_status_abort:
      type = connector_false;
      break;

      /* otherwise it's an error and we want to retry */
    default:
      type = connector_true;
      break;
  }

  return type;
}


connector_callback_status_t app_connector_callback(connector_class_id_t const class_id,
    connector_request_id_t const request_id,
    void * const data)
{
  connector_callback_status_t   status = connector_callback_unrecognized;

  switch (class_id)
  {
    case connector_class_id_config:
      status = app_config_handler(request_id.config_request, data);
      break;
    case connector_class_id_operating_system:
      status = app_os_handler(request_id.os_request, data);
      break;
    case connector_class_id_network_tcp:
      status = app_network_tcp_handler(request_id.network_request, data);
      break;
    case connector_class_id_file_system:
      status = app_file_system_handler(request_id.file_system_request, data);
      break;
    case connector_class_id_data_service:
      status = app_data_service_handler(request_id.data_service_request, data);
      break;
    case connector_class_id_firmware:
      status = app_firmware_handler(request_id.firmware_request, data);
      break;
    case connector_class_id_remote_config:
      status = app_remote_config_handler(request_id.remote_config_request, data);
      break;
    case connector_class_id_status:
      status = app_status_handler(request_id.status_request, data);
      break;
    default:
      /* not supported */
      status = connector_callback_unrecognized;
      break;
  }
  return status;
}

int send_file_to_cloud(char const *buffer ,char file_mode)
{
  int return_status = 0;
  if (!app_handle)
    return -1;
  for (;;)
  {
    connector_status_t const status = app_send_put_request(app_handle, buffer, file_mode);

    switch (status)
    {
      case connector_init_error:
        {
          unsigned int const sleep_time_in_seconds = 1;
          APP_DEBUG("Going to Sleep for 1 sec, no response from server\n");
          sleep(sleep_time_in_seconds);
        }
        break;

      case connector_success:
        goto done;

      default:
        APP_DEBUG("Send data failed [%d]\n", status);
        return_status = 1;
        goto done;
    }
  }

done:
  return return_status;
}

int application_run(connector_handle_t handle)
{
  app_handle = handle;
  UNUSED_ARGUMENT(handle);
  /* No application's thread here.
   * Application has no other process.
   * main() will start connector_run() as a separate thread.
   */
  return 0; 
}

