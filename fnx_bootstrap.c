// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_bootstrap.c 321289 2011-12-21 02:53:29Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "main/SAPI.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_bootstrap.h"

zend_class_entry *fnx_bootstrap_ce;

/** {{{ fnx_bootstrap_methods
*/
zend_function_entry fnx_bootstrap_methods[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(bootstrap) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Bootstrap",  "Fnx\\Bootstrap", fnx_bootstrap_methods);
	fnx_bootstrap_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_bootstrap_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

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
