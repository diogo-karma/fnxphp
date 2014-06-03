// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: simple.c 325432 2012-04-24 09:06:40Z nechtan $ */

zend_class_entry *fnx_route_simple_ce;

#define FNX_ROUTE_SIMPLE_VAR_NAME_MODULE		"module"
#define	FNX_ROUTE_SIMPLE_VAR_NAME_CONTROLLER 	"controller"
#define FNX_ROUTE_SIMPLE_VAR_NAME_ACTION		"action"

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_route_simple_construct_arginfo, 0, 0, 3)
	ZEND_ARG_INFO(0, module_name)
    ZEND_ARG_INFO(0, controller_name)
    ZEND_ARG_INFO(0, action_name)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int fnx_route_simple_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC)
 */
int fnx_route_simple_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC) {
	zval *module, *controller, *action;
	zval *nmodule, *ncontroller, *naction;

	nmodule 	= zend_read_property(fnx_route_simple_ce, route, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_MODULE), 1 TSRMLS_CC);
	ncontroller = zend_read_property(fnx_route_simple_ce, route, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_CONTROLLER), 1 TSRMLS_CC);
	naction 	= zend_read_property(fnx_route_simple_ce, route, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_ACTION), 1 TSRMLS_CC);

	/* if there is no expect parameter in supervars, then null will be return */
	module 		= fnx_request_query(FNX_GLOBAL_VARS_GET, Z_STRVAL_P(nmodule), Z_STRLEN_P(nmodule) TSRMLS_CC);
	controller 	= fnx_request_query(FNX_GLOBAL_VARS_GET, Z_STRVAL_P(ncontroller), Z_STRLEN_P(ncontroller) TSRMLS_CC);
	action 		= fnx_request_query(FNX_GLOBAL_VARS_GET, Z_STRVAL_P(naction), Z_STRLEN_P(naction) TSRMLS_CC);

	if (ZVAL_IS_NULL(module) && ZVAL_IS_NULL(controller) && ZVAL_IS_NULL(action)) {
		return 0;
	}

	if (Z_TYPE_P(module) == IS_STRING && fnx_application_is_module_name(Z_STRVAL_P(module), Z_STRLEN_P(module) TSRMLS_CC)) {
		zend_update_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), module TSRMLS_CC);
	}

	zend_update_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), controller TSRMLS_CC);
	zend_update_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), action TSRMLS_CC);

	return 1;
}
/* }}} */

/** {{{ fnx_route_t * fnx_route_simple_instance(fnx_route_t *this_ptr, zval *module, zval *controller, zval *action TSRMLS_DC)
 */
fnx_route_t * fnx_route_simple_instance(fnx_route_t *this_ptr, zval *module, zval *controller, zval *action TSRMLS_DC) {
	fnx_route_t *instance;

	if (this_ptr) {
		instance  = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_route_simple_ce);
	}

	zend_update_property(fnx_route_simple_ce, instance, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_MODULE), module TSRMLS_CC);
	zend_update_property(fnx_route_simple_ce, instance, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_CONTROLLER), controller TSRMLS_CC);
	zend_update_property(fnx_route_simple_ce, instance, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_ACTION), action TSRMLS_CC);

	return instance;
}
/* }}} */

/** {{{ proto public Fnx_Route_Simple::route(Fnx_Request $req)
*/
PHP_METHOD(fnx_route_simple, route) {
	fnx_request_t *request;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		return;
	} else {
		RETURN_BOOL(fnx_route_simple_route(getThis(), request TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto public Fnx_Route_Simple::__construct(string $module, string $controller, string $action)
 */
PHP_METHOD(fnx_route_simple, __construct) {
	zval *module, *controller, *action;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zzz", &module, &controller, &action) == FAILURE) {
		return;
	}

	if (IS_STRING != Z_TYPE_P(module)
			|| IS_STRING != Z_TYPE_P(controller)
			|| IS_STRING != Z_TYPE_P(action)) {
		fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "Expect 3 string paramsters", fnx_route_simple_ce->name);
		RETURN_FALSE;
	} else {
		(void)fnx_route_simple_instance(getThis(), module, controller, action TSRMLS_CC);
	}
}
/* }}} */

/** {{{ fnx_route_simple_methods
 */
zend_function_entry fnx_route_simple_methods[] = {
	PHP_ME(fnx_route_simple, __construct, fnx_route_simple_construct_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_route_simple, route, fnx_route_route_arginfo, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
 */
FNX_STARTUP_FUNCTION(route_simple) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Route_Classic", "Fnx\\Route\\Classic", fnx_route_simple_methods);
	fnx_route_simple_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	zend_class_implements(fnx_route_simple_ce TSRMLS_CC, 1, fnx_route_ce);

	fnx_route_simple_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(fnx_route_simple_ce, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_CONTROLLER), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_route_simple_ce, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_MODULE), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_route_simple_ce, ZEND_STRL(FNX_ROUTE_SIMPLE_VAR_NAME_ACTION), ZEND_ACC_PROTECTED TSRMLS_CC);

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
