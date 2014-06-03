// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_response.h 324384 2012-03-20 11:50:17Z nechtan $ */

#ifndef PHP_FNX_RESPONSE_H
#define PHP_FNX_RESPONSE_H

#define FNX_RESPONSE_PROPERTY_NAME_HEADER			"_header"
#define FNX_RESPONSE_PROPERTY_NAME_BODY				"_body"
#define FNX_RESPONSE_PROPERTY_NAME_HEADEREXCEPTION	"_sendheader"
#define FNX_RESPONSE_PROPERTY_NAME_RESPONSECODE		"_response_code"

#define FNX_RESPONSE_REPLACE 0
#define FNX_RESPONSE_PREPEND 1
#define FNX_RESPONSE_APPEND  2

extern zend_class_entry *fnx_response_ce;
extern zend_class_entry *fnx_response_http_ce;
extern zend_class_entry *fnx_response_cli_ce;

fnx_response_t * fnx_response_instance(fnx_response_t *this_ptr, char *sapi_name TSRMLS_DC);
int fnx_response_alter_body(fnx_response_t *response, char *name, int name_len, char *body, long body_len, int flag TSRMLS_DC);
int fnx_response_send(fnx_response_t *response TSRMLS_DC);
int fnx_response_set_redirect(fnx_response_t *response, char *url, int len TSRMLS_DC);
int fnx_response_clear_body(fnx_response_t *response TSRMLS_DC);

FNX_STARTUP_FUNCTION(response);

#endif
