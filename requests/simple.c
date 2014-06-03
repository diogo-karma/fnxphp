// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: simple.c 321289 2011-12-21 02:53:29Z nechtan $ */

static zend_class_entry *fnx_request_simple_ce;

/** {{{ fnx_request_t * fnx_request_simple_instance(fnx_request_t *this_ptr, zval *module, zval *controller, zval *action, zval *method, zval *params TSRMLS_DC)
*/
fnx_request_t * fnx_request_simple_instance(fnx_request_t *this_ptr, zval *module, zval *controller, zval *action, zval *method, zval *params TSRMLS_DC) {
	fnx_request_t *instance;

	if (!method) {
		MAKE_STD_ZVAL(method);
		if (!SG(request_info).request_method) {
			if (!strncasecmp(sapi_module.name, "cli", 3)) {
				ZVAL_STRING(method, "CLI", 1);
			} else {
				ZVAL_STRING(method, "Unknow", 1);
			}
		} else {
			ZVAL_STRING(method, (char *)SG(request_info).request_method, 1);
		}
	}

	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_request_simple_ce);
	}

	zend_update_property(fnx_request_simple_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_METHOD), method TSRMLS_CC);

	if (module || controller || action) {
		if (!module || Z_TYPE_P(module) != IS_STRING) {
			zend_update_property_string(fnx_request_simple_ce, instance,
				   	ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), FNX_G(default_module) TSRMLS_CC);
		} else {
			zend_update_property(fnx_request_simple_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), module TSRMLS_CC);
		}

		if (!controller || Z_TYPE_P(controller) != IS_STRING) {
			zend_update_property_string(fnx_request_simple_ce, instance,
				   	ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), FNX_G(default_controller) TSRMLS_CC);
		} else {
			zend_update_property(fnx_request_simple_ce, instance,
					ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), controller TSRMLS_CC);
		}

		if (!action || Z_TYPE_P(action) != IS_STRING) {
			zend_update_property_string(fnx_request_simple_ce, instance,
				   	ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), FNX_G(default_action) TSRMLS_CC);
		} else {
			zend_update_property(fnx_request_simple_ce, instance,
				   	ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), action TSRMLS_CC);
		}

		zend_update_property_bool(fnx_request_simple_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ROUTED), 1 TSRMLS_CC);
	} else {
		zval *argv, **ppzval;
		char *query = NULL;

		argv = fnx_request_query(FNX_GLOBAL_VARS_SERVER, ZEND_STRL("argv") TSRMLS_CC);
		if (IS_ARRAY == Z_TYPE_P(argv)) {
			for(zend_hash_internal_pointer_reset(Z_ARRVAL_P(argv));
					zend_hash_has_more_elements(Z_ARRVAL_P(argv)) == SUCCESS;
					zend_hash_move_forward(Z_ARRVAL_P(argv))) {
				if (zend_hash_get_current_data(Z_ARRVAL_P(argv), (void**)&ppzval) == FAILURE) {
					continue;
				} else {
					if (Z_TYPE_PP(ppzval) == IS_STRING) {
						if (strncasecmp(Z_STRVAL_PP(ppzval), FNX_REQUEST_SERVER_URI, sizeof(FNX_REQUEST_SERVER_URI) - 1)) {
							continue;
						}

						query = estrdup(Z_STRVAL_PP(ppzval) + sizeof(FNX_REQUEST_SERVER_URI));
						break;
					}
				}
			}
		}

		if (query) {
			zend_update_property_string(fnx_request_simple_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI), query TSRMLS_CC);
		} else {
			zend_update_property_string(fnx_request_simple_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI), "" TSRMLS_CC);
		}
	}

	if (!params || IS_ARRAY != Z_TYPE_P(params)) {
		MAKE_STD_ZVAL(params);
		array_init(params);
		zend_update_property(fnx_request_simple_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), params TSRMLS_CC);
		zval_ptr_dtor(&params);
	} else {
		Z_ADDREF_P(params);
		zend_update_property(fnx_request_simple_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), params TSRMLS_CC);
	}

	return instance;
}
/* }}} */

/** {{{ proto public Fnx_Request_Simple::__construct(string $method, string $module, string $controller, string $action, array $params = NULL)
*/
PHP_METHOD(fnx_request_simple, __construct) {
	zval *module 	 = NULL;
	zval *controller = NULL;
	zval *action 	 = NULL;
	zval *params	 = NULL;
	zval *method	 = NULL;
	zval *self 		 = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zzzzz", &method, &module, &controller, &action, &params) == FAILURE) {
		return;
	} else {
		if ((params && IS_ARRAY != Z_TYPE_P(params))) {
			fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC,
				   	"Expects the params is an array", fnx_request_simple_ce->name);
			RETURN_FALSE;
		}

		(void)fnx_request_simple_instance(self, module, controller, action, method, params TSRMLS_CC);
	}
}
/* }}} */

/** {{{ proto public Fnx_Request_Simple::getQuery(mixed $name, mixed $default = NULL)
*/
FNX_REQUEST_METHOD(fnx_request_simple, Query, 	FNX_GLOBAL_VARS_GET);
/* }}} */

/** {{{ proto public Fnx_Request_Simple::getPost(mixed $name, mixed $default = NULL)
*/
FNX_REQUEST_METHOD(fnx_request_simple, Post,  	FNX_GLOBAL_VARS_POST);
/* }}} */

/** {{{ proto public Fnx_Request_Simple::getRequet(mixed $name, mixed $default = NULL)
*/
FNX_REQUEST_METHOD(fnx_request_simple, Request, FNX_GLOBAL_VARS_REQUEST);
/* }}} */

/** {{{ proto public Fnx_Request_Simple::getFiles(mixed $name, mixed $default = NULL)
*/
FNX_REQUEST_METHOD(fnx_request_simple, Files, 	FNX_GLOBAL_VARS_FILES);
/* }}} */

/** {{{ proto public Fnx_Request_Simple::getCookie(mixed $name, mixed $default = NULL)
*/
FNX_REQUEST_METHOD(fnx_request_simple, Cookie, 	FNX_GLOBAL_VARS_COOKIE);
/* }}} */

/** {{{ proto public Fnx_Request_Simple::isAjax()
*/
PHP_METHOD(fnx_request_simple, isAjax) {
	zval * header = fnx_request_query(FNX_GLOBAL_VARS_SERVER, ZEND_STRL("X-Requested-With") TSRMLS_CC);
	if (Z_TYPE_P(header) == IS_STRING
			&& strncasecmp("XMLHttpRequest", Z_STRVAL_P(header), Z_STRLEN_P(header)) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Request_Simple::get(mixed $name, mixed $default)
 * params -> post -> get -> cookie -> server
 */
PHP_METHOD(fnx_request_simple, get) {
	char	*name 	= NULL;
	int 	len	 	= 0;
	zval 	*def 	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &name, &len, &def) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *value = fnx_request_get_param(getThis(), name, len TSRMLS_CC);
		if (value) {
			RETURN_ZVAL(value, 1, 0);
		} else {
			zval *params	= NULL;
			zval **ppzval	= NULL;

			FNX_GLOBAL_VARS_TYPE methods[4] = {
				FNX_GLOBAL_VARS_POST,
				FNX_GLOBAL_VARS_GET,
				FNX_GLOBAL_VARS_COOKIE,
				FNX_GLOBAL_VARS_SERVER
			};

			{
				int i = 0;
				for (;i<4; i++) {
					params = PG(http_globals)[methods[i]];
					if (params && Z_TYPE_P(params) == IS_ARRAY) {
						if (zend_hash_find(Z_ARRVAL_P(params), name, len + 1, (void **)&ppzval) == SUCCESS ){
							RETURN_ZVAL(*ppzval, 1, 0);
						}
					}
				}

			}
			if (def) {
				RETURN_ZVAL(def, 1, 0);
			}
		}
	}
	RETURN_NULL();
}
/* }}} */

/** {{{ proto private Fnx_Request_Simple::__clone
 */
PHP_METHOD(fnx_request_simple, __clone) {
}
/* }}} */

/** {{{ fnx_request_simple_methods
 */
zend_function_entry fnx_request_simple_methods[] = {
	PHP_ME(fnx_request_simple, __construct,	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_request_simple, __clone,		NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CLONE)
	PHP_ME(fnx_request_simple, getQuery, 	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request_simple, getRequest, 	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request_simple, getPost, 		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request_simple, getCookie,	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request_simple, getFiles,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request_simple, get,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request_simple, isAjax,       NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
 */
FNX_STARTUP_FUNCTION(request_simple){
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Request_Classic", "Fnx\\Request\\Classic", fnx_request_simple_methods);
	fnx_request_simple_ce = zend_register_internal_class_ex(&ce, fnx_request_ce, NULL TSRMLS_CC);
	fnx_request_simple_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_class_constant_string(fnx_request_simple_ce, ZEND_STRL("SCHEME_HTTP"),  "http" TSRMLS_CC);
	zend_declare_class_constant_string(fnx_request_simple_ce, ZEND_STRL("SCHEME_HTTPS"), "https" TSRMLS_CC);

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
