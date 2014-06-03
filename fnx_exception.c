// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_exception.c 325512 2012-05-03 08:22:37Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "zend_objects.h"

#include "php_fnx.h"
#include "fnx_application.h"
#include "fnx_namespace.h"
#include "fnx_exception.h"

zend_class_entry *fnx_ce_RuntimeException;
zend_class_entry *fnx_exception_ce;

zend_class_entry *fnx_buildin_exceptions[FNX_MAX_BUILDIN_EXCEPTION];

/** {{{void fnx_trigger_error(int type TSRMLS_DC, char *format, ...)
 */
void fnx_trigger_error(int type TSRMLS_DC, char *format, ...) {
	va_list args;
	char *message;
	uint msg_len;

	va_start(args, format);
	msg_len = vspprintf(&message, 0, format, args);
	va_end(args);

	if (FNX_G(throw_exception)) {
		fnx_throw_exception(type, message TSRMLS_CC);
	} else {
		fnx_application_t *app = zend_read_static_property(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_APP), 1 TSRMLS_CC);
		zend_update_property_long(fnx_application_ce, app, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRNO), type TSRMLS_CC);
		zend_update_property_stringl(fnx_application_ce, app, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRMSG), message, msg_len TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_RECOVERABLE_ERROR, "%s", message);
	}
	efree(message);
}
/* }}} */

/** {{{ zend_class_entry * fnx_get_exception_base(int root TSRMLS_DC)
*/
zend_class_entry * fnx_get_exception_base(int root TSRMLS_DC) {
#if can_handle_soft_dependency_on_SPL && defined(HAVE_SPL) && ((PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1))
	if (!root) {
		if (!spl_ce_RuntimeException) {
			zend_class_entry **pce;

			if (zend_hash_find(CG(class_table), "runtimeexception", sizeof("RuntimeException"), (void **) &pce) == SUCCESS) {
				spl_ce_RuntimeException = *pce;
				return *pce;
			}
		} else {
			return spl_ce_RuntimeException;
		}
	}
#endif

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
	return zend_exception_get_default();
#else
	return zend_exception_get_default(TSRMLS_C);
#endif
}
/* }}} */

/** {{{ void fnx_throw_exception(long code, char *message TSRMLS_DC)
*/
void fnx_throw_exception(long code, char *message TSRMLS_DC) {
	zend_class_entry *base_exception = fnx_exception_ce;

	if ((code & FNX_ERR_BASE) == FNX_ERR_BASE
			&& fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(code)]) {
		base_exception  = fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(code)];
	}

	zend_throw_exception(base_exception, message, code TSRMLS_CC);
}
/* }}} */

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)) || (PHP_MAJOR_VERSION < 5)
/** {{{ proto Fnx_Exception::__construct($mesg = 0, $code = 0, Exception $previous = NULL)
*/
PHP_METHOD(fnx_exception, __construct) {
	char  	*message = NULL;
	zval  	*object, *previous = NULL;
	int 	message_len, code = 0;
	int    	argc = ZEND_NUM_ARGS();

	if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, argc TSRMLS_CC, "|slO!", &message, &message_len, &code, &previous, fnx_get_exception_base(0 TSRMLS_CC)) == FAILURE) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Wrong parameters for Exception([string $exception [, long $code [, Exception $previous = NULL]]])");
	}

	object = getThis();

	if (message) {
		zend_update_property_string(Z_OBJCE_P(object), object, "message", sizeof("message")-1, message TSRMLS_CC);
	}

	if (code) {
		zend_update_property_long(Z_OBJCE_P(object), object, "code", sizeof("code")-1, code TSRMLS_CC);
	}

	if (previous) {
		zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("previous"), previous TSRMLS_CC);
	}
}
/* }}} */

/** {{{ proto Fnx_Exception::getPrevious(void)
*/
PHP_METHOD(fnx_exception, getPrevious) {
	zval *prev = zend_read_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("previous"), 1 TSRMLS_CC);
	RETURN_ZVAL(prev, 1, 0);
}
/* }}} */
#endif

/** {{{ fnx_exception_methods
*/
zend_function_entry fnx_exception_methods[] = {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 3)) || (PHP_MAJOR_VERSION < 5)
	PHP_ME(fnx_exception, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_exception, getPrevious, NULL, ZEND_ACC_PUBLIC)
#endif
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(exception) {
	zend_class_entry ce;
	zend_class_entry startup_ce;
	zend_class_entry route_ce;
	zend_class_entry dispatch_ce;
	zend_class_entry loader_ce;
	zend_class_entry module_notfound_ce;
	zend_class_entry controller_notfound_ce;
	zend_class_entry action_notfound_ce;
	zend_class_entry view_notfound_ce;
	zend_class_entry type_ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Exception", "Fnx\\Exception", fnx_exception_methods);
	fnx_exception_ce = zend_register_internal_class_ex(&ce, fnx_get_exception_base(0 TSRMLS_CC), NULL TSRMLS_CC);
	zend_declare_property_null(fnx_exception_ce, ZEND_STRL("message"), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(fnx_exception_ce, ZEND_STRL("code"), 0,	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_exception_ce, ZEND_STRL("previous"),  ZEND_ACC_PROTECTED TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(startup_ce, "Fnx_Exception_StartupError", "Fnx\\Exception\\StartupError", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_STARTUP_FAILED)] = zend_register_internal_class_ex(&startup_ce, fnx_exception_ce, NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(route_ce, "Fnx_Exception_RouterFailed", "Fnx\\Exception\\RouterFailed", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_ROUTE_FAILED)] = zend_register_internal_class_ex(&route_ce, fnx_exception_ce, NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(dispatch_ce, "Fnx_Exception_DispatchFailed", "Fnx\\Exception\\DispatchFailed", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_DISPATCH_FAILED)] = zend_register_internal_class_ex(&dispatch_ce, fnx_exception_ce, NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(loader_ce, "Fnx_Exception_LoadFailed", "Fnx\\Exception\\LoadFailed", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_AUTOLOAD_FAILED)] = zend_register_internal_class_ex(&loader_ce, fnx_exception_ce, NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(module_notfound_ce, "Fnx_Exception_LoadFailed_Module", "Fnx\\Exception\\LoadFailed\\Module", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_NOTFOUND_MODULE)] = zend_register_internal_class_ex(&module_notfound_ce, fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_AUTOLOAD_FAILED)], NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(controller_notfound_ce, "Fnx_Exception_LoadFailed_Controller", "Fnx\\Exception\\LoadFailed\\Controller", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_NOTFOUND_CONTROLLER)] = zend_register_internal_class_ex(&controller_notfound_ce, fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_AUTOLOAD_FAILED)], NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(action_notfound_ce, "Fnx_Exception_LoadFailed_Action", "Fnx\\Exception\\LoadFailed\\Action", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_NOTFOUND_ACTION)] = zend_register_internal_class_ex(&action_notfound_ce, fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_AUTOLOAD_FAILED)], NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(view_notfound_ce, "Fnx_Exception_LoadFailed_View", "Fnx\\Exception\\LoadFailed\\View", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_NOTFOUND_VIEW)] = zend_register_internal_class_ex(&view_notfound_ce, fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_AUTOLOAD_FAILED)], NULL TSRMLS_CC);

	FNX_INIT_CLASS_ENTRY(type_ce, "Fnx_Exception_TypeError", "Fnx\\Exception\\TypeError", NULL);
	fnx_buildin_exceptions[FNX_EXCEPTION_OFFSET(FNX_ERR_TYPE_ERROR)] = zend_register_internal_class_ex(&type_ce, fnx_exception_ce, NULL TSRMLS_CC);

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
