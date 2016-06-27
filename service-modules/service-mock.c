/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
 *
 * Mock service module
 * Provides all the commands for a fake service
 * Used to develop and validate carleon infrastructure above
 *
 * Copyright 2016 Nicolas Mora <mail@babelouest.org>
 *
 * Licence MIT
 *
 */

#include <jansson.h>
#include <ulfius.h>
#include <hoel.h>
#include <orcania.h>
#include <yder.h>
#include "../carleon.h"

#define RESULT_ERROR     0
#define RESULT_OK        1
#define RESULT_NOT_FOUND 2
#define RESULT_TIMEOUT   3


json_t * c_service_exec(struct _carleon_config * config, const char * command, const char * element, json_t * parameters) {
  char * str_parameters = json_dumps(parameters, JSON_COMPACT);
  y_log_message(Y_LOG_LEVEL_INFO, "mock-service - Executing command '%s' to element '%s' with parameters %s", command, element, str_parameters);
  free(str_parameters);
  if (0 == nstrcmp(command, "exec")) {
    return json_pack("{sisiso}", "result", RESULT_OK, "value", 1, "other_value", json_true());
  } else {
    return json_pack("{sisi}", "result", RESULT_OK, "value", 10);
  }
}

json_t * mock_element_get(struct _carleon_config * config, const char * element_id) {
  json_t * j_query = json_object(), * j_result;
  int res;
  
  json_object_set_new(j_query, "table", json_string("c_mock_service"));
  json_object_set_new(j_query, "columns", json_pack("[ss]", "cms_name AS name", "cms_description AS description"));
  if (element_id != NULL) {
    json_object_set_new(j_query, "where", json_pack("{ss}", "cms_name", element_id));
  }
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    if (json_array_size(j_result) == 0 && element_id != NULL) {
      json_decref(j_result);
      return json_pack("{si}", "result", RESULT_NOT_FOUND);
    } else {
      if (element_id == NULL) {
        json_t * to_return = json_pack("{siso}", "result", RESULT_OK, "element", json_copy(j_result));
        json_decref(j_result);
        return to_return;
      } else {
        json_t * element = json_copy(json_array_get(j_result, 0));
        json_decref(j_result);
        return json_pack("{siso}", "result", RESULT_OK, "element", element);
      }
    }
  } else {
    return json_pack("{si}", "result", RESULT_ERROR);
  }
}

int mock_element_add(struct _carleon_config * config, json_t * element) {
  json_t * cur_element = mock_element_get(config, json_string_value(json_object_get(element, "name"))), 
          * j_query = json_pack("{sss[{ssss}]}", 
                                "table", "c_mock_service", 
                                "values", 
                                  "cms_name", json_string_value(json_object_get(element, "name")),
                                  "cms_description", json_string_value(json_object_get(element, "description")));
  int res;
  
  if (json_integer_value(json_object_get(cur_element, "result")) == RESULT_OK || json_integer_value(json_object_get(cur_element, "result")) == RESULT_ERROR) {
    json_decref(cur_element);
    json_decref(j_query);
    return RESULT_ERROR;
  }
  json_decref(cur_element);
  
  res = h_insert(config->conn, j_query, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return RESULT_OK;
  } else {
    return RESULT_ERROR;
  }
}

int mock_element_modify(struct _carleon_config * config, const char * element_id, json_t * element) {
  json_t * cur_element = mock_element_get(config, element_id),
          * j_query = json_pack("{sss{ss}s{ss}}", 
                                "table", "c_mock_service", 
                                "set", 
                                  "cms_description", json_string_value(json_object_get(element, "description")),
                                "where",
                                  "cms_name", element_id);
  int res;
  
  if (json_integer_value(json_object_get(cur_element, "result")) == RESULT_NOT_FOUND) {
    json_decref(cur_element);
    json_decref(j_query);
    return RESULT_NOT_FOUND;
  }
  json_decref(cur_element);

  res = h_update(config->conn, j_query, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return RESULT_OK;
  } else {
    return RESULT_ERROR;
  }
}

int mock_element_delete(struct _carleon_config * config, const char * element_id) {
  json_t * cur_element = mock_element_get(config, element_id),
          * j_query = json_pack("{sss{ss}}", 
                                "table", "c_mock_service", 
                                "where", 
                                  "cms_name", element_id);
  int res;
  
  if (json_integer_value(json_object_get(cur_element, "result")) == RESULT_NOT_FOUND) {
    json_decref(cur_element);
    json_decref(j_query);
    return RESULT_NOT_FOUND;
  }
  json_decref(cur_element);

  res = h_delete(config->conn, j_query, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    return RESULT_OK;
  } else {
    return RESULT_ERROR;
  }
}

int callback_mock_service_command (const struct _u_request * request, struct _u_response * response, void * user_data) {
  int i_param2 = strtol(u_map_get(request->map_url, "param2"), NULL, 10);
  float f_param3 = strtof(u_map_get(request->map_url, "param3"), NULL);
  json_t * parameters = json_pack("{sssisf}", "param1", u_map_get(request->map_url, "param1"), "param2", i_param2, "param3", f_param3), * j_result;
  
  j_result = c_service_exec((struct _carleon_config *)user_data, "exec", u_map_get(request->map_url, "element_id"), parameters);
  if (j_result == NULL || json_integer_value(json_object_get(j_result, "result")) != RESULT_OK) {
    response->status = 500;
  }
  json_decref(parameters);
  return U_OK;
}

int callback_mock_service (const struct _u_request * request, struct _u_response * response, void * user_data) {
  if (0 == nstrcmp(request->http_verb, "GET")) {
    json_t * element = mock_element_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "element_id"));
    if (json_integer_value(json_object_get(element, "result")) == RESULT_ERROR) {
      response->status = 500;
    } else if (json_integer_value(json_object_get(element, "result")) == RESULT_NOT_FOUND) {
      response->status = 404;
    } else {
      response->json_body = json_copy(json_object_get(element, "element"));
    }
    json_decref(element);
  } else if (0 == nstrcmp(request->http_verb, "POST")) {
    if (json_object_get(request->json_body, "name") != NULL && json_is_string(json_object_get(request->json_body, "name")) && json_string_length(json_object_get(request->json_body, "name")) <= 64 &&
        json_object_get(request->json_body, "description") != NULL && json_is_string(json_object_get(request->json_body, "description")) && json_string_length(json_object_get(request->json_body, "description")) <= 128) {
      int res = mock_element_add((struct _carleon_config *)user_data, request->json_body);
      if (res == RESULT_ERROR) {
        response->status = 500;
      }
    } else {
      response->status = 400;
    }
  } else if (0 == nstrcmp(request->http_verb, "PUT")) {
    json_t * element = mock_element_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "element_id"));
    if (element == NULL) {
      response->status = 404;
    } else if (json_object_get(request->json_body, "description") != NULL && json_is_string(json_object_get(request->json_body, "description")) && json_string_length(json_object_get(request->json_body, "description")) <= 128) {
      if (mock_element_modify((struct _carleon_config *)user_data, u_map_get(request->map_url, "element_id"), request->json_body) == RESULT_ERROR) {
        response->status = 500;
      }
    } else {
      response->status = 400;
    }
    json_decref(element);
  } else if (0 == nstrcmp(request->http_verb, "DELETE")) {
    json_t * element = mock_element_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "element_id"));
    if (element == NULL) {
      response->status = 404;
    } else if (mock_element_delete((struct _carleon_config *)user_data, u_map_get(request->map_url, "element_id")) == RESULT_ERROR) {
      response->status = 500;
    }
    json_decref(element);
  }
  return U_OK;
}

json_t * c_service_init(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config) {
  if (instance != NULL && url_prefix != NULL && config != NULL) {
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/mock-service/@element_id", NULL, NULL, NULL, &callback_mock_service, (void*)config);
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/mock-service/", NULL, NULL, NULL, &callback_mock_service, (void*)config);
    ulfius_add_endpoint_by_val(instance, "POST", url_prefix, "/mock-service/", NULL, NULL, NULL, &callback_mock_service, (void*)config);
    ulfius_add_endpoint_by_val(instance, "PUT", url_prefix, "/mock-service/@element_id", NULL, NULL, NULL, &callback_mock_service, (void*)config);
    ulfius_add_endpoint_by_val(instance, "DELETE", url_prefix, "/mock-service/@element_id", NULL, NULL, NULL, &callback_mock_service, (void*)config);
    ulfius_add_endpoint_by_val(instance, "GET", url_prefix, "/mock-service/@element_id/command/@command_name/@param1/@param2/@param3", NULL, NULL, NULL, &callback_mock_service_command, (void*)config);
    
    return json_pack("{sissss}", 
                      "result", RESULT_OK,
                      "name", "mock-service",
                      "description", "Mock service for Carleon development");
  } else {
    return json_pack("{si}", "result", RESULT_ERROR);
  }
}

json_t * c_service_close(struct _u_instance * instance, const char * url_prefix) {
  if (instance != NULL && url_prefix != NULL) {
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/mock-service/@element_id");
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/mock-service/");
    ulfius_remove_endpoint_by_val(instance, "POST", url_prefix, "/mock-service/");
    ulfius_remove_endpoint_by_val(instance, "PUT", url_prefix, "/mock-service/@element_id");
    ulfius_remove_endpoint_by_val(instance, "DELETE", url_prefix, "/mock-service/@element_id");
    ulfius_remove_endpoint_by_val(instance, "GET", url_prefix, "/mock-service/@element_id/command/@command_name/@param1/@param2/@param3");
    
    return json_pack("{si}", "result", RESULT_OK);
  } else {
    return json_pack("{si}", "result", RESULT_ERROR);
  }
}

json_t * c_service_enable(struct _carleon_config * config, int status) {
  return json_pack("{si}", "result", RESULT_OK);
}

/**
 * 2 commands
 */
json_t * c_service_command_get_list(struct _carleon_config * config) {
  return json_pack("{sis{s{s{s{ssso}s{ssso}s{ss}}}s{s{s{ssso}}}}}", 
                    "result", RESULT_OK, 
                    "commands", 
                      "exec1", 
                        "parameters", 
                          "param1", 
                            "type", "string", "required", json_true(), 
                          "param2", 
                            "type", "integer", "required", json_false(), 
                          "param3", 
                            "type", "real",
                      "exec2",
                        "parameters",
                          "param1",
                            "type", "string", "required", json_true());
}

json_t * c_service_element_get_list(struct _carleon_config * config) {
  return mock_element_get(config, NULL);
}
