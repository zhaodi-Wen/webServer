//
// Created by wzd on 19-12-4.
//

#ifndef CSERVER_HTTP_PARSE_H
#define CSERVER_HTTP_PARSE_H

#include "http.h"

#define CR '\r'
#define LF '\n'


//http请求行解析
int tk_http_parse_request_line(tk_http_request_t *request);
//http请求体解析
int tk_http_parse_request_body(tk_http_request_t *request);
#endif //CSERVER_HTTP_PARSE_H
