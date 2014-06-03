// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: interface.c 325413 2012-04-23 09:19:44Z nechtan $ */

#include "ext/standard/php_smart_str.h"

#define FNX_ROUTE_PROPETY_NAME_MATCH  	"_route"
#define FNX_ROUTE_PROPETY_NAME_ROUTE  	"_default"
#define FNX_ROUTE_PROPETY_NAME_MAP	 	"_maps"
#define FNX_ROUTE_PROPETY_NAME_VERIFY 	"_verify"

#define FNX_ROUTER_URL_DELIMIETER  	 "/"
#define FNX_ROUTE_REGEX_DILIMITER  	 '#'

/* {{{ FNX_ARG_INFO
 */
FNX_BEGIN_ARG_INFO_EX(fnx_route_route_arginfo, 0, 0, 1)
	FNX_ARG_INFO(0, request)
FNX_END_ARG_INFO()
/* }}} */

zend_class_entry *fnx_route_ce;

#include "static.c"
#include "simple.c"
#include "supervar.c"
#include "rewrite.c"
#include "regex.c"
#include "map.c"

/* {{{ fnx_route_t * fnx_route_instance(fnx_route_t *this_ptr,  zval *config TSRMLS_DC)
 */
fnx_route_t * fnx_route_instance(fnx_route_t *this_ptr, zval *config TSRMLS_DC) {
	zval **match, **def, **map, **ppzval;
	fnx_route_t *instance = NULL;

	if (!config || IS_ARRAY != Z_TYPE_P(config)) {
		return NULL;
	}

	if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("type"), (void **)&ppzval) == FAILURE
			|| IS_STRING != Z_TYPE_PP(ppzval)) {
		return NULL;
	}

	if (Z_STRLEN_PP(ppzval) == (sizeof("rewrite") - 1)
			&& strncasecmp(Z_STRVAL_PP(ppzval), "rewrite", sizeof("rewrite") - 1) == 0) {
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("match"), (void **)&match) == FAILURE
				|| Z_TYPE_PP(match) != IS_STRING) {
			return NULL;
		}
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("route"), (void **)&def) == FAILURE
				|| Z_TYPE_PP(def) != IS_ARRAY) {
			return NULL;
		}

		instance = fnx_route_rewrite_instance(NULL, *match, *def, NULL TSRMLS_CC);
	} else if (Z_STRLEN_PP(ppzval) == ("regex", sizeof("regex") - 1)
			&& strncasecmp(Z_STRVAL_PP(ppzval), "regex", sizeof("regex") - 1) == 0) {
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("match"), (void **)&match) == FAILURE || Z_TYPE_PP(match) != IS_STRING) {
			return NULL;
		}
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("route"), (void **)&def) == FAILURE
				|| Z_TYPE_PP(def) != IS_ARRAY) {
			return NULL;
		}
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("map"), (void **)&map) == FAILURE || Z_TYPE_PP(map) != IS_ARRAY) {
			return NULL;
		}

		instance = fnx_route_regex_instance(NULL, *match, *def, *map, NULL TSRMLS_CC);
	} else if (Z_STRLEN_PP(ppzval) == (sizeof("map") - 1)
			&& strncasecmp(Z_STRVAL_PP(ppzval), "map", sizeof("map") - 1) == 0) {
		char *delimiter = NULL;
		uint delim_len  = 0;
		zend_bool controller_prefer = 0;
		
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("controllerPrefer"), (void **)&ppzval) == SUCCESS) {
			zval *tmp = *ppzval;
			Z_ADDREF_P(tmp);
			convert_to_boolean_ex(&tmp);
			controller_prefer = Z_BVAL_P(tmp);
			zval_ptr_dtor(&tmp);
		}

		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("delimiter"), (void **)&ppzval) == SUCCESS
				&& Z_TYPE_PP(ppzval) == IS_STRING) {
			delimiter = Z_STRVAL_PP(ppzval);
			delim_len = Z_STRLEN_PP(ppzval);
		}

		instance = fnx_route_map_instance(NULL, controller_prefer, delimiter, delim_len TSRMLS_CC);
	} else if (Z_STRLEN_PP(ppzval) == (sizeof("simple") - 1)
			&& strncasecmp(Z_STRVAL_PP(ppzval), "simple", sizeof("simple") - 1) == 0) {
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("module"), (void **)&match) == FAILURE
				|| Z_TYPE_PP(match) != IS_STRING) {
			return NULL;
		}
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("controller"), (void **)&def) == FAILURE
				|| Z_TYPE_PP(def) != IS_STRING) {
			return NULL;
		}
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("action"), (void **)&map) == FAILURE
				|| Z_TYPE_PP(map) != IS_STRING) {
			return NULL;
		}

		instance = fnx_route_simple_instance(NULL, *match, *def, *map TSRMLS_CC);
	} else if (Z_STRLEN_PP(ppzval) == (sizeof("supervar") - 1)
			&& strncasecmp(Z_STRVAL_PP(ppzval), "supervar", sizeof("supervar") - 1) == 0) {
		if (zend_hash_find(Z_ARRVAL_P(config), ZEND_STRS("varname"), (void **)&match) == FAILURE
				|| Z_TYPE_PP(match) != IS_STRING) {
			return NULL;
		}

		instance = fnx_route_supervar_instance(NULL, *match TSRMLS_CC);
	}

	return instance;
}
/* }}} */

/** {{{ fnx_route_methods
 */
zend_function_entry fnx_route_methods[] = {
	PHP_ABSTRACT_ME(fnx_route, route, fnx_route_route_arginfo)
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
 */
FNX_STARTUP_FUNCTION(route) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Route_Interface", "Fnx\\Route_Interface", fnx_route_methods);
	fnx_route_ce = zend_register_internal_interface(&ce TSRMLS_CC);

	FNX_STARTUP(route_static);
	FNX_STARTUP(route_simple);
	FNX_STARTUP(route_supervar);
	FNX_STARTUP(route_rewrite);
	FNX_STARTUP(route_regex);
	FNX_STARTUP(route_map);

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
