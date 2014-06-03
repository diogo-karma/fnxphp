// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_action.c 321289 2011-12-21 02:53:29Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_request.h"
#include "fnx_response.h"
#include "fnx_view.h"
#include "fnx_exception.h"
#include "fnx_controller.h"
#include "fnx_action.h"

zend_class_entry *fnx_action_ce;

/** {{{ ARG_INFO
 */

/* }}} */

/** {{{ proto public Fnx_Action_Abstract::getController(void)
*/
PHP_METHOD(fnx_action, getController) {
	fnx_controller_t *controller = zend_read_property(fnx_action_ce, getThis(), ZEND_STRL(FNX_ACTION_PROPERTY_NAME_CTRL), 1 TSRMLS_CC);
	RETURN_ZVAL(controller, 1, 0);
}
/* }}} */

/** {{{ fnx_controller_methods
*/
zend_function_entry fnx_action_methods[] = {
	PHP_ABSTRACT_ME(fnx_action_controller, execute, NULL)
	PHP_ME(fnx_action, getController, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(action) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Action", "Fnx\\Action", fnx_action_methods);
	fnx_action_ce = zend_register_internal_class_ex(&ce, fnx_controller_ce, NULL TSRMLS_CC);
	fnx_action_ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;

	zend_declare_property_null(fnx_action_ce, ZEND_STRL(FNX_ACTION_PROPERTY_NAME_CTRL),	ZEND_ACC_PROTECTED TSRMLS_CC);

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
