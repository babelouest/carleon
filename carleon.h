/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
 *
 * Declarations for constants and prototypes
 *
 * Copyright 2016 Nicolas Mora <mail@babelouest.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU GENERAL PUBLIC LICENSE
 * License as published by the Free Software Foundation;
 * version 3 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU GENERAL PUBLIC LICENSE for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __CARLEON_H_
#define __CARLEON_H_

#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <dlfcn.h>
#include <jansson.h>

/** Angharad libraries **/
#include <ulfius.h>
#include <yder.h>

#define _HOEL_MARIADB
#define _HOEL_SQLITE
#include <hoel.h>

#define _CARLEON_VERSION 0.1

#define C_OK              0
#define C_ERROR           1
#define C_ERROR_MEMORY    2
#define C_ERROR_PARAM     3
#define C_ERROR_DB        4
#define C_ERROR_IO        5
#define C_ERROR_NOT_FOUND 6

#define WEBSERVICE_RESULT_ERROR     0
#define WEBSERVICE_RESULT_OK        1
#define WEBSERVICE_RESULT_NOT_FOUND 2
#define WEBSERVICE_RESULT_TIMEOUT   3

#define CARLEON_TABLE_SERVICE "c_service"
#define CARLEON_TABLE_ELEMENT "c_element"
#define CARLEON_TABLE_PROFILE "c_profile"

struct _carleon_config {
  char                    * services_path;
  struct _h_connection    * conn;
  struct _carleon_service * service_list;
};

/**
 * Structure for a service type
 * contains the handle to the library and handles for all the functions
 */
struct _carleon_service {
  char   * uid;
  void   * dl_handle;
  int      enabled;
  char   * name;
  char   * description;
  
  // dl files functions available
  json_t * (* c_service_init) (struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config);
  json_t * (* c_service_close) (struct _u_instance * instance, const char * url_prefix);
  json_t * (* c_service_enable) (struct _carleon_config * config, int status);
  json_t * (* c_service_command_get_list) (struct _carleon_config * config);
  json_t * (* c_service_element_get_list) (struct _carleon_config * config);
  json_t * (* c_service_exec) (struct _carleon_config * config, json_t * j_command);
};

// carleon core functions
int init_carleon(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config);
int close_carleon(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config);
void clean_carleon(struct _carleon_config * config);
int init_service_list(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config);
int close_service_list(struct _carleon_config * config, struct _u_instance * instance, const char * url_prefix);
void close_service(struct _carleon_service * service, struct _u_instance * instance, const char * url_prefix);
int disconnect_all_services(struct _carleon_config * config);

// services core functions
json_t * service_get(struct _carleon_config * config, const char * service);
json_t * parse_service_from_db(json_t * service);
int service_enable(struct _carleon_config * config, const char * service, const int status);
struct _carleon_service * get_service_from_uid(struct _carleon_config * config, const char * uid);
json_t * service_command_get_list(struct _carleon_service * service);
json_t * service_element_get_list(struct _carleon_service * service);
json_t * service_exec(struct _carleon_service * service, json_t * j_command);

// elements core functions
json_t * service_element_get_tag(struct _carleon_config * config, const char * service, const char * element);
int service_element_cleanup(struct _carleon_config * config, const char * service, const char * element);
int service_element_add_tag(struct _carleon_config * config, const char * service, const char * element, const char * tag);
int service_element_remove_tag(struct _carleon_config * config, const char * service, const char * element, const char * tag);

// profiles core functions
json_t * profile_list(struct _carleon_config * config);
json_t * profile_get(struct _carleon_config * config, const char * profile_id);
int profile_add(struct _carleon_config * config, const char * profile_id, json_t * profile_data);
int profile_modify(struct _carleon_config * config, const char * profile_id, json_t * profile_data);
int profile_delete(struct _carleon_config * config, const char * profile_id);

// Services callback functions
int callback_carleon_service_get (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_service_enable (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_service_element_cleanup (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_service_element_add_tag (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_service_element_remove_tag (const struct _u_request * request, struct _u_response * response, void * user_data);

// Profile callback functions
int callback_carleon_profile_list (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_profile_get (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_profile_add (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_profile_set (const struct _u_request * request, struct _u_response * response, void * user_data);
int callback_carleon_profile_remove (const struct _u_request * request, struct _u_response * response, void * user_data);

#endif