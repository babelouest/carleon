/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
 *
 * Motion service module
 * Used to show images taken and real-time stream of a motion application
 *
 * library MagickCore is required to install
 * sudo apt-get install libmagickcore-dev
 * 
 * Copyright 2016 Nicolas Mora <mail@babelouest.org>
 *
 * Licence GPL V3
 *
 */

#include <sys/types.h>
#include <pthread.h>
#include <dirent.h>
#include <jansson.h>
#include <ulfius.h>
#include <orcania.h>
#include <yder.h>
#include <magick/MagickCore.h>
#include "../carleon.h"

/**
 * Some declarations
 */
json_t * service_motion_get(struct _carleon_config * config, const char * service_name);
int has_service_motion(struct _carleon_config * config, const char * service_name);

/**
 * Return a json array containing errors in the service_motion parameter
 * r an empty array if no error
 */
json_t * is_service_motion_valid(struct _carleon_config * config, json_t * service_motion, int add) {
  json_t * to_return = json_array(), * element;
  size_t index;
  
  if (to_return == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "is_service_motion_valid - Error allocating resources for to_return");
  } else {
    if (!json_is_object(service_motion)) {
      json_array_append_new(to_return, json_pack("{ss}", "service_motion", "service_motion must be a json object"));
    } else {
      if (add) {
        if (json_object_get(service_motion, "name") == NULL || !json_is_string(json_object_get(service_motion, "name")) || json_string_length(json_object_get(service_motion, "name")) > 64) {
          json_array_append_new(to_return, json_pack("{ss}", "service_motion", "name is mandatory and must be a string up to 64 characters"));
        }
        
        if (has_service_motion(config, json_string_value(json_object_get(service_motion, "name")))) {
          json_array_append_new(to_return, json_pack("{ss}", "service_motion", "A service with the same name already exist"));
        }
      }
      
      if (json_object_get(service_motion, "description") != NULL && (!json_is_string(json_object_get(service_motion, "description")) || json_string_length(json_object_get(service_motion, "description")) > 128)) {
        json_array_append_new(to_return, json_pack("{ss}", "service_motion", "description must be a string up to 128 characters"));
      }
      
      if (json_object_get(service_motion, "config_uri") != NULL && (!json_is_string(json_object_get(service_motion, "config_uri")) || json_string_length(json_object_get(service_motion, "config_uri")) > 512)) {
        json_array_append_new(to_return, json_pack("{ss}", "service_motion", "config_uri must be a string up to 512 characters"));
      }
      
      if (json_object_get(service_motion, "file_list") != NULL && !json_is_array(json_object_get(service_motion, "file_list"))) {
        json_array_append_new(to_return, json_pack("{ss}", "service_motion", "file_list must be an array of file paths with the format {name:string, path:string}"));
      } else if (json_object_get(service_motion, "file_list") != NULL) {
        json_array_foreach(json_object_get(service_motion, "file_list"), index, element) {
          if (!json_is_object(element)) {
            json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a file_list element must be an object with the format {name:string, path:string}"));
          } else {
            if (json_object_get(element, "name") == NULL || !json_is_string(json_object_get(element, "name")) || json_string_length(json_object_get(element, "name")) > 64) {
              json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a file_list name is mandatory and must be a string up to 64 characters"));
            }

            if (json_object_get(element, "path") == NULL || !json_is_string(json_object_get(element, "path")) || json_string_length(json_object_get(element, "path")) > 512) {
              json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a file_list path is mandatory and must be a string up to 512 characters"));
            }

            if (json_object_get(element, "thumbnail_path") == NULL || !json_is_string(json_object_get(element, "thumbnail_path")) || json_string_length(json_object_get(element, "thumbnail_path")) > 512) {
              json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a file_list thumnail_path is mandatory and must be a string up to 512 characters"));
            }
          }
        }
      }
      
      if (json_object_get(service_motion, "stream_list") != NULL && !json_is_array(json_object_get(service_motion, "stream_list"))) {
        json_array_append_new(to_return, json_pack("{ss}", "service_motion", "stream_list must be an array of file paths with the format {name:string, uri:string}"));
      } else if (json_object_get(service_motion, "stream_list") != NULL) {
        json_array_foreach(json_object_get(service_motion, "stream_list"), index, element) {
          if (!json_is_object(element)) {
            json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a stream_list element must be an object with the format {name:string, uri:string}"));
          } else {
            if (json_object_get(element, "name") == NULL || !json_is_string(json_object_get(element, "name")) || json_string_length(json_object_get(element, "name")) > 64) {
              json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a stream_list name is mandatory and must be a string up to 64 characters"));
            }

            if (json_object_get(element, "uri") == NULL || !json_is_string(json_object_get(element, "uri")) || json_string_length(json_object_get(element, "uri")) > 512) {
              json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a stream_list uri is mandatory and must be a string up to 512 characters"));
            }

            if (json_object_get(element, "snapshot_uri") != NULL && (!json_is_string(json_object_get(element, "snapshot_uri")) || json_string_length(json_object_get(element, "snapshot_uri")) > 512)) {
              json_array_append_new(to_return, json_pack("{ss}", "service_motion", "a stream_list snapshot_uri must be a string up to 512 characters"));
            }
          }
        }
      }
    }
  }
  return to_return;
}

/**
 * Add a new motion service
 */
json_t * service_motion_add(struct _carleon_config * config, json_t * service_motion) {
  json_t * j_reasons, * j_query, * element;
  int res;
  size_t index;
  
  if (config == NULL) {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  } else if (service_motion == NULL) {
    return json_pack("{siss}", "result", WEBSERVICE_RESULT_PARAM, "reason", "no service_motion specified");
  } else {
    j_reasons = is_service_motion_valid(config, service_motion, 1);
    if (j_reasons == NULL || json_array_size(j_reasons) == 0) {
      json_decref(j_reasons);
      j_query = json_pack("{sss{ssssss}}",
                          "table", 
                            "c_service_motion", 
                          "values",
                            "csm_name",
                            json_string_value(json_object_get(service_motion, "name")),
                            "csm_description",
                            json_object_get(service_motion, "description")!=NULL?json_string_value(json_object_get(service_motion, "description")):"",
                            "csm_config_uri",
                            json_object_get(service_motion, "config_uri")!=NULL?json_string_value(json_object_get(service_motion, "config_uri")):"");
      res = h_insert(config->conn, j_query, NULL);
      json_decref(j_query);
      if (res == H_OK) {
        json_array_foreach(json_object_get(service_motion, "file_list"), index, element) {
          j_query = json_pack("{sss{ssssssss}}",
                              "table", 
                                "c_service_motion_file_list", 
                              "values",
                                "csmfl_name",
                                json_string_value(json_object_get(element, "name")),
                                "csm_name",
                                json_string_value(json_object_get(service_motion, "name")),
                                "csmfl_path",
                                json_string_value(json_object_get(element, "path")),
                                "csmfl_thumbnail_path",
                                json_string_value(json_object_get(element, "thumbnail_path")));
          res = h_insert(config->conn, j_query, NULL);
          json_decref(j_query);
          if (res != H_OK) {
            y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_add - Error adding in table c_service_motion_file_list");
          }
        }

        json_array_foreach(json_object_get(service_motion, "stream_list"), index, element) {
          j_query = json_pack("{sss{ssssssss}}",
                              "table", 
                                "c_service_motion_stream", 
                              "values",
                                "csms_name",
                                json_string_value(json_object_get(element, "name")),
                                "csm_name",
                                json_string_value(json_object_get(service_motion, "name")),
                                "csms_uri",
                                json_string_value(json_object_get(element, "uri")),
                                "csms_snapshot_uri",
                                json_object_get(element, "snapshot_uri")!=NULL?json_string_value(json_object_get(element, "snapshot_uri")):"");
          res = h_insert(config->conn, j_query, NULL);
          json_decref(j_query);
          if (res != H_OK) {
            y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_add - Error adding in table c_service_motion_stream");
          }
        }
        
        return json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
      } else {
        y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_add - Error adding in table c_service_motion");
        return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      }
    } else {
      return json_pack("{siso}", "result", WEBSERVICE_RESULT_PARAM, "reason", j_reasons);
    }
  }
}

/**
 * Update an existing motion service
 */
json_t * service_motion_set(struct _carleon_config * config, const char * name, json_t * service_motion) {
  json_t * j_reasons, * j_query, * element;
  int res;
  size_t index;
  
  if (config == NULL) {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  } else if (service_motion == NULL) {
    return json_pack("{siss}", "result", WEBSERVICE_RESULT_PARAM, "reason", "no service_motion specified");
  } else {
    j_reasons = is_service_motion_valid(config, service_motion, 0);
    if (j_reasons == NULL || json_array_size(j_reasons) == 0) {
      json_decref(j_reasons);
      j_query = json_pack("{sss{ssss}s{ss}}",
                          "table", 
                            "c_service_motion", 
                          "set",
                            "csm_description",
                            json_object_get(service_motion, "description")!=NULL?json_string_value(json_object_get(service_motion, "description")):"",
                            "csm_config_uri",
                            json_object_get(service_motion, "config_uri")!=NULL?json_string_value(json_object_get(service_motion, "config_uri")):"",
                          "where",
                            "csm_name",
                            name);
      res = h_update(config->conn, j_query, NULL);
      json_decref(j_query);
      if (res == H_OK) {
        j_query = json_pack("{sss{ss}}",
                          "table", 
                            "c_service_motion_file_list", 
                          "where",
                            "csm_name",
                            name);
        res = h_delete(config->conn, j_query, NULL);
        json_decref(j_query);
        if (res == H_OK) {
          json_array_foreach(json_object_get(service_motion, "file_list"), index, element) {
            j_query = json_pack("{sss{ssssssss}}",
                                "table", 
                                  "c_service_motion_file_list", 
                                "values",
                                  "csmfl_name",
                                  json_string_value(json_object_get(element, "name")),
                                  "csm_name",
                                  name,
                                  "csmfl_path",
                                  json_string_value(json_object_get(element, "path")),
                                  "csmfl_thumbnail_path",
                                  json_string_value(json_object_get(element, "thumbnail_path")));
            res = h_insert(config->conn, j_query, NULL);
            json_decref(j_query);
            if (res != H_OK) {
              y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_set - Error adding in table c_service_motion_file_list");
            }
          }
        } else {
          y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_set - Error removing old c_service_motion_file_list elements");
        }
        
        j_query = json_pack("{sss{ss}}",
                          "table", 
                            "c_service_motion_stream", 
                          "where",
                            "csm_name",
                            name);
        res = h_delete(config->conn, j_query, NULL);
        json_decref(j_query);
        if (res == H_OK) {
          json_array_foreach(json_object_get(service_motion, "stream_list"), index, element) {
            j_query = json_pack("{sss{ssssssss}}",
                                "table", 
                                  "c_service_motion_stream", 
                                "values",
                                  "csms_name",
                                  json_string_value(json_object_get(element, "name")),
                                  "csm_name",
                                  name,
                                  "csms_uri",
                                  json_string_value(json_object_get(element, "uri")),
                                  "csms_snapshot_uri",
                                  json_object_get(element, "snapshot_uri")!=NULL?json_string_value(json_object_get(element, "snapshot_uri")):"");
            res = h_insert(config->conn, j_query, NULL);
            json_decref(j_query);
            if (res != H_OK) {
              y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_set - Error adding in table c_service_motion_stream");
            }
          }
        } else {
          y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_set - Error removing old c_service_motion_stream elements");
        }
        
        return json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
      } else {
        y_log_message(Y_LOG_LEVEL_ERROR, "service_motion_set - Error setting in table c_service_motion");
        return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      }
    } else {
      return json_pack("{siso}", "result", WEBSERVICE_RESULT_PARAM, "reason", j_reasons);
    }
  }
}

/**
 * Return the file lists of the motion service specified
 */
json_t * service_motion_get_file_list(struct _carleon_config * config, const char * service_name) {
  json_t * j_query = json_pack("{sss[sss]s{ss}}", 
                                "table", 
                                  "c_service_motion_file_list", 
                                "columns", 
                                  "csmfl_name AS name", 
                                  "csmfl_path AS path", 
                                  "csmfl_thumbnail_path AS thumbnail_path", 
                                "where", 
                                  "csm_name",
                                  service_name),
          * j_result;
  int res;
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "file_list", j_result);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * Return the stream lists of the motion service specified
 */
json_t * service_motion_get_stream_list(struct _carleon_config * config, const char * service_name) {
  json_t * j_query = json_pack("{sss[sss]s{ss}}", 
                                "table", 
                                  "c_service_motion_stream", 
                                "columns", 
                                  "csms_name AS name", 
                                  "csms_uri AS uri", 
                                  "csms_snapshot_uri AS snapshot_uri", 
                                "where", 
                                  "csm_name",
                                  service_name),
          * j_result;
  int res;
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "stream_list", j_result);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * return true if the specified motion service exists
 */
int has_service_motion(struct _carleon_config * config, const char * service_name) {
  json_t * j_query = json_pack("{sss{ss}}",
                                "table",
                                  "c_service_motion", 
                                "where",
                                  "csm_name",
                                  service_name),
          * j_result;
  int res;
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    int ret = json_array_size(j_result) != 0;
    json_decref(j_result);
    return ret;
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "has_service_motion - Error executing j_query");
    return 0;
  }
}

/**
 * Remove the specified motion service
 */
json_t * service_motion_remove(struct _carleon_config * config, const char * service_name) {
  json_t * j_query_stream = json_pack("{sss{ss}}",
                                "table",
                                  "c_service_motion_stream", 
                                "where",
                                  "csm_name",
                                  service_name);
  json_t * j_query_file_list = json_pack("{sss{ss}}",
                                "table",
                                  "c_service_motion_file_list", 
                                "where",
                                  "csm_name",
                                  service_name);
  json_t * j_query = json_pack("{sss{ss}}",
                                "table",
                                  "c_service_motion", 
                                "where",
                                  "csm_name",
                                  service_name);
  int res, res_file_list, res_stream;
  
  res_stream = h_delete(config->conn, j_query_stream, NULL);
  res_file_list = h_delete(config->conn, j_query_file_list, NULL);
  res = h_delete(config->conn, j_query, NULL);
  json_decref(j_query);
  json_decref(j_query_file_list);
  json_decref(j_query_stream);
  if (res == H_OK && res_file_list == H_OK && res_stream == H_OK) {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "has_service_motion - Error executing j_queries");
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * get the specified motion service if name is not NULL, otherwise get all motion services
 */
json_t * service_motion_get(struct _carleon_config * config, const char * name) {
  json_t * j_query = json_pack("{ss}", "table", "c_service_motion");
  json_t * j_result, * j_service, * j_return, * j_cur_service, * j_file_list, * j_stream_list, * to_return;
  int res;
  size_t index;
  
  if (name != NULL) {
    json_object_set_new(j_query, "where", json_pack("{ss}", "csm_name", name));
  }
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    if (name != NULL) {
      if (json_array_size(j_result) == 0) {
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_NOT_FOUND);
      } else {
        j_return = json_object();
        if (j_return != NULL) {
          j_service = json_array_get(j_result, 0);
          j_file_list = service_motion_get_file_list(config, json_string_value(json_object_get(j_service, "csm_name")));
          j_stream_list = service_motion_get_stream_list(config, json_string_value(json_object_get(j_service, "csm_name")));
          if (j_file_list != NULL && json_integer_value(json_object_get(j_file_list, "result")) == WEBSERVICE_RESULT_OK &&
              j_stream_list != NULL && json_integer_value(json_object_get(j_stream_list, "result")) == WEBSERVICE_RESULT_OK) {
            json_object_set_new(j_return, "name", json_copy(json_object_get(j_service, "csm_name")));
            json_object_set_new(j_return, "description", json_copy(json_object_get(j_service, "csm_description")));
            json_object_set_new(j_return, "config_uri", json_copy(json_object_get(j_service, "csm_config_uri")));
            json_object_set_new(j_return, "file_list", json_copy(json_object_get(j_file_list, "file_list")));
            json_object_set_new(j_return, "stream_list", json_copy(json_object_get(j_stream_list, "stream_list")));
          } else {
            y_log_message(Y_LOG_LEVEL_ERROR, "get_service_motion_list - Error getting file_list or stream_list for %s", json_string_value(json_object_get(j_service, "csm_name")));
          }
          json_decref(j_file_list);
          json_decref(j_stream_list);
          to_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "element", j_return);
        } else {
          y_log_message(Y_LOG_LEVEL_ERROR, "get_service_motion_list - Error allocating resources for j_return");
          to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
        }
      }
    } else {
      j_return = json_array();
      if (j_return != NULL) {
        json_array_foreach(j_result, index, j_service) {
          j_file_list = service_motion_get_file_list(config, json_string_value(json_object_get(j_service, "csm_name")));
          j_stream_list = service_motion_get_stream_list(config, json_string_value(json_object_get(j_service, "csm_name")));
          if (j_file_list != NULL && json_integer_value(json_object_get(j_file_list, "result")) == WEBSERVICE_RESULT_OK &&
              j_stream_list != NULL && json_integer_value(json_object_get(j_stream_list, "result")) == WEBSERVICE_RESULT_OK) {
            j_cur_service = json_object();
            if (j_cur_service != NULL) {
              json_object_set_new(j_cur_service, "name", json_copy(json_object_get(j_service, "csm_name")));
              json_object_set_new(j_cur_service, "description", json_copy(json_object_get(j_service, "csm_description")));
              json_object_set_new(j_cur_service, "config_uri", json_copy(json_object_get(j_service, "csm_config_uri")));
              json_object_set_new(j_cur_service, "file_list", json_copy(json_object_get(j_file_list, "file_list")));
              json_object_set_new(j_cur_service, "stream_list", json_copy(json_object_get(j_stream_list, "stream_list")));
              json_array_append_new(j_return, j_cur_service);
            } else {
              y_log_message(Y_LOG_LEVEL_ERROR, "get_service_motion_list - Error allocating resources for j_cur_service");
            }
          } else {
            y_log_message(Y_LOG_LEVEL_ERROR, "get_service_motion_list - Error getting file_list or stream_list for %s", json_string_value(json_object_get(j_service, "csm_name")));
          }
          json_decref(j_file_list);
          json_decref(j_stream_list);
        }
        to_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "element", j_return);
      } else {
        y_log_message(Y_LOG_LEVEL_ERROR, "get_service_motion_list - Error allocating resources for j_return");
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      }
    }
    json_decref(j_result);
    return to_return;
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "get_service_motion_list - Error executing j_query");
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * Callback function to get all motion services or one motion service
 */
int callback_service_motion_get (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * j_service_motion;
  
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_get - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    j_service_motion = service_motion_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (j_service_motion != NULL && json_integer_value(json_object_get(j_service_motion, "result")) == WEBSERVICE_RESULT_OK) {
      response->json_body = json_copy(json_object_get(j_service_motion, "element"));
    } else if (j_service_motion != NULL && json_integer_value(json_object_get(j_service_motion, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
      response->status = 404;
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_get - Error get_service_motion_list");
      response->status = 500;
    }
    json_decref(j_service_motion);
  }
  return U_OK;
}

/**
 * Callback function to add a motion service
 */
int callback_service_motion_add (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result;
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_add - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    result = service_motion_add((struct _carleon_config *)user_data, request->json_body);
    if (result == NULL) {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_add - Error in service_motion_add");
      response->status = 500;
    } else if (json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_PARAM) {
      response->json_body = json_copy(json_object_get(result, "reason"));
      response->status = 400;
    }
    json_decref(result);
  }
  return U_OK;
}

/**
 * Callback function to modify a motion service
 */
int callback_service_motion_set (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result;
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_add - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else if (!has_service_motion((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"))) {
    response->status = 404;
  } else {
    result = service_motion_set((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"), request->json_body);
    if (result == NULL || json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_ERROR) {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_add - Error in service_motion_set");
      response->status = 500;
    } else if (json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_PARAM) {
      response->json_body = json_copy(json_object_get(result, "reason"));
      response->status = 400;
    }
    json_decref(result);
  }
  return U_OK;
}

/**
 * Callback function to remove a motion service
 */
int callback_service_motion_remove (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result;
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_remove - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else if (!has_service_motion((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"))) {
    response->status = 404;
  } else {
    result = service_motion_remove((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (result == NULL || json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_ERROR) {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_remove - Error in service_motion_set");
      response->status = 500;
    } else if (json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_PARAM) {
      response->json_body = json_copy(json_object_get(result, "reason"));
      response->status = 400;
    }
    json_decref(result);
  }
  return U_OK;
}

/**
 * return the filename extension
 */
const char * get_filename_ext(const char *path) {
    const char *dot = strrchr(path, '.');
    if(!dot || dot == path) return NULL;
    return dot;
}

int insert_in_array_sort(json_t * array, const char * value) {
	size_t index;
	json_t * element;
	if (array == NULL || value == NULL || strlen(value) == 0) {
		return 0;
	} else {
		json_array_foreach(array, index, element) {
			if (0 > strcmp(value, json_string_value(element))) {
				json_array_insert_new(array, index, json_string(value));
				return 1;
			}
		}
		json_array_append_new(array, json_string(value));
		return 1;
	} 
}

/**
 * Return a list of available files from the specified path
 */
json_t * get_available_files(const char * path, size_t count, size_t offset) {
	DIR *dir;
	struct dirent *ent;
	json_t * to_return, * list = json_array(), * orig_list = json_array();
	size_t index = 0;
	char * full_path;

	if ((dir = opendir (path)) != NULL && list != NULL && orig_list != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			if (0 == nstrcasecmp(get_filename_ext(ent->d_name), ".jpg")) {
				full_path = msprintf("%s/%s", path, ent->d_name);
				if (access(full_path, F_OK) != -1) {
					insert_in_array_sort(orig_list, ent->d_name);
				} else {
					y_log_message(Y_LOG_LEVEL_ERROR, "get_available_files - Can not access file %s", ent->d_name);
				}
				free(full_path);
			}
		}
		closedir (dir);
		for (index = json_array_size(orig_list)-offset-1; json_array_get(orig_list, index) != NULL; index--) {
			if (count <= 0) {
				break;
			} else {
				count--;
			}
			json_array_append_new(list, json_copy(json_array_get(orig_list, index)));
		}
		json_decref(orig_list);
		to_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "list", list);
	} else {
		to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
	}
	return to_return;
}

/**
 * Return 1 if the specified motion service is detected online, else 0
 */
int is_motion_online(struct _carleon_config * config, json_t * service_motion) {
  struct _u_request c_request;
  int to_return = 1;
  
  ulfius_init_request(&c_request);
  c_request.http_url = nstrdup(json_string_value(json_object_get(json_object_get(service_motion, "element"), "config_uri")));
	if (ulfius_send_http_request(&c_request, NULL) != U_OK) {
		to_return = 0;
	}
	ulfius_clean_request(&c_request);
	return to_return;
}

/**
* Status send 3 data sets:
* - motion online status (true|false)
* - files available in the lists, you can specify count and offset in the url
* - name of the stream availables
*/
int callback_service_motion_status (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * service_motion = service_motion_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
  json_t * to_return, * list_object, * list;
  int res;
  size_t count = 20, offset = 0;
  size_t index;
  
  if (service_motion != NULL && json_integer_value(json_object_get(service_motion, "result")) == WEBSERVICE_RESULT_OK) {
		to_return = json_object();
		if (to_return != NULL) {
			// Get online status
			res = is_motion_online((struct _carleon_config *)user_data, service_motion);
			if (res) {
				json_object_set_new(to_return, "online", json_true());
			} else {
				json_object_set_new(to_return, "online", json_false());
			}
			
			// Get avilable files
			if (u_map_get(request->map_url, "count") != NULL) {
				count = strtol(u_map_get(request->map_url, "count"), NULL, 10);
				if (count <= 0) {
					count = 20;
				}
			}
			if (u_map_get(request->map_url, "offset") != NULL) {
				offset = strtol(u_map_get(request->map_url, "offset"), NULL, 10);
				if (offset < 0) {
					offset = 0;
				}
			}
			json_object_set_new(to_return, "file_list", json_object());
			json_array_foreach(json_object_get(json_object_get(service_motion, "element"), "file_list"), index, list_object) {
				list = get_available_files(json_string_value(json_object_get(list_object, "path")), count, offset);
				if (list != NULL && json_integer_value(json_object_get(list, "result")) == WEBSERVICE_RESULT_OK) {
					json_object_set_new(json_object_get(to_return, "file_list"), json_string_value(json_object_get(list_object, "name")), json_copy(json_object_get(list, "list")));
				} else {
					json_object_set_new(json_object_get(to_return, "file_list"), json_string_value(json_object_get(list_object, "name")), json_array());
				}
				json_decref(list);
			}
			
			// get stream names
			json_object_set_new(to_return, "stream_list", json_copy(json_object_get(json_object_get(service_motion, "element"), "stream_list")));
			response->json_body = to_return;
		} else {
			y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_status - Error allocating resources for to_return");
			response->status = 500;
		}
	} else if (service_motion != NULL && json_integer_value(json_object_get(service_motion, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
		response->status = 404;
	} else {
		response->status = 500;
  }
  json_decref(service_motion);
  return U_OK;
}

/**
 * Return a file content
 */
void * get_file(const char * full_path, size_t * len) {
	void * buffer = NULL;
	FILE * f;
	
	if (full_path != NULL && len != NULL) {
		f = fopen (full_path, "rb");
		*len = 0;
		if (f) {
			fseek (f, 0, SEEK_END);
			*len = ftell (f);
			fseek (f, 0, SEEK_SET);
			buffer = malloc(*len*sizeof(void));
			if (buffer) {
				fread (buffer, 1, *len, f);
			}
			fclose (f);
		}
	}
	return buffer;
}

/**
 * Return an tumbnail of the specified image
 */
void * get_thumbnail(const char * full_path, const char * thumbnail_path, size_t * len) {
  void * blob = NULL;
  ExceptionInfo
    *exception;

  Image
    *image,
    *images,
    *resize_image,
    *thumbnails;

  ImageInfo
    *image_info;

  /*
    Initialize the image info structure and read an image.
  */
  exception = AcquireExceptionInfo();
  image_info = CloneImageInfo((ImageInfo *) NULL);
  (void) strcpy(image_info->filename, full_path);
  images = ReadImage(image_info, exception);
  if (exception->severity != UndefinedException) {
    CatchException(exception);
	}
  if (images == (Image *) NULL) {
    return NULL;
	}
  /*
    Convert the image to a thumbnail.
  */
  thumbnails = NewImageList();
  while ((image = RemoveFirstImageFromList(&images)) != (Image *) NULL) {
    resize_image = AdaptiveResizeImage(image, 160, 120, exception);
    if (resize_image == (Image *) NULL) {
      MagickError(exception->severity, exception->reason, exception->description);
		}
    (void) AppendImageToList(&thumbnails, resize_image);
    DestroyImage(image);
  }
  
  if (thumbnail_path != NULL) {
		strcpy(thumbnails->filename,thumbnail_path);
		WriteImage(image_info,thumbnails);
	}
  
  blob = ImagesToBlob(image_info, thumbnails, len, exception);
  /*
    Destroy the image thumbnail and return blob.
  */
  thumbnails = DestroyImageList(thumbnails);
  image_info = DestroyImageInfo(image_info);
  exception = DestroyExceptionInfo(exception);
  return blob;
}

/**
 * Callback function to get an image from a motion service
 */
int callback_service_motion_image (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * service_motion = service_motion_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
	json_t * file_list = NULL, * element;
	char * full_path, * thumbnail_path;
	size_t index;
	
  if (service_motion != NULL && json_integer_value(json_object_get(service_motion, "result")) == WEBSERVICE_RESULT_OK) {
		json_array_foreach(json_object_get(json_object_get(service_motion, "element"), "file_list"), index, element) {
			if (0 == strcmp(json_string_value(json_object_get(element, "name")), u_map_get(request->map_url, "file_list"))) {
				file_list = element;
			}
		}
		if (file_list != NULL) {
			full_path = msprintf("%s/%s", json_string_value(json_object_get(file_list, "path")), u_map_get(request->map_url, "file_name"));
			if (access(full_path, F_OK) != -1) {
				if (u_map_get(request->map_url, "thumbnail") != NULL) {
					thumbnail_path = msprintf("%s/%s", json_string_value(json_object_get(file_list, "thumbnail_path")), u_map_get(request->map_url, "file_name"));
					if (access(thumbnail_path, F_OK) != -1) {
						response->binary_body = get_file(thumbnail_path, &(response->binary_body_length));
						if (response->binary_body == NULL) {
							y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_status - Error getting content for thumbnail file %s", full_path);
							response->status = 500;
						}
					} else {
						response->binary_body = get_thumbnail(full_path, thumbnail_path, &(response->binary_body_length));
						if (response->binary_body == NULL) {
							y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_status - Error generating content for thumbnail file %s", full_path);
							response->status = 500;
						}
					}
					free(thumbnail_path);
				} else {
					response->binary_body = get_file(full_path, &(response->binary_body_length));
					if (response->binary_body == NULL) {
						y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_status - Error getting content for file %s", full_path);
						response->status = 500;
					}
				}
			} else {
				response->status = 404;
			}
			free(full_path);
		} else {
			response->status = 404;
		}
	} else if (service_motion != NULL && json_integer_value(json_object_get(service_motion, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
		response->status = 404;
	} else {
		response->status = 500;
  }
  json_decref(service_motion);
  return U_OK;
}

/**
 * Buffer structure used as a data cache
 * between the http request to the camera stream, 
 * and the http response to the client
 */
struct stream_buffer {
	pthread_mutex_t lock;
	char * stream_url;
	size_t size;
	void * data;
	int client_used;
	int server_used;
};

void close_buffer(struct stream_buffer ** buffer) {
	y_log_message(Y_LOG_LEVEL_DEBUG, "Closing buffer");
	if (*buffer != NULL && !(*buffer)->client_used && !(*buffer)->server_used) {
		free((*buffer)->stream_url);
		free((*buffer)->data);
		(*buffer)->stream_url = NULL;
		(*buffer)->data = NULL;
		pthread_mutex_destroy(&(*buffer)->lock);
		free((*buffer));
		*buffer = NULL;
		free(buffer);
	}
}

/**
 * get stream data from the camera stream, and add it at the end of the buffer
 */
size_t write_distant_body(void * contents, size_t size, size_t nmemb, void * user_data) {
	struct stream_buffer ** buffer = (struct stream_buffer **)user_data;
	size_t res = size * nmemb;

	if ((*buffer) != NULL && (*buffer)->server_used) {
		usleep(50);
		if (!pthread_mutex_lock(&(*buffer)->lock)) {
			void * new_data = malloc((size * nmemb) + (*buffer)->size + 1);
			if (new_data != NULL) {
				if ((*buffer)->data != NULL) {
					memcpy(new_data, (*buffer)->data, (*buffer)->size);
				}
				memcpy(new_data + (*buffer)->size, contents, (size * nmemb));
				free((*buffer)->data);
				(*buffer)->data = new_data;
				(*buffer)->size += (size * nmemb);
				//y_log_message(Y_LOG_LEVEL_DEBUG, "write buffer size %ld, now %ld", (size * nmemb), (*buffer)->size);
			} else {
				y_log_message(Y_LOG_LEVEL_ERROR, "write_distant_body - Error allocating resources for new_data");
				(*buffer)->client_used = 0;
				res = 0;
			}
		}
		pthread_mutex_unlock(&(*buffer)->lock);
	} else {
		(*buffer)->client_used = 0;
		res = 0;
	}
	return res;
}

/**
 * send stream data from the buffer to the client
 */
ssize_t stream_data (void * cls, uint64_t pos, char * buf, size_t max) {
	struct stream_buffer ** buffer = (struct stream_buffer **)cls;
	ssize_t res;
	
	if ((*buffer) != NULL && (*buffer)->client_used) {
		usleep(50);
		if (!pthread_mutex_lock(&(*buffer)->lock)) {
			size_t len = ((*buffer)->size > max)?max:(*buffer)->size;
			void * new_data = NULL;
			
			if (len > 0 && (*buffer)->size > 0) {
				memcpy(buf, (*buffer)->data, len);
				if ((*buffer)->size < len) {
					len = (*buffer)->size;
					free((*buffer)->data);
					(*buffer)->data = NULL;
					(*buffer)->size = 0;
					res = len;
				} else {
					new_data = malloc((*buffer)->size - len + 1);
					if (new_data != NULL) {
						memcpy(new_data, (*buffer)->data + len, (*buffer)->size - len);
						free((*buffer)->data);
						(*buffer)->data = new_data;
						(*buffer)->size -= len;
						res = len;
					} else {
						y_log_message(Y_LOG_LEVEL_ERROR, "stream_data - Error allocating memory for new_data");
						res = ULFIUS_STREAM_END;
					}
				}
				//y_log_message(Y_LOG_LEVEL_DEBUG, "stream_data now buffer size %ld", (*buffer)->size);
			} else {
				res = 0;
			}
		}
		pthread_mutex_unlock(&(*buffer)->lock);
	} else {
    res = ULFIUS_STREAM_END;
	}
	return res;
}

/**
 * Free the stream_buffer at the end of a connexion
 */
void free_stream_data(void * cls) {
	struct stream_buffer ** buffer = (struct stream_buffer **)cls;
	
	if ((*buffer) != NULL) {
		if (!pthread_mutex_lock(&(*buffer)->lock)) {
			//y_log_message(Y_LOG_LEVEL_DEBUG, "free_stream_data - closing buffer");
			(*buffer)->server_used = 0;
			close_buffer(buffer);
		}
	}
}

/**
 * Run the http request to the camera stream in a distinct thread
 */
void * thread_stream_run(void * args) {
	struct stream_buffer ** buffer = (struct stream_buffer **)args;
	int res;
	struct _u_request request;
	struct _u_response response;
	
	if ((*buffer) != NULL) {
		ulfius_init_request(&request);
		ulfius_init_response(&response);
		request.http_url = nstrdup((*buffer)->stream_url);
		res = ulfius_send_http_streaming_request(&request, &response, write_distant_body, buffer);
		if (res != U_OK) {
			//y_log_message(Y_LOG_LEVEL_DEBUG, "thread_stream_run - closing buffer");
		}
		(*buffer)->client_used = 0;
		close_buffer(buffer);
		ulfius_clean_request(&request);
		ulfius_clean_response(&response);
	} else {
		y_log_message(Y_LOG_LEVEL_ERROR, "thread_stream_run - Error input data");
	}
	
	return NULL;
}

/**
 * Callback function used to proxify the stream from the camera to the client
 */
int callback_service_motion_stream (const struct _u_request * request, struct _u_response * response, void * user_data) {
	json_t * service_motion = service_motion_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name")), * element, * stream = NULL;
	size_t index;
	struct stream_buffer ** buffer;
	pthread_t thread_stream;
	int thread_stream_ret = 0, thread_stream_detach = 0;
	pthread_mutexattr_t mutexattr;
	
  if (service_motion != NULL && json_integer_value(json_object_get(service_motion, "result")) == WEBSERVICE_RESULT_OK) {
		// Get online status
		json_array_foreach(json_object_get(json_object_get(service_motion, "element"), "stream_list"), index, element) {
			if (0 == strcmp(json_string_value(json_object_get(element, "name")), u_map_get(request->map_url, "stream_name"))) {
				stream = element;
			}
		}
		if (stream != NULL) {
			buffer = malloc(sizeof(struct stream_buffer *));
			if (buffer!=NULL) {
				(*buffer) = malloc(sizeof(struct stream_buffer));
			}
			if (buffer != NULL && *buffer != NULL) {
				(*buffer)->data = NULL;
				(*buffer)->size = 0;
				(*buffer)->client_used = 1;
				(*buffer)->server_used = 1;
				(*buffer)->stream_url = nstrdup(json_string_value(json_object_get(stream, "uri")));
				u_map_put(response->map_header, "Content-Type", "multipart/x-mixed-replace; boundary=--BoundaryString");
				u_map_put(response->map_header, "Pragma", "no-cache");
				u_map_put(response->map_header, "Cache-Control", "no-cache, private");
				u_map_put(response->map_header, "Expires", "0");
				u_map_put(response->map_header, "Max-Age", "0");
				pthread_mutexattr_init ( &mutexattr );
				pthread_mutexattr_settype( &mutexattr, PTHREAD_MUTEX_RECURSIVE_NP );
				if (pthread_mutex_init(&(*buffer)->lock, &mutexattr) != 0) {
					y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_stream - pthread_mutex_init error");
				} else {
					//y_log_message(Y_LOG_LEVEL_DEBUG, "accessing %s", (*buffer)->stream_url);
					
					thread_stream_ret = pthread_create(&thread_stream, NULL, thread_stream_run, (void *)buffer);
					thread_stream_detach = pthread_detach(thread_stream);
					if (thread_stream_ret || thread_stream_detach) {
						y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_stream - Error creating or detaching event thread, return code: %d, detach code: %d",
												thread_stream_ret, thread_stream_detach);
						response->status = 500;
					}
					
					ulfius_set_stream_response(response, 200, stream_data, free_stream_data, -1, 32 * 1024, buffer);
					json_decref(service_motion);
					return U_OK;
				}
			} else {
				y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_stream - error allocating resources for buffer");
				response->status = 500;
			}
		} else {
			response->status = 404;
		}
	} else {
		response->status = 500;
	}
	json_decref(service_motion);
  return U_OK;
}

/**
 * Send a snapshot command to the specified camera stream
 */
int send_snapshot_command(struct _carleon_config * config, const char * element_name, const char * stream_name) {
	json_t * service_motion = service_motion_get(config, element_name), * stream = NULL, * element;
  struct _u_request c_request;
  int res, to_return;
  size_t index;
  
  if (service_motion != NULL && json_integer_value(json_object_get(service_motion, "result")) == WEBSERVICE_RESULT_OK) {
		json_array_foreach(json_object_get(json_object_get(service_motion, "element"), "stream_list"), index, element) {
			if (0 == strcmp(json_string_value(json_object_get(element, "name")), stream_name)) {
				stream = element;
			}
		}
		if (stream != NULL && json_string_value(json_object_get(stream, "snapshot_uri")) != NULL) {
			ulfius_init_request(&c_request);
			c_request.http_url = nstrdup(json_string_value(json_object_get(stream, "snapshot_uri")));
			res = ulfius_send_http_request(&c_request, NULL);
			if (res != U_OK) {
				to_return = WEBSERVICE_RESULT_ERROR;
			} else {
				to_return = WEBSERVICE_RESULT_OK;
			}
			ulfius_clean_request(&c_request);
		} else {
			to_return = WEBSERVICE_RESULT_NOT_FOUND;
		}
	} else {
		to_return = WEBSERVICE_RESULT_NOT_FOUND;
	}
	json_decref(service_motion);
	return to_return;
}

/**
 * callback function used to send a snapshot command to the specified stream
 */
int callback_service_motion_snapshot (const struct _u_request * request, struct _u_response * response, void * user_data) {
	int res;
	
	if (user_data == NULL) {
		y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_motion_snapshot - Error user_data is NULL");
		response->status = 500;
	} else {
		res = send_snapshot_command((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"), u_map_get(request->map_url, "stream_name"));
		
		if (res == WEBSERVICE_RESULT_NOT_FOUND) {
			response->status = 404;
		} else if (res == WEBSERVICE_RESULT_ERROR) {
			response->status = 500;
		}
	}
	return U_OK;
}

/**
 * Service specific authentication callback function
 * The token must be the last 12 characters of the authentication token used in the header
 */
#define ANGHARAD_TABLE_SESSION "a_session"
int callback_auth_images (const struct _u_request * request, struct _u_response * response, void * user_data) {
	char * session_id;
	json_t * j_query, * j_result;
	int res;
	
	if (u_map_get(request->map_url, "ANGHARAD_SESSION_ID") != NULL && strlen (u_map_get(request->map_url, "ANGHARAD_SESSION_ID")) == 17) {
		session_id = msprintf("%%%s", u_map_get(request->map_url, "ANGHARAD_SESSION_ID"));
		j_query = json_pack("{sss[s]s{s{ssss}sis{ssss}}}",
													"table",
													ANGHARAD_TABLE_SESSION,
													"columns",
														"UNIX_TIMESTAMP(ass_validity) AS validity",
													"where",
														"ass_session_token",
															"operator",
															"LIKE",
															"value",
															session_id,
														"ass_enabled",
														1,
														"ass_validity",
															"operator",
															"raw",
															"value",
															">= NOW()");
		
		free(session_id);
		if (j_query != NULL) {
			res = h_select(((struct _carleon_config *)user_data)->conn, j_query, &j_result, NULL);
			json_decref(j_query);
			if (res == H_OK) {
				if (json_is_array(j_result) && json_array_size(j_result) > 0) {
					json_decref(j_result);
					return U_OK;
				} else {
					y_log_message(Y_LOG_LEVEL_DEBUG, "Unauthorized");
					json_decref(j_result);
					return U_ERROR_UNAUTHORIZED;
				}
			} else {
				y_log_message(Y_LOG_LEVEL_ERROR, "callback_auth_images - Error executing h_select");
				return U_ERROR;
			}
		} else {
			y_log_message(Y_LOG_LEVEL_ERROR, "callback_auth_images - Error allocating resources for j_query");
			return U_ERROR;
		}
	} else {
		return U_ERROR_UNAUTHORIZED;
	}
}

/**
 * Initialize the motion service
 */
json_t * c_service_init(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config) {
  if (instance != NULL && url_prefix != NULL && config != NULL) {
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/", NULL, NULL, NULL, &callback_service_motion_get, (void*)config);
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name", NULL, NULL, NULL, &callback_service_motion_get, (void*)config);
    ulfius_add_endpoint_by_val(instance, "POST", url_prefix, "/service-motion/", NULL, NULL, NULL, &callback_service_motion_add, (void*)config);
    ulfius_add_endpoint_by_val(instance, "PUT", url_prefix, "/service-motion/@name", NULL, NULL, NULL, &callback_service_motion_set, (void*)config);
    ulfius_add_endpoint_by_val(instance, "DELETE", url_prefix, "/service-motion/@name", NULL, NULL, NULL, &callback_service_motion_remove, (void*)config);

    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name/status", NULL, NULL, NULL, &callback_service_motion_status, (void*)config);
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name/image/@file_list/@file_name", callback_auth_images, (void*)config, NULL, &callback_service_motion_image, (void*)config);
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name/stream/@stream_name", callback_auth_images, (void*)config, NULL, &callback_service_motion_stream, (void*)config);
    ulfius_add_endpoint_by_val(instance, "PUT", url_prefix, "/service-motion/@name/stream/@stream_name/snapshot", NULL, NULL, NULL, &callback_service_motion_snapshot, (void*)config);

    return json_pack("{sissss}", 
                      "result", WEBSERVICE_RESULT_OK,
                      "name", "service-motion",
                      "description", "Motion service for camera management");
		MagickCoreGenesis("service-motion", MagickTrue);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * Closes the motion service
 */
json_t * c_service_close(struct _u_instance * instance, const char * url_prefix) {
  if (instance != NULL && url_prefix != NULL) {
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/");
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name");
    ulfius_remove_endpoint_by_val(instance, "POST", url_prefix, "/service-motion/");
    ulfius_remove_endpoint_by_val(instance, "PUT", url_prefix, "/service-motion/@name");
    ulfius_remove_endpoint_by_val(instance, "DELETE", url_prefix, "/service-motion/@name");

    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name/status");
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name/image/@image_list/@image_file");
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/service-motion/@name/stream/@stream_name");
    ulfius_remove_endpoint_by_val(instance, "PUT", url_prefix, "/service-motion/@name/stream/@stream_name/snapshot");

		MagickCoreTerminus();
		
    return json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * send the available commands
 */
json_t * c_service_command_get_list(struct _carleon_config * config) {
  return json_pack("{sis{s{s{ss}}s{s{s{ssso}}s{ss}}}}",
                    "result", WEBSERVICE_RESULT_OK,
                    "commands",
                      "online",
                        "result",
                          "type", "boolean",
                      "snapshot",
                        "parameters",
                          "stream",
                            "type", "string", "required", json_true(),
                        "result",
                            "type", "boolean");
}

/**
 * Get the list of available elements
 */
json_t * c_service_element_get_list(struct _carleon_config * config) {
  return service_motion_get(config, NULL);
}

/**
 * Execute a command
 */
json_t * c_service_exec(struct _carleon_config * config, const char * command, const char * element, json_t * parameters) {
	json_t * service_motion, * result = NULL;

	if (command != NULL) {
		if (0 == strcmp(command, "online")) {
			service_motion = service_motion_get(config, element);
			if (service_motion != NULL && json_integer_value(json_object_get(service_motion, "result")) == WEBSERVICE_RESULT_OK) {
				result = json_pack("{sisi}", "result", WEBSERVICE_RESULT_OK, "value", is_motion_online(config, service_motion));
			} else {
				result = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
			}
			json_decref(service_motion);
		} else if (0 == strcmp(command, "snapshot")) {
			if (parameters != NULL && json_object_get(parameters, "stream") != NULL) {
				result = json_pack("{si}", "result", send_snapshot_command(config, element, json_string_value(json_object_get(parameters, "stream"))));
			} else {
				result = json_pack("{si}", "result", WEBSERVICE_RESULT_PARAM);
			}
		} else {
			result = json_pack("{si}", "result", WEBSERVICE_RESULT_NOT_FOUND);
		}
	} else {
		result = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
	}
	return result;
}
