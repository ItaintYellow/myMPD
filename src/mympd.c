/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <pwd.h>

#include "mongoose/mongoose.h"
#include "mpd_client.h"
#include "config.h"

extern char *optarg;
static sig_atomic_t s_signal_received = 0;
static struct mg_serve_http_opts s_http_server_opts;
char s_redirect[250];

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);  // Reinstantiate signal handler
  s_signal_received = sig_num;
}

static void handle_api(struct mg_connection *nc, struct http_message *hm) {
    if (!is_websocket(nc)) {
        mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Type: application/json\r\n\r\n");
    }
    char buf[1000] = {0};
    memcpy(buf, hm->body.p,sizeof(buf) - 1 < hm->body.len ? sizeof(buf) - 1 : hm->body.len);
    struct mg_str d = {buf, strlen(buf)};
    callback_mympd(nc, d);
    if (!is_websocket(nc)) {
        mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
    }
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    switch(ev) {
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
             #ifdef DEBUG
             fprintf(stdout,"New Websocket connection\n");
             #endif
             struct mg_str d = {(char *) "{\"cmd\":\"MPD_API_WELCOME\"}", 25 };
             callback_mympd(nc, d);
             break;
        }
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            #ifdef DEBUG
            printf("HTTP request: %.*s\n",hm->uri.len,hm->uri.p);
            #endif
            if (mg_vcmp(&hm->uri, "/api") == 0) {
              handle_api(nc, hm);
            }
            else {
              mg_serve_http(nc, hm, s_http_server_opts);
            }
            break;
        }
        case MG_EV_CLOSE: {
            if (is_websocket(nc)) {
              #ifdef DEBUG
              printf("Websocket connection closed\n");
              #endif
              mympd_close_handler(nc);
            }
            else {
              #ifdef DEBUG
              fprintf(stdout,"HTTP Close\n");
              #endif
            }
            break;
        }        
    }
}

static void ev_handler_http(struct mg_connection *nc_http, int ev, void *ev_data) {
    switch(ev) {
        case MG_EV_HTTP_REQUEST: {
            printf("Redirecting to %s\n", s_redirect);
            mg_http_send_redirect(nc_http, 301, mg_mk_str(s_redirect), mg_mk_str(NULL));
            break;
        }
    }
}

int main(int argc, char **argv) {
    int n, option_index = 0;
    struct mg_mgr mgr;
    struct mg_connection *nc;
    struct mg_connection *nc_http;
    unsigned int current_timer = 0, last_timer = 0;
    char *run_as_user = NULL;
    char *webport = "80";
    char *sslport = "443";
    mpd.port = 6600;
    strcpy(mpd.host, "127.0.0.1");
    streamport = 8000;
    strcpy(coverimage, "folder.jpg");
    mpd.statefile = "/var/lib/mympd/mympd.state";
    struct mg_bind_opts bind_opts;
    const char *err;
    bool ssl = false;
    char *s_ssl_cert = "/etc/mympd/ssl/server.pem";
    char *s_ssl_key = "/etc/mympd/ssl/server.key";
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    static struct option long_options[] = {
        {"mpdhost",      required_argument, 0, 'h'},
        {"mpdport",      required_argument, 0, 'p'},
        {"mpdpass",      required_argument, 0, 'm'},        
        {"webport",      required_argument, 0, 'w'},
        {"ssl",		 no_argument,	    0, 'S'},
        {"sslport",	 required_argument, 0, 'W'},
        {"sslcert",	 required_argument, 0, 'C'},
        {"sslkey",	 required_argument, 0, 'K'},
        {"user",         required_argument, 0, 'u'},
        {"streamport",	 required_argument, 0, 's'},
        {"coverimage",	 required_argument, 0, 'i'},
        {"statefile",	 required_argument, 0, 't'},
        {"version",      no_argument,       0, 'v'},
        {"help",         no_argument,       0,  0 },
        {0,              0,                 0,  0 }
    };

    while((n = getopt_long(argc, argv, "h:p:w:SW:C:K:u:vm:s:i:t:",
                long_options, &option_index)) != -1) {
        switch (n) {
            case 't':
                mpd.statefile = strdup(optarg);
                break;
            case 'h':
                strncpy(mpd.host, optarg, sizeof(mpd.host));
                break;
            case 'p':
                mpd.port = atoi(optarg);
                break;
            case 'w':
                webport = strdup(optarg);
                break;
            case 'S':
                ssl = true;
                break;
            case 'W':
                sslport = strdup(optarg);
                break;
            case 'C':
                s_ssl_cert = strdup(optarg);
                break;                
            case 'K':
                s_ssl_key = strdup(optarg);
                break;
            case 'u':
                run_as_user = strdup(optarg);
                break;
            case 'm':
                if (strlen(optarg) > 0)
                    mpd.password = strdup(optarg);
                break;
            case 's':
                streamport = atoi(optarg);
                break;
            case 'i':
                strncpy(coverimage, optarg, sizeof(coverimage));
                break;
            case 'v':
                fprintf(stdout, "myMPD  %d.%d.%d\n"
                        "Copyright (C) 2018 Juergen Mang <mail@jcgames.de>\n"
                        "Built " __DATE__ " "__TIME__"\n",
                        MYMPD_VERSION_MAJOR, MYMPD_VERSION_MINOR, MYMPD_VERSION_PATCH);
                return EXIT_SUCCESS;
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION]...\n\n"
                        " -h, --host <host>\t\tconnect to mpd at host [localhost]\n"
                        " -p, --port <port>\t\tconnect to mpd at port [6600]\n"
                        " -w, --webport [ip:]<port>\tlisten interface/port for webserver [80]\n"
                        " -S, --ssl\tenable ssl\n"
                        " -W, --sslport [ip:]<port>\tlisten interface/port for ssl webserver [443]\n"
                        " -C, --sslcert <filename>\tfilename for ssl certificate [/etc/mympd/ssl/server.pem]\n"
                        " -K, --sslkey <filename>\tfilename for ssl key [/etc/mympd/ssl/server.key]\n"
                        " -u, --user <username>\t\tdrop priviliges to user after socket bind\n"
                        " -v, --version\t\t\tget version\n"
                        " -m, --mpdpass <password>\tspecifies the password to use when connecting to mpd\n"
                        " -s, --streamport <port>\tconnect to mpd http stream at port [8000]\n"
                        " -i, --coverimage <filename>\tfilename for coverimage [folder.jpg]\n"
                        " -t, --statefile <filename>\tfilename for mympd state [/var/lib/mympd/mympd.state]\n"
                        " --help\t\t\t\tthis help\n"
                        , argv[0]);
                return EXIT_FAILURE;
        }

    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    
    mg_mgr_init(&mgr, NULL);

    if (ssl == true) {
        snprintf(s_redirect, 200, "https://%s:%s/", hostname, sslport);
        nc_http = mg_bind(&mgr, webport, ev_handler_http);
        if (nc_http == NULL) {
           fprintf(stderr, "Error starting server on port %s\n", webport );
           return EXIT_FAILURE;
        }
        memset(&bind_opts, 0, sizeof(bind_opts));
        bind_opts.ssl_cert = s_ssl_cert;
        bind_opts.ssl_key = s_ssl_key;
        bind_opts.error_string = &err;
        nc = mg_bind_opt(&mgr, sslport, ev_handler, bind_opts);
        if (nc == NULL) {
            fprintf(stderr, "Error starting server on port %s: %s\n", sslport, err);
            return EXIT_FAILURE;
        }
    }
    else {
        nc = mg_bind(&mgr, webport, ev_handler);
        if (nc == NULL) {
           fprintf(stderr, "Error starting server on port %s\n", webport );
           return EXIT_FAILURE;
        }
    }

    if (run_as_user != NULL) {
        printf("Droping privileges\n");
        struct passwd *pw;
        if ((pw = getpwnam(run_as_user)) == NULL) {
            printf("Unknown user\n");
            return EXIT_FAILURE;
        } else if (setgid(pw->pw_gid) != 0) {
            printf("setgid() failed\n");
            return EXIT_FAILURE;
        } else if (setuid(pw->pw_uid) != 0) {
            printf("setuid() failed\n");
            return EXIT_FAILURE;
        }
    }
    
    if (getuid() == 0) {
      printf("myMPD should not be run with root privileges\n");
      mg_mgr_free(&mgr);
      return EXIT_FAILURE;
    }
    
    if (ssl == true)
        mg_set_protocol_http_websocket(nc_http);
        
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = SRC_PATH;
    s_http_server_opts.enable_directory_listing = "no";

    printf("myMPD started on http port %s\n", webport);
    if (ssl == true)
        printf("myMPD started on ssl port %s\n", sslport);
        
    while (s_signal_received == 0) {
        mg_mgr_poll(&mgr, 200);
        current_timer = time(NULL);
        if (current_timer - last_timer) {
            last_timer = current_timer;
            mympd_poll(&mgr);
        }
    }
    mg_mgr_free(&mgr);
    mympd_disconnect();
    return EXIT_SUCCESS;
}
