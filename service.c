/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
 *
 * Service managament functions
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

json_t * service_get(struct _carleon_config * config, const char * uid) {
  json_t * j_query, * j_result, * to_return, * j_service;
  int res;
  size_t index;
  
  if (config == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_get - Error no config");
    return NULL;
  }
  
  if (uid == NULL) {
    j_query = json_pack("{ss}", "table", CARLEON_TABLE_SERVICE);
  } else {
    j_query = json_pack("{sss{ss}}", "table", CARLEON_TABLE_SERVICE, "where", "cs_uid", uid);
  }
  
  if (j_query == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_get - Error allocating resources for j_query");
    return NULL;
  }
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    if (uid == NULL) {
      to_return = json_array();
      if (to_return == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "service_get - Error allocating resources for to_return");
        json_decref(j_result);
        return NULL;
      }
      json_array_foreach(j_result, index, j_service) {
        json_t * service = parse_service_from_db(j_service);
        struct _carleon_service * c_service = get_service_from_uid(config, json_string_value(json_object_get(service, "uid")));
        if (c_service != NULL) {
          json_t * tmp = c_service->c_service_command_get_list(config);
          if (json_integer_value(json_object_get(tmp, "result")) == WEBSERVICE_RESULT_OK) {
            json_object_set_new(service, "commands", json_copy(json_object_get(tmp, "commands")));
          } else {
            json_object_set_new(service, "commands", json_array());
          }
          json_decref(tmp);
          tmp = c_service->c_service_element_get_list(config);
          if (json_integer_value(json_object_get(tmp, "result")) == WEBSERVICE_RESULT_OK) {
            json_t * elt_list = json_copy(json_object_get(tmp, "element")), * elt;
            size_t index;
            
            json_array_foreach(elt_list, index, elt) {
              if (json_object_get(elt, "name") != NULL && json_is_string(json_object_get(elt, "name"))) {
                json_object_set_new(elt, "tags", service_element_get_tag(config, json_string_value(json_object_get(service, "uid")), json_string_value(json_object_get(elt, "name"))));
              }
            }
            json_object_set_new(service, "element", elt_list);
          }
          json_decref(tmp);
        } else {
          y_log_message(Y_LOG_LEVEL_ERROR, "Error getting service %s", json_string_value(json_object_get(service, "uid")));
        }
        json_array_append_new(to_return, service);
      }
      json_decref(j_result);
      return to_return;
    } else {
      if (json_array_size(j_result) > 0) {
        to_return = parse_service_from_db(json_array_get(j_result, 0));
        json_decref(j_result);
        return to_return;
      } else {
        json_decref(j_result);
        return NULL;
      }
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_get - Error executing select query");
    return NULL;
  }
}

json_t * parse_service_from_db(json_t * service) {
  if (service != NULL && json_is_object(service)) {
    return json_pack("{ssssssso}", "uid", json_string_value(json_object_get(service, "cs_uid")),
                                   "name", json_string_value(json_object_get(service, "cs_name")),
                                   "description", json_string_value(json_object_get(service, "cs_description")),
                                   "enabled", json_integer_value(json_object_get(service, "cs_enabled")) == 1 ? json_true() : json_false());
  }
  return NULL;
}

int service_enable(struct _carleon_config * config, const char * uid, const int status) {
  json_t * j_service = service_get(config, uid), * j_query;
  int res;
  
  if (j_service == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_enable - service %s not found", uid);
    return C_ERROR_NOT_FOUND;
  }
  
  if (config == NULL || uid == NULL || (status != 0 && status != 1)) {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_enable - Error input parameters");
    return C_ERROR_PARAM;
  }
  
  json_decref(j_service);
  
  j_query = json_pack("{sss{si}s{ss}}", "table", CARLEON_TABLE_SERVICE, "set", "cs_enabled", status, "where", "cs_uid", uid);
  
  if (j_query == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_enable - Error allocating resources for j_query");
    return C_ERROR_MEMORY;
  }
  
  res = h_update(config->conn, j_query, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return C_OK;
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_enable - Error executing query");
    return C_ERROR_DB;
  }
}

json_t * service_element_get_tag(struct _carleon_config * config, const char * service, const char * element) {
  json_t * j_query = json_pack("{sss{ssss}}", "table", CARLEON_TABLE_ELEMENT, "where", "cs_uid", service, "ce_name", element), * j_result, * tags;
  int res;
  
  if (j_query == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_element_get_tag - Error generating j_query");
    return NULL;
  } else {
    res = h_select(config->conn, j_query, &j_result, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      tags = json_loads(json_string_value(json_object_get(json_array_get(j_result, 0), "ce_tag")), JSON_DECODE_ANY, NULL);
      json_decref(j_result);
      if (tags == NULL) {
        return json_array();
      } else {
        return tags;
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "service_element_get_tag - Error getting tags");
      return NULL;
    }
  }
}

int service_element_add_tag(struct _carleon_config * config, const char * service, const char * element, const char * tag) {
  json_t * tags = service_element_get_tag(config, service, element), 
         * j_query = json_pack("{sss{ssss}}", 
                                "table", CARLEON_TABLE_ELEMENT, 
                                "where", 
                                  "cs_uid", service, 
                                  "ce_name", element),
         * j_result,
         * j_service = service_get(config, service);
  int res;
  
  if (j_service == NULL) {
    json_decref(tags);
    json_decref(j_query);
    return C_ERROR_NOT_FOUND;
  } else if (j_query != NULL) {
    json_decref(j_service);
    res = h_select(config->conn, j_query, &j_result, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      char * str_tags;
      json_array_append_new(tags, json_string(tag));
      str_tags = json_dumps(tags, JSON_COMPACT);
      if (json_array_size(j_result) == 0) {
        j_query = json_pack("{sss{ssssss}}", 
                            "table", CARLEON_TABLE_ELEMENT, 
                            "values",
                              "cs_uid", service,
                              "ce_name", element,
                              "ce_tag", str_tags);
      } else {
        j_query = json_pack("{sss{ss}s{ssss}}", 
                            "table", CARLEON_TABLE_ELEMENT, 
                            "set",
                              "ce_tag", str_tags,
                            "where", 
                              "cs_uid", service, 
                              "ce_name", element);
      }
      free(str_tags);
      json_decref(tags);
      if (j_query != NULL) {
        if (json_array_size(j_result) == 0) {
          res = h_insert(config->conn, j_query, NULL);
        } else {
          res = h_update(config->conn, j_query, NULL);
        }
        json_decref(j_query);
        json_decref(j_result);
        if (res == H_OK) {
          return C_OK;
        } else {
          return C_ERROR_DB;
        }
      } else {
        y_log_message(Y_LOG_LEVEL_ERROR, "service_element_add_tag - Error allocating j_query");
        return C_ERROR_MEMORY;
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "service_element_add_tag - Error executing j_query for select");
      return C_ERROR_MEMORY;
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_element_add_tag - Error allocating j_query for select");
    return C_ERROR_MEMORY;
  }
}

int service_element_remove_tag(struct _carleon_config * config, const char * service, const char * element, const char * tag) {
  json_t * tags = service_element_get_tag(config, service, element), * j_query = NULL, * j_service = service_get(config, service);
  int index, res;
  char * str_tag;
  
  if (j_service == NULL) {
    json_decref(tags);
    Ssreturn C_ERROR_NOT_FOUND;
  }
  
  json_decref(j_service);
  
  for (index = json_array_size(tags)-1; index >= 0; index--) {
    if (0 == nstrcmp(json_string_value(json_array_get(tags, index)), tag)) {
      json_array_remove(tags, index);
    }
  }
  
  str_tag = json_dumps(tags, JSON_COMPACT);
  j_query = json_pack("{sss{ss}s{ssss}}", 
                      "table", CARLEON_TABLE_ELEMENT, 
                      "set",
                        "ce_tag", str_tag,
                      "where", 
                        "cs_uid", service, 
                        "ce_name", element);
  free(str_tag);
  json_decref(tags);
  if (j_query != NULL) {
    res = h_update(config->conn, j_query, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      return C_OK;
    } else {
      return C_ERROR_DB;
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_element_remove_tag - Error allocating j_query");
    return C_ERROR_MEMORY;
  }
}

int service_element_cleanup(struct _carleon_config * config, const char * service, const char * element) {
  json_t * j_query = json_pack("{sss{ssss}}", 
                      "table", CARLEON_TABLE_ELEMENT, 
                      "where", 
                        "cs_uid", service, 
                        "ce_name", element);
  int res;
  
  if (j_query != NULL) {
    res = h_delete(config->conn, j_query, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      return C_OK;
    } else {
      return C_ERROR_DB;
    }
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "service_element_cleanup - Error allocating j_query");
    return C_ERROR_MEMORY;
  }
}

json_t * service_exec(struct _carleon_config * config, struct _carleon_service * service, const char * command, const char * element, json_t * parameters) {
  if (config != NULL && service != NULL && command != NULL) {
    return service->c_service_exec(config, command, element, parameters);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}
