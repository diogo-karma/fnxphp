// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: cli.c 321289 2011-12-21 02:53:29Z nechtan $ */


zend_class_entry * fnx_response_cli_ce;

/** {{{ fnx_response_methods
*/
zend_function_entry fnx_response_cli_methods[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(response_cli) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Response_Cli", "Fnx\\Response\\Cli", fnx_response_cli_methods);

	fnx_response_cli_ce = zend_register_internal_class_ex(&ce, fnx_response_ce, NULL TSRMLS_CC);

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
