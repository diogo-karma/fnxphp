// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_plugin.c 321289 2011-12-21 02:53:29Z nechtan $ */

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
#include "fnx_namespace.h"
#include "fnx_plugin.h"

zend_class_entry * fnx_plugin_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(plugin_arg, 0, 0, 2)
	ZEND_ARG_OBJ_INFO(0, request, Fnx_Request, 0)
	ZEND_ARG_OBJ_INFO(0, response, Fnx_Response, 0)
ZEND_END_ARG_INFO()

#ifdef FNX_HAVE_NAMESPACE
ZEND_BEGIN_ARG_INFO_EX(plugin_arg_ns, 0, 0, 2)
	ZEND_ARG_OBJ_INFO(0, request, Fnx\\Request, 0)
	ZEND_ARG_OBJ_INFO(0, response, Fnx\\Response, 0)
ZEND_END_ARG_INFO()
#endif
/* }}} */

/** {{{ proto public Fnx_Plugin::routerStartup(Fnx_Request_Abstract $request, Fnx_Response_Abstarct $response)
*/
PHP_METHOD(fnx_plugin, routerStartup) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Plugin::routerShutdown(Fnx_Request_Abstract $request, Fnx_Response_Abstarct $response)
*/
PHP_METHOD(fnx_plugin, routerShutdown) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Plugin::dispatchLoopStartup(Fnx_Request_Abstract $request, Fnx_Response_Abstarct $response)
*/
PHP_METHOD(fnx_plugin, dispatchLoopStartup) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Plugin::preDispatch(Fnx_Request_Abstract $request, Fnx_Response_Abstarct $response)
*/
PHP_METHOD(fnx_plugin, preDispatch) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Plugin::postDispatch(Fnx_Request_Abstract $request, Fnx_Response_Abstarct $response)
*/
PHP_METHOD(fnx_plugin, postDispatch) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Plugin::dispatchLoopShutdown(Fnx_Request_Abstract $request, Fnx_Response_Abstarct $response)
*/
PHP_METHOD(fnx_plugin, dispatchLoopShutdown) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Plugin::preResponse(Fnx_Request_Abstract $request, Fnx_Response_Abstarct $response)
*/
PHP_METHOD(fnx_plugin, preResponse) {
	RETURN_TRUE;
}
/* }}} */

/** {{{ fnx_plugin_methods
*/
zend_function_entry fnx_plugin_methods[] = {
	PHP_ME(fnx_plugin, routerStartup,  		 plugin_arg, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, routerShutdown,  		 plugin_arg, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, dispatchLoopStartup,   plugin_arg, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, dispatchLoopShutdown,  plugin_arg, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, preDispatch,  		 plugin_arg, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, postDispatch, 		 plugin_arg, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, preResponse, 			 plugin_arg, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};

#ifdef FNX_HAVE_NAMESPACE
zend_function_entry fnx_plugin_methods_ns[] = {
	PHP_ME(fnx_plugin, routerStartup,  		 plugin_arg_ns, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, routerShutdown,  		 plugin_arg_ns, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, dispatchLoopStartup,   plugin_arg_ns, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, dispatchLoopShutdown,  plugin_arg_ns, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, preDispatch,  		 plugin_arg_ns, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, postDispatch, 		 plugin_arg_ns, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_plugin, preResponse, 			 plugin_arg_ns, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
#endif
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(plugin) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Plugin", "Fnx\\Plugin", namespace_switch(fnx_plugin_methods));
	fnx_plugin_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_plugin_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

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
