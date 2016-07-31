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

#include "../carleon.h"

#define PORT 2473
#define PREFIX "carleon"

json_t * c_service_init(struct _u_instance * instance, const char * url_prefix, struct _carleon_config * config);
json_t * c_service_close(struct _u_instance * instance, const char * url_prefix);
json_t * c_service_command_get_list(struct _carleon_config * config);
json_t * c_service_element_get_list(struct _carleon_config * config);
json_t * c_service_exec(struct _carleon_config * config, const char * command, const char * element, json_t * parameters);

int main(int argc, char ** argv) {
  // Initialize the instance
  struct _u_instance instance;
  struct _carleon_config config;
  
  config.conn = 
  
  if (ulfius_init_instance(&instance, PORT, NULL) != U_OK) {
    y_log_message(Y_LOG_LEVEL_ERROR, "Error ulfius_init_instance, abort");
    return(1);
  }
  
  
}
