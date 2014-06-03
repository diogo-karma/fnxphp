// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: rewrite.c 321289 2011-12-21 02:53:29Z nechtan $ */

zend_class_entry *fnx_route_rewrite_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_route_rewrite_construct_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, match)
    ZEND_ARG_ARRAY_INFO(0, route, 0)
    ZEND_ARG_ARRAY_INFO(0, verify, 1)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ fnx_route_t * fnx_route_rewrite_instance(fnx_route_t *this_ptr, zval *match, zval *router, zval *verify TSRMLS_DC)
 */
fnx_route_t * fnx_route_rewrite_instance(fnx_route_t *this_ptr, zval *match, zval *route, zval *verify TSRMLS_DC) {
	fnx_route_t	*instance;

	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_route_rewrite_ce);
	}

	zend_update_property(fnx_route_rewrite_ce, instance, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_MATCH), match TSRMLS_CC);
	zend_update_property(fnx_route_rewrite_ce, instance, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_ROUTE), route TSRMLS_CC);

	if (!verify) {
		zend_update_property_null(fnx_route_rewrite_ce, instance, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_VERIFY) TSRMLS_CC);
	} else {
		zend_update_property(fnx_route_rewrite_ce, instance, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_VERIFY), verify TSRMLS_CC);
	}

	return instance;
}
/* }}} */

/** {{{ static zval * fnx_route_rewrite_match(fnx_route_t *router, char *uir, int len TSRMLS_DC)
 */
static zval * fnx_route_rewrite_match(fnx_route_t *router, char *uir, int len TSRMLS_DC) {
	char *seg, *pmatch, *ptrptr;
	int  seg_len;
	zval *match;
	pcre_cache_entry *pce_regexp;
	smart_str pattern = {0};

	if (!len) {
		return NULL;
	}

	match  = zend_read_property(fnx_route_rewrite_ce, router, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_MATCH), 1 TSRMLS_CC);
	pmatch = estrndup(Z_STRVAL_P(match), Z_STRLEN_P(match));

	smart_str_appendc(&pattern, FNX_ROUTE_REGEX_DILIMITER);
	smart_str_appendc(&pattern, '^');

	seg = php_strtok_r(pmatch, FNX_ROUTER_URL_DELIMIETER, &ptrptr);
	while (seg) {
		seg_len = strlen(seg);
		if (seg_len) {
			smart_str_appendl(&pattern, FNX_ROUTER_URL_DELIMIETER, 1);

			if(*(seg) == '*') {
				smart_str_appendl(&pattern, "(?P<__fnx_route_rest>.*)", sizeof("(?P<__fnx_route_rest>.*)") -1);
				break;
			}

			if(*(seg) == ':') {
				smart_str_appendl(&pattern, "(?P<", sizeof("(?P<") -1 );
				smart_str_appendl(&pattern, seg + 1, seg_len - 1);
				smart_str_appendl(&pattern, ">[^"FNX_ROUTER_URL_DELIMIETER"]+)", sizeof(">[^"FNX_ROUTER_URL_DELIMIETER"]+)") - 1);
			} else {
				smart_str_appendl(&pattern, seg, seg_len);
			}

		}
		seg = php_strtok_r(NULL, FNX_ROUTER_URL_DELIMIETER, &ptrptr);
	}

	efree(pmatch);
	smart_str_appendc(&pattern, FNX_ROUTE_REGEX_DILIMITER);
	smart_str_appendc(&pattern, 'i');
	smart_str_0(&pattern);

	if ((pce_regexp = pcre_get_compiled_regex_cache(pattern.c, pattern.len TSRMLS_CC)) == NULL) {
		smart_str_free(&pattern);
		return NULL;
	} else {
		zval *matches, *subparts;

		smart_str_free(&pattern);

		MAKE_STD_ZVAL(matches);
		MAKE_STD_ZVAL(subparts);
		ZVAL_LONG(matches, 0);
		ZVAL_NULL(subparts);

		php_pcre_match_impl(pce_regexp, uir, len, matches, subparts /* subpats */,
				0/* global */, 0/* ZEND_NUM_ARGS() >= 4 */, 0/*flags PREG_OFFSET_CAPTURE*/, 0/* start_offset */ TSRMLS_CC);

		if (!Z_LVAL_P(matches)) {
			zval_ptr_dtor(&matches);
			zval_ptr_dtor(&subparts);
			return NULL;
		} else {
			zval *ret, **ppzval;
			char *key;
			int	 len = 0;
			long idx = 0;
			HashTable *ht;

			MAKE_STD_ZVAL(ret);
			array_init(ret);

			ht = Z_ARRVAL_P(subparts);
			for(zend_hash_internal_pointer_reset(ht);
					zend_hash_has_more_elements(ht) == SUCCESS;
					zend_hash_move_forward(ht)) {

				if (zend_hash_get_current_key_type(ht) != HASH_KEY_IS_STRING) {
					continue;
				}

				zend_hash_get_current_key_ex(ht, &key, &len, &idx, 0, NULL);
				if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
					continue;
				}

				if (!strncmp(key, "__fnx_route_rest", len)) {
					zval *args = fnx_router_parse_parameters(Z_STRVAL_PP(ppzval) TSRMLS_CC);
					if (args) {
						zend_hash_copy(Z_ARRVAL_P(ret), Z_ARRVAL_P(args), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
						zval_ptr_dtor(&args);
					}
				} else {
					Z_ADDREF_P(*ppzval);
					zend_hash_update(Z_ARRVAL_P(ret), key, len, (void **)ppzval, sizeof(zval *), NULL);
				}
			}

			zval_ptr_dtor(&matches);
			zval_ptr_dtor(&subparts);
			return ret;
		}
	}

	return NULL;
}
/* }}} */

/** {{{ int fnx_route_rewrite_route(fnx_route_t *router, fnx_request_t *request TSRMLS_DC)
 */
int fnx_route_rewrite_route(fnx_route_t *router, fnx_request_t *request TSRMLS_DC) {
	char *request_uri;
	zval *args, *base_uri, *zuri;

	zuri 	 = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI), 1 TSRMLS_CC);
	base_uri = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), 1 TSRMLS_CC);

	if (base_uri && IS_STRING == Z_TYPE_P(base_uri)
			&& strstr(Z_STRVAL_P(zuri), Z_STRVAL_P(base_uri)) == Z_STRVAL_P(zuri)) {
		request_uri  = estrdup(Z_STRVAL_P(zuri) + Z_STRLEN_P(base_uri));
	} else {
		request_uri  = estrdup(Z_STRVAL_P(zuri));
	}

	if (!(args = fnx_route_rewrite_match(router, request_uri, strlen(request_uri) TSRMLS_CC))) {
		efree(request_uri);
		return 0;
	} else {
		zval **module, **controller, **action, *routes;

		routes = zend_read_property(fnx_route_rewrite_ce, router, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_ROUTE), 1 TSRMLS_CC);
		if (zend_hash_find(Z_ARRVAL_P(routes), ZEND_STRS("module"), (void **)&module) == SUCCESS) {
			zend_update_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), *module TSRMLS_CC);
		}

		if (zend_hash_find(Z_ARRVAL_P(routes), ZEND_STRS("controller"), (void **)&controller) == SUCCESS) {
			zend_update_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), *controller TSRMLS_CC);
		}

		if (zend_hash_find(Z_ARRVAL_P(routes), ZEND_STRS("action"), (void **)&action) == SUCCESS) {
			zend_update_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), *action TSRMLS_CC);
		}

		(void)fnx_request_set_params_multi(request, args TSRMLS_CC);
		zval_ptr_dtor(&args);
		efree(request_uri);
		return 1;
	}

}
/* }}} */

/** {{{ proto public Fnx_Route_Rewrite::route(Fnx_Request_Abstarct $request)
 */
PHP_METHOD(fnx_route_rewrite, route) {
	fnx_route_t 	*route;
	fnx_request_t 	*request;

	route = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		return;
	}

	if (!request || IS_OBJECT != Z_TYPE_P(request)
			|| !instanceof_function(Z_OBJCE_P(request), fnx_request_ce TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expect a %s instance", fnx_request_ce->name);
		RETURN_FALSE;
	}

	RETURN_BOOL(fnx_route_rewrite_route(route, request TSRMLS_CC));
}
/** }}} */

/** {{{ proto public Fnx_Route_Rewrite::match(string $uri)
 */
PHP_METHOD(fnx_route_rewrite, match) {
	char *uri;
	uint len;
	zval *matches;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &uri, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (!len) RETURN_FALSE;

	if ((matches = fnx_route_rewrite_match(getThis(), uri, len TSRMLS_CC))) {
		RETURN_ZVAL(matches, 0, 0);
	}

	RETURN_FALSE;
}
/** }}} */

/** {{{ proto public Fnx_Route_Rewrite::__construct(string $match, array $route, array $verify = NULL)
 */
PHP_METHOD(fnx_route_rewrite, __construct) {
	zval 		*match, *route, *verify = NULL;
	fnx_route_t	*self = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "za|a", &match, &route, &verify) ==  FAILURE) {
		return;
	}

	if (IS_STRING != Z_TYPE_P(match) || !Z_STRLEN_P(match)) {
		fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "Expects a string as the first parameter", fnx_route_rewrite_ce->name);
		RETURN_FALSE;
	}

	if (IS_ARRAY != Z_TYPE_P(route)) {
		fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "Expects an array as the second parameter", fnx_route_rewrite_ce->name);
		RETURN_FALSE;
	}

	if (verify && IS_ARRAY != Z_TYPE_P(verify)) {
		fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "Expects an array as third parameter",  fnx_route_rewrite_ce->name);
		RETURN_FALSE;
	}

	(void)fnx_route_rewrite_instance(self, match, route, verify TSRMLS_CC);

	if (self) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/** }}} */

/** {{{ fnx_route_rewrite_methods
 */
zend_function_entry fnx_route_rewrite_methods[] = {
	PHP_ME(fnx_route_rewrite, __construct, fnx_route_rewrite_construct_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_route_rewrite, route, fnx_route_route_arginfo, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
 */
FNX_STARTUP_FUNCTION(route_rewrite) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Route_Rewrite", "Fnx\\Route\\Rewrite", fnx_route_rewrite_methods);
	fnx_route_rewrite_ce = zend_register_internal_class_ex(&ce, fnx_route_ce, NULL TSRMLS_CC);
	zend_class_implements(fnx_route_rewrite_ce TSRMLS_CC, 1, fnx_route_ce);
	fnx_route_rewrite_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(fnx_route_rewrite_ce, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_MATCH),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_route_rewrite_ce, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_ROUTE),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_route_rewrite_ce, ZEND_STRL(FNX_ROUTE_PROPETY_NAME_VERIFY), ZEND_ACC_PROTECTED TSRMLS_CC);

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

