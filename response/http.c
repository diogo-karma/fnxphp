// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: http.c 321289 2011-12-21 02:53:29Z nechtan $ */

zend_class_entry *fnx_response_http_ce;

/** {{{ fnx_response_methods
*/
zend_function_entry fnx_response_http_methods[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(response_http) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Response_Http", "Fnx\\Response\\Http", fnx_response_http_methods);

	fnx_response_http_ce = zend_register_internal_class_ex(&ce, fnx_response_ce, NULL TSRMLS_CC);

	zend_declare_property_bool(fnx_response_http_ce, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_HEADEREXCEPTION), 1, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(fnx_response_http_ce, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_RESPONSECODE),	200, ZEND_ACC_PROTECTED TSRMLS_CC);

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
