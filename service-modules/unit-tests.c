/**
 *
 * Angharad system
 *
 * Manage all subsystems and handles the house automation server:
 * - Benoic
 * - Gareth
 * - Carleon
 *
 * Unit tests
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

/**
 * In fact, I should make a library or something like that to facilitate the tests
 */

#include <string.h>
#include <jansson.h>
#include <ulfius.h>
#include <orcania.h>

#define SERVER_URL "http://localhost:2473/"
#define PREFIX_ANGHARAD "angharad"
#define PREFIX_BENOIC "benoic"
#define PREFIX_CARLEON "carleon"

/**
 * decode a u_map into a string
 */
char * print_map(const struct _u_map * map) {
  char * line, * to_return = NULL;
  const char **keys;
  int len, i;
  if (map != NULL) {
    keys = u_map_enum_keys(map);
    for (i=0; keys[i] != NULL; i++) {
      len = snprintf(NULL, 0, "key is %s, value is %s\n", keys[i], u_map_get(map, keys[i]));
      line = malloc((len+1)*sizeof(char));
      snprintf(line, (len+1), "key is %s, value is %s\n", keys[i], u_map_get(map, keys[i]));
      if (to_return != NULL) {
        len = strlen(to_return) + strlen(line) + 1;
        to_return = realloc(to_return, (len+1)*sizeof(char));
      } else {
        to_return = malloc((strlen(line) + 1)*sizeof(char));
        to_return[0] = 0;
      }
      strcat(to_return, line);
      free(line);
    }
    return to_return;
  } else {
    return NULL;
  }
}

/**
 * Developper-friendly response print
 */
void print_response(struct _u_response * response) {
  char * dump_json = NULL;
  if (response != NULL) {
    printf("Status: %ld\n\n", response->status);
    if (response->json_body != NULL) {
      dump_json = json_dumps(response->json_body, JSON_INDENT(2));
      printf("Json body:\n%s\n\n", dump_json);
      free(dump_json);
    } else if (response->string_body != NULL) {
      printf("String body: %s\n\n", response->string_body);
    }
  }
}

int test_request_status(struct _u_request * req, long int expected_status, json_t * expected_contains) {
  int res, to_return = 0;
  struct _u_response response;
  
  ulfius_init_response(&response);
  res = ulfius_send_http_request(req, &response);
  if (res == U_OK) {
    if (response.status != expected_status) {
      printf("##########################\nError status (%s %s %ld)\n", req->http_verb, req->http_url, expected_status);
      print_response(&response);
      printf("##########################\n\n");
    } else if (expected_contains != NULL && (response.json_body == NULL || json_search(response.json_body, expected_contains) == NULL)) {
      char * dump_expected = json_dumps(expected_contains, JSON_ENCODE_ANY), * dump_response = json_dumps(response.json_body, JSON_ENCODE_ANY);
      printf("##########################\nError json (%s %s)\n", req->http_verb, req->http_url);
      printf("Expected result in response:\n%s\nWhile response is:\n%s\n", dump_expected, dump_response);
      printf("##########################\n\n");
      free(dump_expected);
      free(dump_response);
    } else {
      printf("Success (%s %s %ld)\n\n", req->http_verb, req->http_url, expected_status);
      to_return = 1;
    }
  } else {
    printf("Error in http request: %d\n", res);
  }
  ulfius_clean_response(&response);
  return to_return;
}

void run_simple_test(const char * method, const char * url, json_t * request_body, int expected_status, json_t * expected_body) {
  struct _u_request request;
  ulfius_init_request(&request);
  request.http_verb = strdup(method);
  request.http_url = strdup(url);
  request.json_body = json_copy(request_body);
  
  test_request_status(&request, expected_status, expected_body);
  
  ulfius_clean_request(&request);
}

void run_service_mock_tests() {
  json_t * service_mock = json_loads("{\
		\"name\": \"mock1\",\
		\"description\": \"mock1\"\
  }", JSON_DECODE_ANY, NULL);
  json_t * service_mock2 = json_loads("{\
		\"name\": \"mock1\",\
		\"description\": \"mock1 updated\"\
  }", JSON_DECODE_ANY, NULL);
  
  run_simple_test("POST", SERVER_URL PREFIX_CARLEON "/mock-service/", service_mock, 200, NULL);
  run_simple_test("GET", SERVER_URL PREFIX_CARLEON "/mock-service/mock1", NULL, 200, service_mock);
  run_simple_test("GET", SERVER_URL PREFIX_CARLEON "/mock-service/mock2", NULL, 404, NULL);
  run_simple_test("PUT", SERVER_URL PREFIX_CARLEON "/mock-service/mock1", service_mock2, 200, NULL);
  run_simple_test("GET", SERVER_URL PREFIX_CARLEON "/mock-service/", NULL, 200, NULL);
  run_simple_test("DELETE", SERVER_URL PREFIX_CARLEON "/mock-service/mock1", NULL, 200, NULL);

  json_decref(service_mock);
  json_decref(service_mock2);
}

void run_service_motion_tests() {
  json_t * service_motion = json_loads("{\
		\"name\": \"motion1\",\
		\"description\": \"motion1\",\
		\"config_uri\": \"motion1_config\",\
		\"file_list\":[\
			{\
			\"name\":\"path1\",\
			\"path\":\"path1\",\
			\"thumbnail_path\":\"path1\"\
			}\
		],\
		\"stream_list\":[\
			{\
			\"name\":\"uri1\",\
			\"uri\":\"uri1\",\
			\"snapshot_uri\":\"suri1\"\
			}\
		]\
  }", JSON_DECODE_ANY, NULL);
  json_t * service_motion_update = json_loads("{\
		\"description\": \"motion12\",\
		\"config_uri\": \"motion12_config\",\
		\"file_list\":[\
			{\
			\"name\":\"path12\",\
			\"path\":\"path12\",\
			\"thumbnail_path\":\"path12\"\
			}\
		],\
		\"stream_list\":[\
			{\
			\"name\":\"uri12\",\
			\"uri\":\"uri12\",\
			\"snapshot_uri\":\"suri12\"\
			}\
		]\
  }", JSON_DECODE_ANY, NULL);
  json_t * service_motion_invalid = json_loads("{\
		\"name\": \"motion2\",\
		\"description\": 3,\
		\"file_list\":[\
			{\
			\"path\":\"path1\"\
			}\
		],\
		\"stream_list\":[\
			{\
			\"name\":\"uri1\",\
			\"uri\":3,\
			\"snapshot_uri\":\"potayto\"\
			}\
		]\
  }", JSON_DECODE_ANY, NULL);
  
  run_simple_test("POST", SERVER_URL PREFIX_CARLEON "/service-motion/", service_motion, 200, NULL);
  run_simple_test("POST", SERVER_URL PREFIX_CARLEON "/service-motion/", service_motion_invalid, 400, NULL);
  run_simple_test("GET", SERVER_URL PREFIX_CARLEON "/service-motion/motion1", NULL, 200, service_motion);
  run_simple_test("GET", SERVER_URL PREFIX_CARLEON "/service-motion/motion2", NULL, 404, NULL);
  run_simple_test("PUT", SERVER_URL PREFIX_CARLEON "/service-motion/motion1", service_motion_update, 200, NULL);
  run_simple_test("GET", SERVER_URL PREFIX_CARLEON "/service-motion/", NULL, 200, NULL);
  run_simple_test("DELETE", SERVER_URL PREFIX_CARLEON "/service-motion/motion1", NULL, 200, NULL);

  json_decref(service_motion);
  json_decref(service_motion_update);
  json_decref(service_motion_invalid);
}

int main(void) {
  printf("Press <enter> to run service mock tests\n");
  getchar();
  run_service_mock_tests();
  printf("Press <enter> to run service motion tests\n");
  getchar();
  run_service_motion_tests();
  return 0;
}
