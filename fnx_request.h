// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_request.h 321289 2011-12-21 02:53:29Z nechtan $ */

#ifndef FNX_REQUEST_H
#define FNX_REQUEST_H

#define FNX_REQUEST_PROPERTY_NAME_MODULE		"module"
#define FNX_REQUEST_PROPERTY_NAME_CONTROLLER	"controller"
#define FNX_REQUEST_PROPERTY_NAME_ACTION		"action"
#define FNX_REQUEST_PROPERTY_NAME_METHOD		"method"
#define FNX_REQUEST_PROPERTY_NAME_PARAMS		"params"
#define FNX_REQUEST_PROPERTY_NAME_URI		"uri"
#define FNX_REQUEST_PROPERTY_NAME_STATE		"dispatched"
#define FNX_REQUEST_PROPERTY_NAME_LANG		"language"
#define FNX_REQUEST_PROPERTY_NAME_ROUTED		"routed"
#define FNX_REQUEST_PROPERTY_NAME_BASE		"_base_uri"
#define FNX_REQUEST_PROPERTY_NAME_EXCEPTION  "_exception"

#define FNX_REQUEST_SERVER_URI				"request_uri="

#define FNX_GLOBAL_VARS_TYPE					unsigned int
#define FNX_GLOBAL_VARS_POST 				TRACK_VARS_POST
#define FNX_GLOBAL_VARS_GET     				TRACK_VARS_GET
#define FNX_GLOBAL_VARS_ENV     				TRACK_VARS_ENV
#define FNX_GLOBAL_VARS_FILES   				TRACK_VARS_FILES
#define FNX_GLOBAL_VARS_SERVER  				TRACK_VARS_SERVER
#define FNX_GLOBAL_VARS_REQUEST 				TRACK_VARS_REQUEST
#define FNX_GLOBAL_VARS_COOKIE  				TRACK_VARS_COOKIE

#define FNX_REQUEST_IS_METHOD(x) \
PHP_METHOD(fnx_request, is##x) {\
	zval * method  = zend_read_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_METHOD), 1 TSRMLS_CC);\
	if (strncasecmp(#x, Z_STRVAL_P(method), Z_STRLEN_P(method)) == 0) { \
		RETURN_TRUE; \
	} \
	RETURN_FALSE; \
}

#define FNX_REQUEST_METHOD(ce, x, type) \
PHP_METHOD(ce, get##x) { \
	char *name; \
	int  len; \
    zval *ret; \
	zval *def = NULL; \
	if (ZEND_NUM_ARGS() == 0) {\
		ret = fnx_request_query(type, NULL, 0 TSRMLS_CC);\
	}else if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &name, &len, &def) == FAILURE) {\
		WRONG_PARAM_COUNT;\
	} else { \
		ret = fnx_request_query(type, name, len TSRMLS_CC); \
		if (ZVAL_IS_NULL(ret)) {\
			if (def != NULL) {\
				RETURN_ZVAL(def, 1, 0); \
			}\
		}\
	}\
	RETURN_ZVAL(ret, 1, 0);\
}

extern zend_class_entry * fnx_request_ce;

zval * fnx_request_query(uint type, char * name, uint len TSRMLS_DC);
fnx_request_t * fnx_request_instance(fnx_request_t *this_ptr, char *info TSRMLS_DC);
int fnx_request_set_base_uri(fnx_request_t *request, char *base_uri, char *request_uri TSRMLS_DC);
PHPAPI void php_session_start(TSRMLS_D);

inline zval * fnx_request_get_method(fnx_request_t *instance TSRMLS_DC);
inline zval * fnx_request_get_param(fnx_request_t *instance, char *key, int len TSRMLS_DC);
inline zval * fnx_request_get_language(fnx_request_t *instance TSRMLS_DC);

inline int fnx_request_is_routed(fnx_request_t *request TSRMLS_DC);
inline int fnx_request_is_dispatched(fnx_request_t *request TSRMLS_DC);
inline int fnx_request_set_dispatched(fnx_request_t *request, int flag TSRMLS_DC);
inline int fnx_request_set_routed(fnx_request_t *request, int flag TSRMLS_DC);
inline int fnx_request_set_params_single(fnx_request_t *instance, char *key, int len, zval *value TSRMLS_DC);
inline int fnx_request_set_params_multi(fnx_request_t *instance, zval *values TSRMLS_DC);

FNX_STARTUP_FUNCTION(request);

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
