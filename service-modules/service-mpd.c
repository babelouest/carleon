/**
 *
 * Carleon House Automation side services
 *
 * Command house automation side services via an HTTP REST interface
 *
 * MPD service module
 * Provides a controller to a MPD application
 * - get the current status
 * - play/pause/stop
 * - get available playlists
 * - load and play a specified playlist
 * - set the volume
 *
 * library mpdclient is required to install
 * sudo apt-get install libmpdclient-dev
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

#include <jansson.h>
#include <ulfius.h>
#include <hoel.h>
#include <orcania.h>
#include <yder.h>
#include "../carleon.h"

#include <mpd/client.h>
#include <mpd/status.h>
#include <mpd/player.h>
#include <mpd/playlist.h>
#include <mpd/mixer.h>

#define MPD_TABLE_NAME "c_service_mpd"
#define MPD_CONNECT_TIMEOUT 3000

/**
 * get the specified mpd or all mpd
 */
json_t * mpd_get(struct _carleon_config * config, const char * mpd_name) {
  json_t * j_query = json_pack("{sss[ssss]}", 
                    "table", 
                      MPD_TABLE_NAME,
                    "columns",
                      "cmpd_name AS name",
                      "cmpd_description AS description",
                      "cmpd_host AS host",
                      "cmpd_port AS port"
                    ), * j_result, * to_return;
  int res;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (j_query == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get - Error allocating resources for j_query");
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
  
  if (mpd_name != NULL) {
    json_object_set_new(j_query, "where", json_pack("{ss}", "cmpd_name", mpd_name));
  }
  
  res = h_select(config->conn, j_query, &j_result, NULL);
  json_decref(j_query);
  if (res == H_OK) {
    if (mpd_name == NULL) {
      to_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "element", json_copy(j_result));
    } else {
      if (json_array_size(j_result) == 0) {
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_NOT_FOUND);
      } else {
        to_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "element", json_copy(json_array_get(j_result, 0)));
      }
    }
    json_decref(j_result);
  } else {
    y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get - Error executing j_query");
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
  return to_return;
}

/**
 * check the specified mpd has valid parameters
 */
json_t * is_mpd_valid(struct _carleon_config * config, json_t * mpd, int add) {
  json_t * to_return = json_array(), * j_result, * element;
  size_t index;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (to_return == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "is_mpd_valid - Error allocating resources for to_return");
    return NULL;
  }
  
  if (mpd == NULL || !json_is_object(mpd)) {
    json_array_append_new(to_return, json_pack("{ss}", "mpd", "mpd must be a json object"));
  } else {
    if (!add) {
      j_result = mpd_get(config, NULL);
      json_array_foreach(json_object_get(j_result, "element"), index, element) {
        if (0 == nstrcmp(json_string_value(json_object_get(element, "name")), json_string_value(json_object_get(mpd, "name")))) {
          json_array_append_new(to_return, json_pack("{ss}", "name", "a mpd service with the same name already exist"));
        }
      }
      json_decref(j_result);
    }
    
    if (add && (json_object_get(mpd, "name") == NULL || !json_is_string(json_object_get(mpd, "name")) || json_string_length(json_object_get(mpd, "name")) > 64)) {
      json_array_append_new(to_return, json_pack("{ss}", "name", "name is mandatory and must be a string up to 64 characters"));
    }
    
    if (json_object_get(mpd, "description") != NULL && (!json_is_string(json_object_get(mpd, "description")) || json_string_length(json_object_get(mpd, "description")) > 128)) {
      json_array_append_new(to_return, json_pack("{ss}", "description", "description must be a string up to 128 characters"));
    }
    
    if (json_object_get(mpd, "host") == NULL || !json_is_string(json_object_get(mpd, "host")) || json_string_length(json_object_get(mpd, "host")) > 128) {
      json_array_append_new(to_return, json_pack("{ss}", "host", "host is mandatory and must be a string up to 128 characters"));
    }
    
    if (json_object_get(mpd, "port") != NULL && (!json_is_integer(json_object_get(mpd, "port")) || json_integer_value(json_object_get(mpd, "port")) <= 0 || json_integer_value(json_object_get(mpd, "port")) > 65535)) {
      json_array_append_new(to_return, json_pack("{ss}", "port", "port must be an integer between 1 and 65535"));
    }
    
    if (json_object_get(mpd, "password") != NULL && (!json_is_string(json_object_get(mpd, "password")) || json_string_length(json_object_get(mpd, "password")) > 128)) {
      json_array_append_new(to_return, json_pack("{ss}", "password", "password must be a string up to 128 characters"));
    }
    
  }
  
  return to_return;
}

/**
 * add the specified mpd
 */
json_t * mpd_add(struct _carleon_config * config, json_t * mpd) {
  json_t * j_query = json_pack("{sss{sssssssIss}}",
                      "table",
                        MPD_TABLE_NAME,
                      "values",
                        "cmpd_name", json_string_value(json_object_get(mpd, "name")),
                        "cmpd_description", json_object_get(mpd, "description")!=NULL?json_string_value(json_object_get(mpd, "description")):"",
                        "cmpd_host", json_string_value(json_object_get(mpd, "host")),
                        "cmpd_port", json_object_get(mpd, "port")!=NULL?json_integer_value(json_object_get(mpd, "port")):0,
                        "cmpd_password", json_object_get(mpd, "password")!=NULL?json_string_value(json_object_get(mpd, "password")):""),
          * j_reasons, * j_return;
  int res;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  j_reasons = is_mpd_valid(config, mpd, 1);
  if (j_reasons != NULL && json_array_size(j_reasons) == 0) {
    res = h_insert(config->conn, j_query, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      j_return = json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
    } else {
      j_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
  } else if (j_reasons != NULL && json_array_size(j_reasons) > 0) {
    j_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_PARAM, "reason", j_reasons);
  } else {
    j_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
  json_decref(j_reasons);
  return j_return;
}

/**
 * update the specified mpd
 */
json_t * mpd_set(struct _carleon_config * config, const char * name, json_t * mpd) {
  json_t * j_query = json_pack("{sss{sssssIss}s{ss}}",
                      "table",
                        MPD_TABLE_NAME,
                      "set",
                        "cmpd_description", json_object_get(mpd, "description")!=NULL?json_string_value(json_object_get(mpd, "description")):"",
                        "cmpd_host", json_string_value(json_object_get(mpd, "host")),
                        "cmpd_port", json_object_get(mpd, "port")!=NULL?json_integer_value(json_object_get(mpd, "port")):0,
                        "cmpd_password", json_object_get(mpd, "password")!=NULL?json_string_value(json_object_get(mpd, "password")):"",
                      "where",
                        "cmpd_name", name),
          * j_reasons, * j_return;
  int res;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  j_reasons = is_mpd_valid(config, mpd, 0);
  if (j_reasons != NULL && json_array_size(j_reasons) == 0) {
    res = h_update(config->conn, j_query, NULL);
    json_decref(j_query);
    if (res == H_OK) {
      j_return =  json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
    } else {
      j_return =  json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
  } else if (j_reasons != NULL && json_array_size(j_reasons) > 0) {
    j_return =  json_pack("{siso}", "result", WEBSERVICE_RESULT_PARAM, "reason", j_reasons);
  } else {
    j_return =  json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
  json_decref(j_reasons);
  return j_return;
}

/**
 * remove the specified mpd
 */
json_t * mpd_remove(struct _carleon_config * config, const char * name) {
  json_t * j_query = json_pack("{sss{ss}}",
                      "table",
                        MPD_TABLE_NAME,
                      "where",
                        "cmpd_name", name);
  int res;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  res = h_delete(config->conn, j_query, NULL);
  json_decref(j_query);
  
  if (res == H_OK) {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * Callback function
 * get the specified mpd or all mpd
 */
int callback_service_mpd_get (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * j_service_mpd;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_get - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    j_service_mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (j_service_mpd != NULL && json_integer_value(json_object_get(j_service_mpd, "result")) == WEBSERVICE_RESULT_OK) {
      response->json_body = json_copy(json_object_get(j_service_mpd, "element"));
    } else if (j_service_mpd != NULL && json_integer_value(json_object_get(j_service_mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
      response->status = 404;
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_get - Error get_service_motion_list");
      response->status = 500;
    }
    json_decref(j_service_mpd);
  }
  return U_OK;
}

/**
 * Callback function
 * add the specified mpd
 */
int callback_service_mpd_add (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_add - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    result = mpd_add((struct _carleon_config *)user_data, request->json_body);
    if (result == NULL) {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_add - Error in mpd_add");
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
 * Callback function
 * update the specified mpd
 */
int callback_service_mpd_set (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result, * mpd;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_set - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (mpd != NULL && json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
      response->status = 404;
    } else if (mpd != NULL && json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_OK) {
      result = mpd_set((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"), request->json_body);
      if (result == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_set - Error in mpd_set");
        response->status = 500;
      } else if (json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_PARAM) {
        response->json_body = json_copy(json_object_get(result, "reason"));
        response->status = 400;
      }
      json_decref(result);
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_set - Error in mpd_get");
      response->status = 500;
    }
    json_decref(mpd);
  }
  return U_OK;
}

/**
 * Callback function
 * remove the specified mpd
 */
int callback_service_mpd_remove (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result, * mpd;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_remove - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (mpd != NULL && json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
      response->status = 404;
    } else if (mpd != NULL && json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_OK) {
      result = mpd_remove((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
      if (result == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_remove - Error in mpd_remove");
        response->status = 500;
      } else if (json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_PARAM) {
        response->json_body = json_copy(json_object_get(result, "reason"));
        response->status = 400;
      }
      json_decref(result);
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_remove - Error in mpd_get");
      response->status = 500;
    }
    json_decref(mpd);
  }
  return U_OK;
}

/**
 * get the status the specified mpd
 */
json_t * mpd_get_status(struct _carleon_config * config, json_t * mpd) {
	struct mpd_connection * conn;
  json_t * to_return;
  struct mpd_status * status;
  struct mpd_song * song;
  int auth = 1;

  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
	conn = mpd_connection_new(json_string_value(json_object_get(mpd, "host")), json_integer_value(json_object_get(mpd, "port")), MPD_CONNECT_TIMEOUT);
  
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_connection_new, message is %s", mpd_connection_get_error_message(conn));
    to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  } else {
    if (json_object_get(mpd, "password") != NULL && json_is_string(json_object_get(mpd, "password")) && json_string_length(json_object_get(mpd, "password")) > 0) {
      if (!mpd_run_password(conn, json_string_value(json_object_get(mpd, "password")))) {
        auth = 0;
      }
    }
    if (auth) {
      mpd_command_list_begin(conn, true);
      mpd_send_status(conn);
      mpd_send_current_song(conn);
      mpd_command_list_end(conn);

      status = mpd_recv_status(conn);
      if (status == NULL) {
        y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_recv_status, message is %s", mpd_connection_get_error_message(conn));
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      } else {
        char * state = "";
        
        switch (mpd_status_get_state(status)) {
          case MPD_STATE_PLAY:
            state = "play";
            break;
          case MPD_STATE_PAUSE:
            state = "pause";
            break;
          case MPD_STATE_STOP:
            state = "stop";
            break;
          default:
            state = "unknown";
            break;
        }
        to_return = json_pack("{sis{sssisisi}}", 
                              "result", 
                                WEBSERVICE_RESULT_OK,
                              "status",
                                "state", state,
                                "volume", mpd_status_get_volume(status),
                                "elapsed_time", mpd_status_get_elapsed_time(status),
                                "total_time", mpd_status_get_total_time(status));
      }
      mpd_status_free(status);
      mpd_response_next(conn);
      
      song = mpd_recv_song(conn);
      if (song != NULL && to_return != NULL) {
        json_object_set_new(json_object_get(to_return, "status"), "title", json_string(mpd_song_get_tag(song, MPD_TAG_TITLE, 0)));
        json_object_set_new(json_object_get(to_return, "status"), "artist", json_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)));
        json_object_set_new(json_object_get(to_return, "status"), "album", json_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)));
        json_object_set_new(json_object_get(to_return, "status"), "date", json_string(mpd_song_get_tag(song, MPD_TAG_DATE, 0)));
        json_object_set_new(json_object_get(to_return, "status"), "name", json_string(mpd_song_get_tag(song, MPD_TAG_NAME, 0)));
        json_object_set_new(json_object_get(to_return, "status"), "track", json_string(mpd_song_get_tag(song, MPD_TAG_TRACK, 0)));
        mpd_song_free(song);
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_run_password, message is %s", mpd_connection_get_error_message(conn));
      to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
  }
  mpd_connection_free(conn);
  return to_return;
}

/**
 * Callback function
 * get the status the specified mpd
 */
int callback_service_mpd_status (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result, * mpd;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_status - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (mpd == NULL) {
      y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_status - Error getting mpd");
      response->status = 500;
    } else {
      if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
        response->status = 404;
      } else if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_OK) {
        result = mpd_get_status((struct _carleon_config *)user_data, json_object_get(mpd, "element"));
        if (result != NULL && json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_OK) {
          response->json_body = json_copy(json_object_get(result, "status"));
        } else {
          response->status = 500;
        }
        json_decref(result);
      } else {
        y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_status - Error getting mpd");
        response->status = 500;
      }
    }
    json_decref(mpd);
  }
  return U_OK;
}

/**
 * Run an action on the specified mpd (play/pause/stop)
 */
json_t * mpd_set_action(struct _carleon_config * config, json_t * mpd, const char * action) {
	struct mpd_connection *conn;
  json_t * to_return;
  int auth = 1;

  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
	conn = mpd_connection_new(json_string_value(json_object_get(mpd, "host")), json_integer_value(json_object_get(mpd, "port")), MPD_CONNECT_TIMEOUT);
  
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_connection_new, message is %s", mpd_connection_get_error_message(conn));
    to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  } else {
    if (json_object_get(mpd, "password") != NULL && json_is_string(json_object_get(mpd, "password")) && json_string_length(json_object_get(mpd, "password")) > 0) {
      if (!mpd_run_password(conn, json_string_value(json_object_get(mpd, "password")))) {
        auth = 0;
      }
    }
    if (auth) {
      to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
      if (0 == nstrcmp(action, "stop")) {
        if (!mpd_run_stop(conn)) {
          to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
        }
      } else if (0 == nstrcmp(action, "play")) {
        if (!mpd_run_play(conn)) {
          to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
        }
      } else if (0 == nstrcmp(action, "pause")) {
        if (!mpd_run_pause(conn, true)) {
          to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
        }
      } else {
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_run_password, message is %s", mpd_connection_get_error_message(conn));
      to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
  }
  mpd_connection_free(conn);
  return to_return;
}

/**
 * Callback function
 * Run an action on the specified mpd (play/pause/stop)
 */
int callback_service_mpd_action (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result, * mpd;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_action - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (mpd == NULL) {
      y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_action - Error getting mpd");
      response->status = 500;
    } else {
      if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
        response->status = 404;
      } else if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_OK) {
        result = mpd_set_action((struct _carleon_config *)user_data, json_object_get(mpd, "element"), u_map_get(request->map_url, "action_name"));
        if (result != NULL && json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_OK) {
          response->json_body = json_copy(json_object_get(result, "status"));
        } else {
          response->status = 500;
        }
        json_decref(result);
      } else {
        y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_action - Error getting mpd");
        response->status = 500;
      }
    }
    json_decref(mpd);
  }
  return U_OK;
}

/**
 * Get all the playlists available for the specified mpd
 */
json_t * mpd_get_playlists(struct _carleon_config * config, json_t * mpd) {
	struct mpd_connection * conn;
  struct mpd_playlist * playlist;
  json_t * to_return, * j_list;
  int auth = 1;

  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
	conn = mpd_connection_new(json_string_value(json_object_get(mpd, "host")), json_integer_value(json_object_get(mpd, "port")), MPD_CONNECT_TIMEOUT);
  
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_playlists - Error mpd_connection_new, message is %s", mpd_connection_get_error_message(conn));
    to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  } else {
    if (json_object_get(mpd, "password") != NULL && json_is_string(json_object_get(mpd, "password")) && json_string_length(json_object_get(mpd, "password")) > 0) {
      if (!mpd_run_password(conn, json_string_value(json_object_get(mpd, "password")))) {
        auth = 0;
      }
    }
    if (auth) {
      if (!mpd_send_list_playlists(conn)) {
        y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_playlists - Error mpd_send_list_playlists, message is %s", mpd_connection_get_error_message(conn));
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      } else {
        j_list = json_array();
        if (j_list == NULL) {
          y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_playlists - Error allocating resources for j_list");
          to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
        } else {
          while ((playlist = mpd_recv_playlist(conn)) != NULL) {
            json_array_append_new(j_list, json_string(mpd_playlist_get_path(playlist)));
            mpd_playlist_free(playlist);
          }
          to_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "list", j_list);
        }
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_run_password, message is %s", mpd_connection_get_error_message(conn));
      to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
  }
  mpd_connection_free(conn);
  return to_return;
}

/**
 * Callback function
 * Get all the playlists available for the specified mpd
 */
int callback_service_mpd_playlists_get (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result, * mpd;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_playlists_get - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (mpd == NULL) {
      y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_playlists_get - Error getting mpd");
      response->status = 500;
    } else {
      if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
        response->status = 404;
      } else if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_OK) {
        result = mpd_get_playlists((struct _carleon_config *)user_data, json_object_get(mpd, "element"));
        if (result == NULL || json_integer_value(json_object_get(result, "result")) != WEBSERVICE_RESULT_OK) {
          response->status = 500;
        } else {
          response->json_body = json_copy(json_object_get(result, "list"));
        }
        json_decref(result);
      } else {
        y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_playlists_get - Error getting mpd");
        response->status = 500;
      }
    }
    json_decref(mpd);
  }
  return U_OK;
}

/**
 * Load the specified playlist of the specified mpd
 */
json_t * mpd_load_playlist(struct _carleon_config * config, json_t * mpd, const char * playlist) {
	struct mpd_connection *conn;
  struct mpd_playlist * m_playlist;
  json_t * to_return;
  int found = 0, auth = 1;

  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
	conn = mpd_connection_new(json_string_value(json_object_get(mpd, "host")), json_integer_value(json_object_get(mpd, "port")), MPD_CONNECT_TIMEOUT);
  
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    y_log_message(Y_LOG_LEVEL_ERROR, "mpd_load_playlist - Error mpd_connection_new, message is %s", mpd_connection_get_error_message(conn));
    to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  } else {
    if (json_object_get(mpd, "password") != NULL && json_is_string(json_object_get(mpd, "password")) && json_string_length(json_object_get(mpd, "password")) > 0) {
      if (!mpd_run_password(conn, json_string_value(json_object_get(mpd, "password")))) {
        auth = 0;
      }
    }
    if (auth) {
      if (!mpd_send_list_playlists(conn)) {
        y_log_message(Y_LOG_LEVEL_ERROR, "mpd_load_playlist - Error mpd_send_list_playlists, message is %s", mpd_connection_get_error_message(conn));
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      } else {
        while ((m_playlist = mpd_recv_playlist(conn)) != NULL) {
          if (0 == nstrcmp(mpd_playlist_get_path(m_playlist), playlist)) {
            found = 1;
          }
          mpd_playlist_free(m_playlist);
        }
        if (found) {
          if (mpd_run_clear(conn) && mpd_run_load(conn, playlist) && mpd_run_play(conn)) {
            to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
          } else {
            y_log_message(Y_LOG_LEVEL_ERROR, "mpd_load_playlist - Error mpd_run_load, message is %s", mpd_connection_get_error_message(conn));
            to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
          }
        } else {
          to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_NOT_FOUND);
        }
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_run_password, message is %s", mpd_connection_get_error_message(conn));
      to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
  }
  mpd_connection_free(conn);
  return to_return;
}

/**
 * Callback function
 * Load the specified playlist of the specified mpd
 */
int callback_service_mpd_playlists_load (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result, * mpd;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_playlists_load - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (mpd == NULL) {
      y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_playlists_load - Error getting mpd");
      response->status = 500;
    } else {
      if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
        response->status = 404;
      } else if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_OK) {
        result = mpd_load_playlist((struct _carleon_config *)user_data, json_object_get(mpd, "element"), u_map_get(request->map_url, "playlist_name"));
        if (result == NULL || json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_ERROR) {
          response->status = 500;
        } else if (json_integer_value(json_object_get(result, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
          response->status = 404;
        }
        json_decref(result);
      } else {
        y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_playlists_load - Error getting mpd");
        response->status = 500;
      }
    }
    json_decref(mpd);
  }
  return U_OK;
}

/**
 * change the volume of the specified mpd
 */
json_t * mpd_set_volume(struct _carleon_config * config, json_t * mpd, uint volume) {
	struct mpd_connection * conn;
  json_t * to_return;
  int auth = 1;

  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
	conn = mpd_connection_new(json_string_value(json_object_get(mpd, "host")), json_integer_value(json_object_get(mpd, "port")), MPD_CONNECT_TIMEOUT);
  
  if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
    y_log_message(Y_LOG_LEVEL_ERROR, "mpd_volume_set - Error mpd_connection_new, message is %s", mpd_connection_get_error_message(conn));
    to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  } else {
    if (json_object_get(mpd, "password") != NULL && json_is_string(json_object_get(mpd, "password")) && json_string_length(json_object_get(mpd, "password")) > 0) {
      if (!mpd_run_password(conn, json_string_value(json_object_get(mpd, "password")))) {
        auth = 0;
      }
    }
    if (auth) {
      if (mpd_run_set_volume(conn, volume)) {
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
      } else {
        y_log_message(Y_LOG_LEVEL_ERROR, "mpd_volume_set - Error mpd_run_set_volume, message is %s", mpd_connection_get_error_message(conn));
        to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
      }
    } else {
      y_log_message(Y_LOG_LEVEL_ERROR, "mpd_get_status - Error mpd_run_password, message is %s", mpd_connection_get_error_message(conn));
      to_return = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    }
  }
  mpd_connection_free(conn);
  return to_return;
}

/**
 * Callback function
 * change the volume of the specified mpd
 */
int callback_service_mpd_volume_set (const struct _u_request * request, struct _u_response * response, void * user_data) {
  json_t * result, * mpd;
  uint volume;
  
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (user_data == NULL) {
    y_log_message(Y_LOG_LEVEL_ERROR, "callback_service_mpd_volume_set - Error, user_data is NULL");
    return U_ERROR_PARAMS;
  } else {
    mpd = mpd_get((struct _carleon_config *)user_data, u_map_get(request->map_url, "name"));
    if (mpd == NULL) {
      y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_volume_set - Error getting mpd");
      response->status = 500;
    } else {
      if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_NOT_FOUND) {
        response->status = 404;
      } else if (json_integer_value(json_object_get(mpd, "result")) == WEBSERVICE_RESULT_OK) {
        char * ptr;
        volume = strtol(u_map_get(request->map_url, "volume"), &ptr, 10);
        if (ptr == u_map_get(request->map_url, "volume")) {
          response->status = 400;
          response->json_body = json_pack("{ss}", "error", "Volume invalid");
        } else if (volume < 0 || volume > 100) {
          response->status = 400;
          response->json_body = json_pack("{ss}", "error", "Volume invalid");
        } else {
          result = mpd_set_volume((struct _carleon_config *)user_data, json_object_get(mpd, "element"), volume);
          if (result == NULL || json_integer_value(json_object_get(result, "result")) != WEBSERVICE_RESULT_OK) {
            response->status = 500;
          }
          json_decref(result);
        }
      } else {
        y_log_message(Y_LOG_LEVEL_DEBUG, "callback_service_mpd_volume_set - Error getting mpd");
        response->status = 500;
      }
    }
    json_decref(mpd);
  }
  return U_OK;
}

/**
 * Initialize the mpd service
 */
json_t * c_service_init(struct _carleon_config * config) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (config != NULL) {
    ulfius_add_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/", NULL, NULL, NULL, &callback_service_mpd_get, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/@name", NULL, NULL, NULL, &callback_service_mpd_get, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "POST", config->url_prefix, "/service-mpd/", NULL, NULL, NULL, &callback_service_mpd_add, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name", NULL, NULL, NULL, &callback_service_mpd_set, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "DELETE", config->url_prefix, "/service-mpd/@name", NULL, NULL, NULL, &callback_service_mpd_remove, (void*)config);

    ulfius_add_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/@name/status", NULL, NULL, NULL, &callback_service_mpd_status, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name/action/@action_name", NULL, NULL, NULL, &callback_service_mpd_action, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/@name/playlists", NULL, NULL, NULL, &callback_service_mpd_playlists_get, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name/playlist/@playlist_name", NULL, NULL, NULL, &callback_service_mpd_playlists_load, (void*)config);
    ulfius_add_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name/volume/@volume", NULL, NULL, NULL, &callback_service_mpd_volume_set, (void*)config);
    return json_pack("{sissss}", 
                      "result", WEBSERVICE_RESULT_OK,
                      "name", "service-mpd",
                      "description", "Control a distant MPD");
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * Closes the mpd service
 */
json_t * c_service_close(struct _carleon_config * config) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  if (config != NULL) {
    ulfius_remove_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/");
    ulfius_remove_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/@name");
    ulfius_remove_endpoint_by_val(config->instance, "POST", config->url_prefix, "/service-mpd/");
    ulfius_remove_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name");
    ulfius_remove_endpoint_by_val(config->instance, "DELETE", config->url_prefix, "/service-mpd/@name");

    ulfius_remove_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/@name/status");
    ulfius_remove_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name/action/@action_name");
    ulfius_remove_endpoint_by_val(config->instance, "GET", config->url_prefix, "/service-mpd/@name/playlists");
    ulfius_remove_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name/playlists/@playlist_name");
    ulfius_remove_endpoint_by_val(config->instance, "PUT", config->url_prefix, "/service-mpd/@name/volume/@value");

    return json_pack("{si}", "result", WEBSERVICE_RESULT_OK);
  } else {
    return json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
  }
}

/**
 * send the available commands
 */
json_t * c_service_command_get_list(struct _carleon_config * config) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  json_t * to_return = json_pack("{siso}", "result", WEBSERVICE_RESULT_OK, "commands", json_object());
  
  if (to_return != NULL) {
    json_object_set_new(json_object_get(to_return, "commands"), "action", json_pack("{s{s{ssso}}}", "parameters",
                          "action", "type", "string", "required", json_true()));
    json_object_set_new(json_object_get(to_return, "commands"), "load_playlist", json_pack("{s{s{ssso}}}", "parameters",
                          "playlist", "type", "string", "required", json_true()));
    json_object_set_new(json_object_get(to_return, "commands"), "set_volume", json_pack("{s{s{ssso}}}", "parameters",
                          "volume", "type", "integer", "required", json_true()));
  }
  return to_return;
}

/**
 * Execute a command
 */
json_t * c_service_exec(struct _carleon_config * config, const char * command, const char * element, json_t * parameters) {
	json_t * mpd, * result = NULL;

  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
	if (command != NULL) {
    mpd = mpd_get(config, element);
    if (mpd == NULL || json_integer_value(json_object_get(mpd, "result")) != WEBSERVICE_RESULT_OK) {
      result = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
    } else {
      if (0 == nstrcmp(command, "action")) {
        result = mpd_set_action(config, json_object_get(mpd, "element"), json_string_value(json_object_get(parameters, "action")));
      } else if (0 == nstrcmp(command, "load_playlist")) {
        result = mpd_load_playlist(config, json_object_get(mpd, "element"), json_string_value(json_object_get(parameters, "playlist")));
      } else if (0 == nstrcmp(command, "set_volume")) {
        result = mpd_set_volume(config, json_object_get(mpd, "element"), json_integer_value(json_object_get(parameters, "volume")));
      } else {
        result = json_pack("{si}", "result", WEBSERVICE_RESULT_NOT_FOUND);
      }
    }
    json_decref(mpd);
	} else {
		result = json_pack("{si}", "result", WEBSERVICE_RESULT_ERROR);
	}
	return result;
}

/**
 * Get the list of available elements
 */
json_t * c_service_element_get_list(struct _carleon_config * config) {
  y_log_message(Y_LOG_LEVEL_DEBUG, "Entering function %s from file %s", __PRETTY_FUNCTION__, __FILE__);
  return mpd_get(config, NULL);
}
