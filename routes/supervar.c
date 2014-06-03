 // Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: supervar.c 325541 2012-05-05 03:54:56Z nechtan $ */

#define FNX_ROUTE_SUPERVAR_PROPETY_NAME_VAR "_var_name"

zend_class_entry *fnx_route_supervar_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_route_supervar_construct_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, supervar_name)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int fnx_route_supervar_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC)
 */
int fnx_route_supervar_route(fnx_route_t *route, fnx_request_t *request TSRMLS_DC) {
	zval *varname, *zuri;
	char *req_uri;

	varname = zend_read_property(fnx_route_supervar_ce, route, ZEND_STRL(FNX_ROUTE_SUPERVAR_PROPETY_NAME_VAR), 1 TSRMLS_CC);

	zuri = fnx_request_query(FNX_GLOBAL_VARS_GET, Z_STRVAL_P(varname), Z_STRLEN_P(varname) TSRMLS_CC);

	if (!zuri || ZVAL_IS_NULL(zuri)) {
		return 0;
	}

	req_uri = estrndup(Z_STRVAL_P(zuri), Z_STRLEN_P(zuri));
    fnx_route_pathinfo_route(request, req_uri, Z_STRLEN_P(zuri) TSRMLS_CC);
	efree(req_uri);
	return 1;
}
/* }}} */

/** {{{ fnx_route_t * fnx_route_supervar_instance(fnx_route_t *this_ptr, zval *name TSRMLS_DC)
 */
fnx_route_t * fnx_route_supervar_instance(fnx_route_t *this_ptr, zval *name TSRMLS_DC) {
	fnx_route_t *instance;

	if (!name || IS_STRING != Z_TYPE_P(name) || !Z_STRLEN_P(name)) {
		return NULL;
	}

	if (this_ptr) {
		instance  = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_route_supervar_ce);
	}

	zend_update_property(fnx_route_supervar_ce, instance, ZEND_STRL(FNX_ROUTE_SUPERVAR_PROPETY_NAME_VAR), name TSRMLS_CC);

	return instance;
}
/* }}} */

/** {{{ proto public Fnx_Route_Supervar::route(string $uri)
 */
PHP_METHOD(fnx_route_supervar, route) {
	fnx_request_t *request;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &request) == FAILURE) {
		return;
	} else {
		RETURN_BOOL(fnx_route_supervar_route(getThis(), request TSRMLS_CC));
	}
}
/** }}} */

/** {{{ proto public Fnx_Route_Supervar::__construct(string $varname)
 */
PHP_METHOD(fnx_route_supervar, __construct) {
	zval *var;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &var) ==   FAILURE) {
		return;
	}

	if (Z_TYPE_P(var) != IS_STRING || !Z_STRLEN_P(var)) {
		fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "Expects a string super var name", fnx_route_supervar_ce->name);
		RETURN_FALSE;
	}

	zend_update_property(fnx_route_supervar_ce, getThis(), ZEND_STRL(FNX_ROUTE_SUPERVAR_PROPETY_NAME_VAR), var TSRMLS_CC);
}
/** }}} */

/** {{{ fnx_route_supervar_methods
 */
zend_function_entry fnx_route_supervar_methods[] = {
	PHP_ME(fnx_route_supervar, __construct, fnx_route_supervar_construct_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_route_supervar, route, fnx_route_route_arginfo, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
 */
FNX_STARTUP_FUNCTION(route_supervar) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Route_Supervar", "Fnx\\Route\\Supervar", fnx_route_supervar_methods);
	fnx_route_supervar_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	zend_class_implements(fnx_route_supervar_ce TSRMLS_CC, 1, fnx_route_ce);
	fnx_route_supervar_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(fnx_route_supervar_ce, ZEND_STRL(FNX_ROUTE_SUPERVAR_PROPETY_NAME_VAR),  ZEND_ACC_PROTECTED TSRMLS_CC);

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

