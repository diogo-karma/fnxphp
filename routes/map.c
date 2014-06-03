// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: map.c 324890 2012-04-06 05:46:43Z nechtan $*/

zend_class_entry *fnx_route_map_ce;

#define FNX_ROUTE_MAP_VAR_NAME_DELIMETER	"_delimeter"
#define FNX_ROUTE_MAP_VAR_NAME_CTL_PREFER	"_ctl_router"

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_route_map_construct_arginfo, 0, 0, 0)
    ZEND_ARG_INFO(0, controller_prefer)
	ZEND_ARG_INFO(0, delimiter)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ fnx_route_t * fnx_route_map_instance(fnx_route_t *this_ptr, zend_bool controller_prefer, char *delim, uint len TSRMLS_DC)
 */
fnx_route_t * fnx_route_map_instance(fnx_route_t *this_ptr, zend_bool controller_prefer, char *delim, uint len TSRMLS_DC) {
	fnx_route_t *instance;

	if (this_ptr) {
		instance  = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_route_map_ce);
	}

	if (controller_prefer) {
		zend_update_property_bool(fnx_route_map_ce, instance,
				ZEND_STRL(FNX_ROUTE_MAP_VAR_NAME_CTL_PREFER), 1 TSRMLS_CC);
	}

	if (delim && len) {
		zend_update_property_stringl(fnx_route_map_ce, instance,
				ZEND_STRL(FNX_ROUTE_MAP_VAR_NAME_DELIMETER), delim, len TSRMLS_CC);
	}

	return instance;
}
/* }}} */

/** {{{ int fnx_route_map_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC)
*/
int fnx_route_map_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC) {
	zval *ctl_prefer, *delimer, *zuri, *base_uri, *params;
	char *req_uri, *tmp, *rest, *ptrptr, *seg;
	char *query_str = NULL;
	uint seg_len = 0;

	smart_str route_result = {0};

	zuri 	 = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI), 1 TSRMLS_CC);
	base_uri = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), 1 TSRMLS_CC);

	ctl_prefer = zend_read_property(fnx_route_map_ce, route, ZEND_STRL(FNX_ROUTE_MAP_VAR_NAME_CTL_PREFER), 1 TSRMLS_CC);
	delimer	   = zend_read_property(fnx_route_map_ce, route, ZEND_STRL(FNX_ROUTE_MAP_VAR_NAME_DELIMETER), 1 TSRMLS_CC);

	if (base_uri && IS_STRING == Z_TYPE_P(base_uri)
			&& strstr(Z_STRVAL_P(zuri), Z_STRVAL_P(base_uri)) == Z_STRVAL_P(zuri)) {
		req_uri  = estrdup(Z_STRVAL_P(zuri) + Z_STRLEN_P(base_uri));
	} else {
		req_uri  = estrdup(Z_STRVAL_P(zuri));
	}

	if (Z_TYPE_P(delimer) == IS_STRING
			&& Z_STRLEN_P(delimer)) {
		if ((query_str = strstr(req_uri, Z_STRVAL_P(delimer))) != NULL
			&& *(query_str - 1) == '/') {
			tmp  = req_uri;
			rest = query_str + Z_STRLEN_P(delimer);
			if (*rest == '\0') {
				req_uri 	= estrndup(req_uri, query_str - req_uri);
				query_str 	= NULL;
				efree(tmp);
			} else if (*rest == '/') {
				req_uri 	= estrndup(req_uri, query_str - req_uri);
				query_str   = estrdup(rest);
				efree(tmp);
			} else {
				query_str = NULL;
			}
		}
	}

	seg = php_strtok_r(req_uri, FNX_ROUTER_URL_DELIMIETER, &ptrptr);
	while (seg) {
		seg_len = strlen(seg);
		if (seg_len) {
			smart_str_appendl(&route_result, seg, seg_len);
		}
		smart_str_appendc(&route_result, '_');
		seg = php_strtok_r(NULL, FNX_ROUTER_URL_DELIMIETER, &ptrptr);
	}

	if (route_result.len) {
		if (Z_BVAL_P(ctl_prefer)) {
			zend_update_property_stringl(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), route_result.c, route_result.len - 1 TSRMLS_CC);
		} else {
			zend_update_property_stringl(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), route_result.c, route_result.len - 1 TSRMLS_CC);
		}
		efree(route_result.c);
	}

	if (query_str) {
		params = fnx_router_parse_parameters(query_str TSRMLS_CC);
		(void)fnx_request_set_params_multi(request, params TSRMLS_CC);
		zval_ptr_dtor(&params);
		efree(query_str);
	}

	efree(req_uri);

	return 1;
}
/* }}} */

/** {{{ proto public Fnx_Route_Simple::route(Fnx_Request $req)
*/
PHP_METHOD(fnx_route_map, route) {
	fnx_request_t *request;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		return;
	} else {
		RETURN_BOOL(fnx_route_map_route(getThis(), request TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto public Fnx_Route_Simple::__construct(bool $controller_prefer=FALSE, string $delimer = '#!')
*/
PHP_METHOD(fnx_route_map, __construct) {
	char *delim	= NULL;
	uint delim_len = 0;
	zend_bool controller_prefer = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|bs",
			   	&controller_prefer, &delim, &delim_len) == FAILURE) {
		return;
	}

	(void)fnx_route_map_instance(getThis(), controller_prefer, delim, delim_len TSRMLS_CC);
}
/* }}} */

/** {{{ fnx_route_map_methods
*/
zend_function_entry fnx_route_map_methods[] = {
	PHP_ME(fnx_route_map, __construct, fnx_route_map_construct_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_route_map, route, fnx_route_route_arginfo, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(route_map) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Route_Map", "Fnx\\Route\\Map", fnx_route_map_methods);
	fnx_route_map_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	zend_class_implements(fnx_route_map_ce TSRMLS_CC, 1, fnx_route_ce);

	fnx_route_map_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_bool(fnx_route_map_ce, ZEND_STRL(FNX_ROUTE_MAP_VAR_NAME_CTL_PREFER), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_route_map_ce, ZEND_STRL(FNX_ROUTE_MAP_VAR_NAME_DELIMETER),  ZEND_ACC_PROTECTED TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
