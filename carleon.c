/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
 *
 * Initialization and http callback functions
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

#include "carleon.h"

int init_carleon(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config) {
  if (instance != NULL && url_prefix != NULL && config != NULL) {
    
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/service/", NULL, NULL, NULL, &callback_carleon_service_get, (void*)config);
    ulfius_add_endpoint_by_val(instance, "PUT", url_prefix, "/service/@service_uid/enable/@enable_value", NULL, NULL, NULL, &callback_carleon_service_enable, (void*)config);
    ulfius_add_endpoint_by_val(instance, "PUT", url_prefix, "/service/@service_uid/@element_id/cleanup", NULL, NULL, NULL, &callback_carleon_service_element_cleanup, (void*)config);
    ulfius_add_endpoint_by_val(instance, "PUT", url_prefix, "/service/@service_uid/@element_id/@tag", NULL, NULL, NULL, &callback_carleon_service_element_add_tag, (void*)config);
    ulfius_add_endpoint_by_val(instance, "DELETE", url_prefix, "/service/@service_uid/@element_id/@tag", NULL, NULL, NULL, &callback_carleon_service_element_remove_tag, (void*)config);

    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/profile", NULL, NULL, NULL, &callback_carleon_profile_list, (void*)config);
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/profile/@profile_id", NULL, NULL, NULL, &callback_carleon_profile_get, (void*)config);
    ulfius_add_endpoint_by_val(instance, "PUT", url_prefix, "/profile/@profile_id", NULL, NULL, NULL, &callback_carleon_profile_set, (void*)config);
    ulfius_add_endpoint_by_val(instance, "DELETE", url_prefix, "/profile/@profile_id", NULL, NULL, NULL, &callback_carleon_profile_remove, (void*)config);
    
    if (init_service_list(instance, url_prefix, config) == C_OK) {
      y_log_message(Y_LOG_LEVEL_INFO, "carleon is available on prefix %s", url_prefix);
      return C_OK;
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "init_carleon - Error initializing services list");
      return C_ERROR;
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "init_carleon - Error input parameters");
    return C_ERROR_PARAM;
  }
}

int close_carleon(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config) {
  if (instance != NULL) {
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/service/");
    ulfius_remove_endpoint_by_val(instance, "PUT", url_prefix, "/service/@service_uid/enable/@enable_value");
    ulfius_remove_endpoint_by_val(instance, "PUT", url_prefix, "/service/@service_uid/@element_id/cleanup");
    ulfius_remove_endpoint_by_val(instance, "POST", url_prefix, "/service/@service_uid/@element_id/@tag");
    ulfius_remove_endpoint_by_val(instance, "DELETE", url_prefix, "/service/@service_uid/@element_id/@tag");

    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/profile");
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/profile/@profile_id");
    ulfius_remove_endpoint_by_val(instance, "PUT", url_prefix, "/profile/@profile_id");
    ulfius_remove_endpoint_by_val(instance, "DELETE", url_prefix, "/profile/@profile_id");
    
    if (close_service_list(config, instance, url_prefix) == C_OK) {
      y_log_message(Y_LOG_LEVEL_INFO, "closing carleon");
      return C_OK;
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "close_carleon - Error closing services list");
      return C_ERROR;
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "close_carleon - Error input parameters");
    return C_ERROR_PARAM;
  }
}

int init_service_list(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config) {
  json_t * j_query, * j_result, * service_handshake, * service_list, * service;
  DIR * services_directory;
  struct dirent * in_file;
  char * file_path;
  void * file_handle;
  int res;
  size_t nb_service = 0, index;
  
  config->service_list = malloc(sizeof(struct _carleon_service));
  
  if (config->conn != NULL && config->services_path != NULL && config->service_list != NULL) {
    config->service_list->uid = NULL;
    config->service_list->dl_handle = NULL;
    config->service_list->enabled = 0;
    config->service_list->name = NULL;
    config->service_list->description = NULL;
    
    // Disable all types in the database
    j_query = json_object();
    if (j_query == NULL) {
      y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error allocating resources for j_query");
      return C_ERROR_MEMORY;
    }
    json_object_set_new(j_query, "table", json_string(CARLEON_TABLE_SERVICE));
    json_object_set_new(j_query, "set", json_pack("{ss}", "cs_enabled", "0"));
    res = h_update(config->conn, j_query, NULL);
    if (res != H_OK) {
      y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error updating table %s", CARLEON_TABLE_SERVICE);
      return C_ERROR_DB;
    }
    json_decref(j_query);
    
    // read module_path and load modules
    if (NULL == (services_directory = opendir (config->services_path))) {
      y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error reading libraries folder %s", config->services_path);
      return C_ERROR_IO;
    }
    
    while ((in_file = readdir(services_directory))) {
      if (!strcmp (in_file->d_name, ".")) {
        continue;
      }
      if (!strcmp (in_file->d_name, "..")) {
        continue;
      }
      
      file_path = msprintf("%s/%s", config->services_path, in_file->d_name);
      
      if (file_path == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error allocating resources for file_path");
        return C_ERROR_MEMORY;
      }
      
      file_handle = dlopen(file_path, RTLD_LAZY);
      
      if (file_handle != NULL) {
        struct _carleon_service cur_service;
        
        dlerror();
        cur_service.dl_handle = file_handle;
        *(void **) (&cur_service.c_service_init) = dlsym(cur_service.dl_handle, "c_service_init");
        *(void **) (&cur_service.c_service_close) = dlsym(cur_service.dl_handle, "c_service_close");
        *(void **) (&cur_service.c_service_enable) = dlsym(cur_service.dl_handle, "c_service_enable");
        *(void **) (&cur_service.c_service_command_get_list) = dlsym(cur_service.dl_handle, "c_service_command_get_list");
        *(void **) (&cur_service.c_service_element_get_list) = dlsym(cur_service.dl_handle, "c_service_element_get_list");
        *(void **) (&cur_service.c_service_exec) = dlsym(cur_service.dl_handle, "c_service_exec");
        
        if ((cur_service.c_service_init != NULL) && (cur_service.c_service_close != NULL) && (cur_service.c_service_enable != NULL) && 
            (cur_service.c_service_command_get_list != NULL) && (cur_service.c_service_element_get_list != NULL) && (cur_service.c_service_exec)) {
          y_log_message(Y_LOG_LEVEL_INFO, "Adding service from file %s", file_path);
          service_handshake = (*cur_service.c_service_init)(instance, url_prefix, config);
          cur_service.uid = nstrdup(json_string_value(json_object_get(service_handshake, "uid")));
          cur_service.name = nstrdup(json_string_value(json_object_get(service_handshake, "name")));
          cur_service.description = nstrdup(json_string_value(json_object_get(service_handshake, "description")));
          json_decref(service_handshake);
          service_handshake = NULL;
          
          if (cur_service.uid != NULL && cur_service.name != NULL && cur_service.description != NULL) {
            nb_service++;
            config->service_list = realloc(config->service_list, (nb_service+1)*sizeof(struct _carleon_service));
            if (config->service_list == NULL) {
              y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error allocating resources for service_list");
              close_service_list(config, instance, url_prefix);
              return C_ERROR_MEMORY;
            }
            config->service_list[nb_service - 1].uid = cur_service.uid;
            config->service_list[nb_service - 1].name = cur_service.name;
            config->service_list[nb_service - 1].description = cur_service.description;
            
            config->service_list[nb_service - 1].dl_handle = cur_service.dl_handle;
            config->service_list[nb_service - 1].c_service_init = cur_service.c_service_init;
            config->service_list[nb_service - 1].c_service_close = cur_service.c_service_close;
            config->service_list[nb_service - 1].c_service_enable = cur_service.c_service_enable;
            config->service_list[nb_service - 1].c_service_command_get_list = cur_service.c_service_command_get_list;
            config->service_list[nb_service - 1].c_service_element_get_list = cur_service.c_service_element_get_list;
            config->service_list[nb_service - 1].c_service_exec = cur_service.c_service_exec;

            config->service_list[nb_service].uid = NULL;
            config->service_list[nb_service].name = NULL;
            config->service_list[nb_service].description = NULL;
            config->service_list[nb_service].dl_handle = NULL;
            
            // Insert or update device type in database
            j_query = json_object();
            if (j_query == NULL) {
              y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error allocating resources for j_query");
              return C_ERROR_DB;
            }
            json_object_set_new(j_query, "table", json_string(CARLEON_TABLE_SERVICE));
            json_object_set_new(j_query, "where", json_pack("{ss}", "cs_uid", cur_service.uid));
            res = h_select(config->conn, j_query, &j_result, NULL);
            json_decref(j_query);
            if (res != H_OK) {
              y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error getting type");
            } else {
              j_query = json_object();
              if (j_query == NULL) {
                y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error allocating resources for j_query");
                return C_ERROR_MEMORY;
              }
              json_object_set_new(j_query, "table", json_string(CARLEON_TABLE_SERVICE));
              if (json_array_size(j_result) == 0) {
                // Insert new device_type
                json_object_set_new(j_query, "values", json_pack("{sssssssi}", 
                                                        "cs_uid", cur_service.uid, 
                                                        "cs_name", cur_service.name, 
                                                        "cs_description", cur_service.description, 
                                                        "cs_enabled", 1
                                                      ));
                res = h_insert(config->conn, j_query, NULL);
              } else {
                // Update existing device type
                json_object_set_new(j_query, "set", json_pack("{sssssi}", 
                                                              "cs_name", cur_service.name, 
                                                              "cs_description", cur_service.description, 
                                                              "cs_enabled", 1
                                                            ));
                json_object_set_new(j_query, "where", json_pack("{ss}", "cs_uid", cur_service.uid));
                res = h_update(config->conn, j_query, NULL);
              }
              json_decref(j_query);
              json_decref(j_result);
              if (res != H_OK) {
                y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error setting device types in database");
              }
            }
          } else {
            close_service(&cur_service, instance, url_prefix);
            y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error handshake for module %s", file_path);
          }
        } else {
          y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error getting all function handles for module %s: ", file_path);
        }
      }
      free(file_path);
    }
    closedir(services_directory);
    
    // Connect all devices that are marked connected in the database
    service_list = service_get(config, NULL);
    if (service_list != NULL) {
      json_array_foreach(service_list, index, service) {
        if (json_object_get(service, "connected") == json_true()) {
          int res = service_enable(config, json_string_value(json_object_get(service, "uid")), 1);
          if (res == C_OK) {
            y_log_message(Y_LOG_LEVEL_INFO, "Service %s connected", json_string_value(json_object_get(service, "name")));
          } else {
            y_log_message(Y_LOG_LEVEL_ERROR, "Error connecting service %s, reason: %d", json_string_value(json_object_get(service, "name")), res);
          }
        }
      }
      json_decref(service_list);
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error getting service list");
      return C_ERROR_DB;
    }
    return C_OK;
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "init_service_list - Error input parameters");
    return C_ERROR_PARAM;
  }
}

struct _carleon_service * get_service_from_uid(struct _carleon_config * config, const char * uid) {
  int i;
  
  if (config == NULL || uid == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "get_service_from_uid - Error input parameters");
    return NULL;
  }
  
  for (i=0; config->service_list[i].uid != NULL; i++) {
    if (0 == strcmp(config->service_list[i].uid, uid)) {
      return (config->service_list + i);
    }
  }
  return NULL;
}

void clean_carleon(struct _carleon_config * config) {
  free(config->services_path);
  free(config);
}

int close_service_list(struct _carleon_config * config, struct _u_instance * instance, const char * url_prefix) {
  int i;
  
  if (config == NULL || config->service_list == NULL) {
    return C_ERROR_PARAM;
  } else {
    for (i=0; config->service_list != NULL && config->service_list[i].uid != NULL; i++) {
      close_service((config->service_list + i), instance, url_prefix);
    }
    free(config->service_list);
    config->service_list = NULL;
    return C_OK;
  }
}

void close_service(struct _carleon_service * service, struct _u_instance * instance, const char * url_prefix) {
  json_t * j_result = service->c_service_close(instance, url_prefix);
  
  if (j_result == NULL || json_integer_value(json_object_get(j_result, "result")) != WEBSERVICE_RESULT_OK) {
    y_log_message(Y_LOG_LEVEL_ERROR, "close_service - Error closing service %s", service->name);
  }
  json_decref(j_result);
  
  dlclose(service->dl_handle);
  free(service->uid);
  free(service->name);
  free(service->description);
}

int callback_carleon_service_get (const struct _u_request * request, struct _u_response * response, void * user_data) {
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_service_get - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    response->json_body = service_get((struct _carleon_config *)user_data, NULL);
    if (response->json_body == NULL) {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_service_get - Error getting service list, aborting");
      response->status = 500;
    }
    return U_OK;
  }
}

int callback_carleon_service_enable (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int res;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_service_get - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    res = service_enable((struct _carleon_config *)user_data, u_map_get(request->map_url, "service_uid"), 0 == strcmp(u_map_get(request->map_url, "enable_value"), "1")?1:0);
    if (res == C_OK) {
      return U_OK;
    } else if (res == C_ERROR_NOT_FOUND) {
      ulfius_set_json_response(response, 404, json_pack("{ss}", "error", "service not found"));
    } else if (res == C_ERROR_PARAM) {
      ulfius_set_json_response(response, 400, json_pack("{ss}", "error", "enable_value must be 0 or 1"));
    } else {
      response->status = 500;
    }
    return U_OK;
  }
}

int callback_carleon_service_element_add_tag (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int res;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_service_element_add_tag - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    res = service_element_add_tag((struct _carleon_config *)user_data, u_map_get(request->map_url, "service_uid"), u_map_get(request->map_url, "element_id"), u_map_get(request->map_url, "tag"));
    if (res == C_OK) {
      return U_OK;
    } else if (res == C_ERROR_NOT_FOUND) {
      ulfius_set_json_response(response, 404, json_pack("{ss}", "error", "service not found"));
    } else if (res == C_ERROR_PARAM) {
      ulfius_set_json_response(response, 400, json_pack("{ss}", "error", "enable_value must be 0 or 1"));
    } else {
      response->status = 500;
    }
    return U_OK;
  }
}

int callback_carleon_service_element_remove_tag (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int res;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_service_element_remove_tag - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    res = service_element_remove_tag((struct _carleon_config *)user_data, u_map_get(request->map_url, "service_uid"), u_map_get(request->map_url, "element_id"), u_map_get(request->map_url, "tag"));
    if (res == C_OK) {
      return U_OK;
    } else if (res == C_ERROR_NOT_FOUND) {
      ulfius_set_json_response(response, 404, json_pack("{ss}", "error", "service not found"));
    } else if (res == C_ERROR_PARAM) {
      ulfius_set_json_response(response, 400, json_pack("{ss}", "error", "enable_value must be 0 or 1"));
    } else {
      response->status = 500;
    }
    return U_OK;
  }
}

int callback_carleon_service_element_cleanup (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int res;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_service_element_cleanup - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    res = service_element_cleanup((struct _carleon_config *)user_data, u_map_get(request->map_url, "service_uid"), u_map_get(request->map_url, "element_id"));
    if (res == C_OK) {
      return U_OK;
    } else if (res == C_ERROR_NOT_FOUND) {
      ulfius_set_json_response(response, 404, json_pack("{ss}", "error", "service not found"));
    } else if (res == C_ERROR_PARAM) {
      ulfius_set_json_response(response, 400, json_pack("{ss}", "error", "enable_value must be 0 or 1"));
    } else {
      response->status = 500;
    }
    return U_OK;
  }
}

int callback_carleon_profile_list (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * j_list;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_profile_list - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    j_list = profile_list((struct _carleon_config *)user_data);
    if (json_integer_value(json_object_get(j_list, "result")) != WEBSERVICE_RESULT_OK) {
      response->status = 500;
    } else {
      response->json_body = json_copy(json_object_get(j_list, "list"));
    }
    json_decref(j_list);
    return U_OK;
  }
}

int callback_carleon_profile_get (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * j_profile;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_profile_list - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    j_profile = profile_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "profile_id"));
    if (json_integer_value(json_object_get(j_profile, "result")) == WEBSERVICE_RESULT_OK) {
      response->json_body = json_copy(json_object_get(j_profile, "profile"));
    } else if (json_integer_value(json_object_get(j_profile, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
      response->status = 404;
    } else {
      response->status = 500;
    }
    json_decref(j_profile);
    return U_OK;
  }
}

int callback_carleon_profile_set (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int res;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_profile_list - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    res = profile_modify((struct _carleon_config *)user_data, u_map_get(request->map_url, "profile_id"), request->json_body);
    if (res == C_ERROR_PARAM) {
      response->status = 400;
    } else if (res != C_OK) {
      response->status = 500;
    }
    return U_OK;
  }
}

int callback_carleon_profile_remove (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int res;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_carleon_profile_list - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    res = profile_delete((struct _carleon_config *)user_data, u_map_get(request->map_url, "profile_id"));
    if (res == C_ERROR_PARAM) {
      response->status = 400;
    } else if (res == C_ERROR_NOT_FOUND) {
      response->status = 404;
    } else if (res != C_OK) {
      response->status = 500;
    }
    return U_OK;
  }
}