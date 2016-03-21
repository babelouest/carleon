/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
 *
 * Profile managament functions
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

json_t * profile_list(struct _carleon_config * config) {
  json_t * j_query, * j_result, * profile, * to_return;
  int res;
  size_t index;
  
  if (config == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "profile_list - Error no config");
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
  
  j_query = json_pack("{sss[s]}", "table", CARLEON_TABLE_PROFILE, "columns", "cp_name");
  
  if (j_query == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "profile_list - Error allocating resources for j_query");
    return NULL;
  }
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    to_return = json_array();
    if (to_return == NULL) {
      y_log_message(Y_LOG_LEVEL_ERROR, "profile_list - Error allocating resources for to_return");
      json_decref(j_result);
      return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
    json_array_foreach(j_result, index, profile) {
      json_array_append_new(to_return, json_copy(json_object_get(profile, "cp_name")));
    }
    json_decref(j_result);
    return json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "list", to_return);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

json_t * profile_get(struct _carleon_config * config, const char * profile_id) {
  json_t * j_query, * j_result, * profile;
  int res;
  
  if (config == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "profile_get - Error no config");
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
  
  j_query = json_pack("{sss{ss}}", "table", CARLEON_TABLE_PROFILE, "where", "cp_name", profile_id);
  
  if (j_query == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "profile_get - Error allocating resources for j_query");
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    if (json_array_size(j_result) == 0) {
      json_decref(j_result);
      return json_pack("{si}", "result", WEBSERVICE_RESULT_NOT_FOUND);
    } else {
      profile = json_loads(json_string_value(json_object_get(json_array_get(j_result, 0), "cp_data")), JSON_DECODE_ANY, NULL);
      json_decref(j_result);
      if (profile == NULL) {
        profile = json_object();
        if (profile == NULL) {
          y_log_message(Y_LOG_LEVEL_ERROR, "profile_get - Error allocating resources for profile");
          return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
        }
      }
      return json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "profile", profile);
    }
  } else {
    return NULL;
  }
}

int profile_modify(struct _carleon_config * config, const char * profile_id, json_t * profile_data) {
  json_t * profile = profile_get(config, profile_id), * j_query;
  char * str_data;
  int res;
  
  if (profile_data == NULL || profile_id == NULL) {
    json_decref(profile);
    return C_ERROR_PARAM;
  }
  
  if (profile != NULL && json_integer_value(json_object_get(profile, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
    json_decref(profile);
    str_data = json_dumps(profile_data, JSON_COMPACT);
    j_query = json_pack("{sss{ssss}}", "table", CARLEON_TABLE_PROFILE, "values", "cp_data", str_data, "cp_name", profile_id);
    free(str_data);
    res = h_insert(config->conn, j_query, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      return C_OK;
    } else {
      return C_ERROR_DB;
    }
  } else if (profile != NULL && json_integer_value(json_object_get(profile, "result")) == WEBSERVICE_RESULT_OK) {
    json_decref(profile);
    str_data = json_dumps(profile_data, JSON_COMPACT);
    j_query = json_pack("{sss{ss}s{ss}}", "table", CARLEON_TABLE_PROFILE, "set", "cp_data", str_data, "where", "cp_name", profile_id);
    free(str_data);
    res = h_update(config->conn, j_query, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      return C_OK;
    } else {
      return C_ERROR_DB;
    }
  } else {
    json_decref(profile);
    return WEBSERVICE_RESULT_ERROR;
  }
}

int profile_delete(struct _carleon_config * config, const char * profile_id) {
  json_t * profile = profile_get(config, profile_id), * j_query;
  int res;
  
  if (profile_id == NULL) {
    json_decref(profile);
    return C_ERROR_PARAM;
  }
  
  if (profile != NULL && json_integer_value(json_object_get(profile, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
    json_decref(profile);
    return C_ERROR_NOT_FOUND;
  } else if (profile != NULL && json_integer_value(json_object_get(profile, "result")) == WEBSERVICE_RESULT_OK) {
    json_decref(profile);
    j_query = json_pack("{sss{ss}}", "table", CARLEON_TABLE_PROFILE, "where", "cp_name", profile_id);
    res = h_delete(config->conn, j_query, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      return C_OK;
    } else {
      return C_ERROR_DB;
    }
  } else {
    json_decref(profile);
    return WEBSERVICE_RESULT_ERROR;
  }
}