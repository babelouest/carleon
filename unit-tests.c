/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
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

#include <string.h>
#include <jansson.h>
#include <ulfius.h>
#include <orcania.h>

//#define SERVER_URL_PREFIX "http://localhost:2756/carleon"
#define SERVER_URL_PREFIX "http://localhost:2473/carleon"

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

void run_service_tests() {
  json_t * mock_valid1 = json_loads("{\
    \"name\":\"mock1\",\
    \"description\":\"Description for mock1\"\
  }", JSON_DECODE_ANY, NULL);
  json_t * mock_valid2 = json_loads("{\
    \"name\":\"mock2\",\
    \"description\":\"Description for mock2\"\
  }", JSON_DECODE_ANY, NULL);
  json_t * mock_valid3 = json_loads("{\
    \"description\":\"New description for mock2\"\
  }", JSON_DECODE_ANY, NULL);
  json_t * mock1_check = json_loads("{\
    \"name\": \"mock1\", \
    \"description\": \"Description for mock 1\",\
    \"tag\":\
    [\
      \"tag1\",\
      \"tag2\"\
    ]\
  }", JSON_DECODE_ANY, NULL);
  json_t * mock2_check1 = json_loads("{\
    \"name\": \"mock2\", \
    \"description\": \"Description for mock 1\",\
    \"tag\":\
    [\
      \"tag1\",\
      \"tag1\"\
    ]\
  }", JSON_DECODE_ANY, NULL);
  json_t * mock2_check2 = json_loads("{\
    \"name\": \"mock2\", \
    \"description\": \"Description for mock 1\",\
    \"tag\":\
    [\
    ]\
  }", JSON_DECODE_ANY, NULL);
  
  run_simple_test("GET", SERVER_URL_PREFIX "/service/", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/reload", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/enable/0", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/enable/1", NULL, 200, NULL);
  run_simple_test("GET", SERVER_URL_PREFIX "/mock-service/", NULL, 200, NULL);
  run_simple_test("POST", SERVER_URL_PREFIX "/mock-service/", mock_valid1, 200, NULL);
  run_simple_test("POST", SERVER_URL_PREFIX "/mock-service/", mock_valid2, 200, NULL);
  run_simple_test("GET", SERVER_URL_PREFIX "/mock-service/mock1", NULL, 200, mock_valid1);
  run_simple_test("GET", SERVER_URL_PREFIX "/mock-service/mock2", NULL, 200, mock_valid2);
  run_simple_test("GET", SERVER_URL_PREFIX "/mock-service/mock3", NULL, 404, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/mock-service/mock2", mock_valid3, 200, NULL);
  run_simple_test("GET", SERVER_URL_PREFIX "/mock-service/mock2", NULL, 200, mock_valid3);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/mock1/tag1", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/mock1/tag2", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/mock2/tag1", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/mock2/tag1", NULL, 200, NULL);
  run_simple_test("GET", SERVER_URL_PREFIX "/service/", NULL, 200, mock1_check);
  run_simple_test("GET", SERVER_URL_PREFIX "/service/", NULL, 200, mock2_check1);
  run_simple_test("DELETE", SERVER_URL_PREFIX "/service/mock-service/mock2/tag1", NULL, 200, NULL);
  run_simple_test("GET", SERVER_URL_PREFIX "/service/", NULL, 200, mock2_check2);
  run_simple_test("DELETE", SERVER_URL_PREFIX "/mock-service/mock1", NULL, 200, NULL);
  run_simple_test("DELETE", SERVER_URL_PREFIX "/mock-service/mock2", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/mock1/cleanup", NULL, 200, NULL);
  run_simple_test("PUT", SERVER_URL_PREFIX "/service/mock-service/mock2/cleanup", NULL, 200, NULL);
  
  json_decref(mock_valid1);
  json_decref(mock_valid2);
  json_decref(mock_valid3);
  json_decref(mock1_check);
  json_decref(mock2_check1);
  json_decref(mock2_check2);
}

int main(void) {
  printf("Press <enter> to run service tests\n");
  getchar();
  run_service_tests();
  return 0;
}
