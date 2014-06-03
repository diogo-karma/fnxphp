// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: static.c 325563 2012-05-07 08:21:57Z nechtan $ */

zend_class_entry * fnx_route_static_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_route_static_match_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, uri)
ZEND_END_ARG_INFO()
/* }}} */

static int fnx_route_pathinfo_route(fnx_request_t *request, char *req_uri, int req_uri_len TSRMLS_DC) /* {{{ */ {
	zval *params;
	char *module = NULL, *controller = NULL, *action = NULL, *rest = NULL;

	do {
#define strip_slashs(p) while (*p == ' ' || *p == '/') { ++p; }
		char *s, *p, *q;
		char *uri;

		if (req_uri_len == 0
				|| (req_uri_len == 1 && *req_uri == '/')) {
			break;
		}

		uri = req_uri;
		s = p = uri;
		q = req_uri + req_uri_len - 1;

		while (*q == ' ' || *q == '/') {
			*q-- = '\0';
		}

		strip_slashs(p);

		if ((s = strstr(p, "/")) != NULL) {
			if (fnx_application_is_module_name(p, s-p TSRMLS_CC)) {
				module = estrndup(p, s - p);
				p  = s + 1;
		        strip_slashs(p);
				if ((s = strstr(p, "/")) != NULL) {
					controller = estrndup(p, s - p);
					p  = s + 1;
				}
			} else {
				controller = estrndup(p, s - p);
				p  = s + 1;
			}
		}

		strip_slashs(p);
		if ((s = strstr(p, "/")) != NULL) {
			action = estrndup(p, s - p);
			p  = s + 1;
		}

		strip_slashs(p);
		if (*p != '\0') {
			do {
				if (!module && !controller && !action) {
					if (fnx_application_is_module_name(p, strlen(p) TSRMLS_CC)) {
						module = estrdup(p);
						break;
					}
				}

				if (!controller) {
					controller = estrdup(p);
					break;
				}

				if (!action) {
					action = estrdup(p);
					break;
				}

				rest = estrdup(p);
			} while (0);
		}

		if (module && controller == NULL) {
			controller = module;
			module = NULL;
		} else if (module && action == NULL) {
			action = controller;
			controller = module;
			module = NULL;
	    } else if (controller && action == NULL ) {
			/* /controller */
			if (FNX_G(action_prefer)) {
				action = controller;
				controller = NULL;
			}
		}
	} while (0);

	if (module != NULL) {
		zend_update_property_string(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), module TSRMLS_CC);
		efree(module);
	}
	if (controller != NULL) {
		zend_update_property_string(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), controller TSRMLS_CC);
		efree(controller);
	}

	if (action != NULL) {
		zend_update_property_string(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), action TSRMLS_CC);
		efree(action);
	}

	if (rest) {
		params = fnx_router_parse_parameters(rest TSRMLS_CC);
		(void)fnx_request_set_params_multi(request, params TSRMLS_CC);
		zval_ptr_dtor(&params);
		efree(rest);
	}

}
/* }}} */

/** {{{ int fnx_route_static_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC)
 */
int fnx_route_static_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC) {
	zval *zuri, *base_uri;
	char *req_uri;

	zuri 	 = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI), 1 TSRMLS_CC);
	base_uri = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), 1 TSRMLS_CC);

	if (base_uri && IS_STRING == Z_TYPE_P(base_uri)
			&& strstr(Z_STRVAL_P(zuri), Z_STRVAL_P(base_uri)) == Z_STRVAL_P(zuri)) {
		req_uri  = estrdup(Z_STRVAL_P(zuri) + Z_STRLEN_P(base_uri));
	} else {
		req_uri  = estrdup(Z_STRVAL_P(zuri));
	}

	fnx_route_pathinfo_route(request, req_uri, Z_STRLEN_P(zuri) TSRMLS_CC);
	efree(req_uri);
	return 1;
}
/* }}} */

/** {{{ proto public Fnx_Router_Classical::route(Fnx_Request $req)
*/
PHP_METHOD(fnx_route_static, route) {
	fnx_request_t *request;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		return;
	} else {
		RETURN_BOOL(fnx_route_static_route(getThis(), request TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto public Fnx_Router_Classical::match(string $uri)
*/
PHP_METHOD(fnx_route_static, match) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ fnx_route_static_methods
 */
zend_function_entry fnx_route_static_methods[] = {
	PHP_ME(fnx_route_static, match, fnx_route_static_match_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_route_static, route, fnx_route_route_arginfo, 		ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
 */
FNX_STARTUP_FUNCTION(route_static) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Route_Static", "Fnx\\Route\\Static", fnx_route_static_methods);
	fnx_route_static_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	zend_class_implements(fnx_route_static_ce TSRMLS_CC, 1, fnx_router_ce);

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
