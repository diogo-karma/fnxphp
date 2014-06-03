// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx.c 325413 2012-04-23 09:19:44Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

#include "php_fnx.h"
#include "fnx_logo.h"
#include "fnx_loader.h"
#include "fnx_exception.h"
#include "fnx_application.h"
#include "fnx_dispatcher.h"
#include "fnx_config.h"
#include "fnx_view.h"
#include "fnx_controller.h"
#include "fnx_action.h"
#include "fnx_request.h"
#include "fnx_response.h"
#include "fnx_router.h"
#include "fnx_bootstrap.h"
#include "fnx_plugin.h"
#include "fnx_registry.h"
#include "fnx_session.h"

ZEND_DECLARE_MODULE_GLOBALS(fnx);

/* {{{ fnx_functions[]
*/
zend_function_entry fnx_functions[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_INI
 */
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("fnx.library",         	"",  PHP_INI_ALL, OnUpdateString, global_library, zend_fnx_globals, fnx_globals)
	STD_PHP_INI_BOOLEAN("fnx.action_prefer",   	"0", PHP_INI_ALL, OnUpdateBool, action_prefer, zend_fnx_globals, fnx_globals)
	STD_PHP_INI_BOOLEAN("fnx.lowcase_path",    	"0", PHP_INI_ALL, OnUpdateBool, lowcase_path, zend_fnx_globals, fnx_globals)
	STD_PHP_INI_BOOLEAN("fnx.use_spl_autoload", "0", PHP_INI_ALL, OnUpdateBool, use_spl_autoload, zend_fnx_globals, fnx_globals)
	STD_PHP_INI_ENTRY("fnx.forward_limit", 		"5", PHP_INI_ALL, OnUpdateLongGEZero, forward_limit, zend_fnx_globals, fnx_globals)
	STD_PHP_INI_BOOLEAN("fnx.name_suffix", 		"1", PHP_INI_ALL, OnUpdateBool, name_suffix, zend_fnx_globals, fnx_globals)
	STD_PHP_INI_ENTRY("fnx.name_separator", 	"_",  PHP_INI_ALL, OnUpdateString, name_separator, zend_fnx_globals, fnx_globals)
	STD_PHP_INI_BOOLEAN("fnx.cache_config",    	"0", PHP_INI_SYSTEM, OnUpdateBool, cache_config, zend_fnx_globals, fnx_globals)
/* {{{ This only effects internally */
	STD_PHP_INI_BOOLEAN("fnx.st_compatible",     "0", PHP_INI_ALL, OnUpdateBool, st_compatible, zend_fnx_globals, fnx_globals)
/* }}} */
	STD_PHP_INI_ENTRY("fnx.environ",        	"dev", PHP_INI_SYSTEM, OnUpdateString, environ, zend_fnx_globals, fnx_globals)
#ifdef FNX_HAVE_NAMESPACE
	STD_PHP_INI_BOOLEAN("fnx.use_namespace",   	"1", PHP_INI_SYSTEM, OnUpdateBool, use_namespace, zend_fnx_globals, fnx_globals)
#endif
PHP_INI_END();
/* }}} */

/** {{{ PHP_GINIT_FUNCTION
*/
PHP_GINIT_FUNCTION(fnx)
{
	fnx_globals->autoload_started   = 0;
	fnx_globals->configs			= NULL;
	fnx_globals->directory			= NULL;
	fnx_globals->local_library  = NULL;
	fnx_globals->ext			    = FNX_DEFAULT_EXT;
	fnx_globals->view_ext			= FNX_DEFAULT_VIEW_EXT;
	fnx_globals->default_module		= FNX_ROUTER_DEFAULT_MODULE;
	fnx_globals->default_controller = FNX_ROUTER_DEFAULT_CONTROLLER;
	fnx_globals->default_action		= FNX_ROUTER_DEFAULT_ACTION;
	fnx_globals->bootstrap			= FNX_DEFAULT_BOOTSTRAP;
	fnx_globals->modules			= NULL;
	fnx_globals->default_route      = NULL;
}
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(fnx)
{
	REGISTER_INI_ENTRIES();

	php_register_info_logo(FNX_LOGO_GUID, FNX_LOGO_MIME_TYPE, fnx_logo, sizeof(fnx_logo));

#ifdef FNX_HAVE_NAMESPACE
	if(FNX_G(use_namespace)) {

		REGISTER_STRINGL_CONSTANT("FNX\\VERSION", FNX_VERSION, 	sizeof(FNX_VERSION) - 1, 	CONST_PERSISTENT | CONST_CS);
		REGISTER_STRINGL_CONSTANT("FNX\\ENVIRON", FNX_G(environ), strlen(FNX_G(environ)), 	CONST_PERSISTENT | CONST_CS);

		REGISTER_STRINGL_CONSTANT("FNX\\HELLOWORLD", FNX_HELLOWORD, 	strlen(FNX_HELLOWORD), 	CONST_PERSISTENT | CONST_CS);
		REGISTER_STRINGL_CONSTANT("FNX\\ABOUT", FNX_ABOUT, 	strlen(FNX_ABOUT), 	CONST_PERSISTENT | CONST_CS);
		REGISTER_STRINGL_CONSTANT("FNX\\URL", FNX_URL, 	strlen(FNX_URL), 	CONST_PERSISTENT | CONST_CS);
		
		REGISTER_LONG_CONSTANT("FNX\\ERR\\STARTUP_FAILED", 		FNX_ERR_STARTUP_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\ROUTE_FAILED", 		FNX_ERR_ROUTE_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\DISPATCH_FAILED", 	FNX_ERR_DISPATCH_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\AUTOLOAD_FAILED", 	FNX_ERR_AUTOLOAD_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\NOTFOUND\\MODULE", 	FNX_ERR_NOTFOUND_MODULE, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\NOTFOUND\\CONTROLLER",FNX_ERR_NOTFOUND_CONTROLLER, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\NOTFOUND\\ACTION", 	FNX_ERR_NOTFOUND_ACTION, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\NOTFOUND\\VIEW", 		FNX_ERR_NOTFOUND_VIEW, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\CALL_FAILED",			FNX_ERR_CALL_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX\\ERR\\TYPE_ERROR",			FNX_ERR_TYPE_ERROR, CONST_PERSISTENT | CONST_CS);

	} else {
#endif
		REGISTER_STRINGL_CONSTANT("FNX_VERSION", FNX_VERSION, 	sizeof(FNX_VERSION) - 1, 	CONST_PERSISTENT | CONST_CS);
		REGISTER_STRINGL_CONSTANT("FNX_ENVIRON", FNX_G(environ),strlen(FNX_G(environ)), 	CONST_PERSISTENT | CONST_CS);

		REGISTER_STRINGL_CONSTANT("FNX_HELLOWORLD", FNX_HELLOWORD, 	strlen(FNX_HELLOWORD), 	CONST_PERSISTENT | CONST_CS);
		REGISTER_STRINGL_CONSTANT("FNX_ABOUT", FNX_ABOUT, 	strlen(FNX_ABOUT), 	CONST_PERSISTENT | CONST_CS);
		REGISTER_STRINGL_CONSTANT("FNX_URL", FNX_URL, 	strlen(FNX_URL), 	CONST_PERSISTENT | CONST_CS);
		
		REGISTER_LONG_CONSTANT("FNX_ERR_STARTUP_FAILED", 		FNX_ERR_STARTUP_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_ROUTE_FAILED", 			FNX_ERR_ROUTE_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_DISPATCH_FAILED", 		FNX_ERR_DISPATCH_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_AUTOLOAD_FAILED", 		FNX_ERR_AUTOLOAD_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_NOTFOUND_MODULE", 		FNX_ERR_NOTFOUND_MODULE, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_NOTFOUND_CONTROLLER", 	FNX_ERR_NOTFOUND_CONTROLLER, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_NOTFOUND_ACTION", 		FNX_ERR_NOTFOUND_ACTION, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_NOTFOUND_VIEW", 		FNX_ERR_NOTFOUND_VIEW, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_CALL_FAILED",			FNX_ERR_CALL_FAILED, CONST_PERSISTENT | CONST_CS);
		REGISTER_LONG_CONSTANT("FNX_ERR_TYPE_ERROR",			FNX_ERR_TYPE_ERROR, CONST_PERSISTENT | CONST_CS);
#ifdef FNX_HAVE_NAMESPACE
	}
#endif

	/* startup components */
	FNX_STARTUP(application);
	FNX_STARTUP(bootstrap);
	FNX_STARTUP(dispatcher);
	FNX_STARTUP(loader);
	FNX_STARTUP(request);
	FNX_STARTUP(response);
	FNX_STARTUP(controller);
	FNX_STARTUP(action);
	FNX_STARTUP(config);
	FNX_STARTUP(view);
	FNX_STARTUP(router);
	FNX_STARTUP(plugin);
	FNX_STARTUP(registry);
	FNX_STARTUP(session);
	FNX_STARTUP(exception);

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(fnx)
{
	UNREGISTER_INI_ENTRIES();

	if (FNX_G(configs)) {
		zend_hash_destroy(FNX_G(configs));
		pefree(FNX_G(configs), 1);
	}

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION(fnx)
{
	FNX_G(running)  			= 0;
	FNX_G(in_exception) 		= 0;
	FNX_G(throw_exception)   	= 1;
	FNX_G(catch_exception)   	= 0;
	FNX_G(directory)			= NULL;
	FNX_G(bootstrap)			= NULL;
	FNX_G(local_library)     	= NULL;
	FNX_G(local_namespace)     	= NULL;
	FNX_G(modules)				= NULL;
	FNX_G(base_uri)				= NULL;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	FNX_G(buffer)				= NULL;
	FNX_G(owrite_handler)		= NULL;
	FNX_G(buf_nesting)			= 0;
#endif

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(fnx)
{
	if (FNX_G(directory)) {
		efree(FNX_G(directory));
	}
	if (FNX_G(local_library)) {
		efree(FNX_G(local_library));
	}
	if (FNX_G(local_namespace)) {
		efree(FNX_G(local_namespace));
	}
	if (FNX_G(bootstrap)) {
		efree(FNX_G(bootstrap));
	}
	if (FNX_G(modules)) {
		zval_dtor(FNX_G(modules));
		efree(FNX_G(modules));
	}
	if (FNX_G(base_uri)) {
		efree(FNX_G(base_uri));
	}
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	if (FNX_G(buffer)) {
		efree(FNX_G(buffer));
	}
#endif
	FNX_G(default_route) = NULL;

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(fnx)
{
	php_info_print_table_start();
	
	php_info_print_table_header(2, "Fnx - Next generation framework standards");

	php_info_print_table_row(2, "version 1.1 stable rv.52 #nechtan", "hack@fnxinc.com");
	
	php_info_print_table_end();

	// DISPLAY_INI_ENTRIES();
}
/* }}} */

/** {{{ DL support
 */
#ifdef COMPILE_DL_FNX
ZEND_GET_MODULE(fnx)
#endif
/* }}} */

/** {{{ module depends
 */
#if ZEND_MODULE_API_NO >= 20050922
zend_module_dep fnx_deps[] = {
	ZEND_MOD_REQUIRED("spl")
	ZEND_MOD_REQUIRED("pcre")
	ZEND_MOD_OPTIONAL("session")
	{NULL, NULL, NULL}
};
#endif
/* }}} */

/** {{{ fnx_module_entry
*/
zend_module_entry fnx_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
	STANDARD_MODULE_HEADER_EX, NULL,
	fnx_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	"fnx",
	fnx_functions,
	PHP_MINIT(fnx),
	PHP_MSHUTDOWN(fnx),
	PHP_RINIT(fnx),
	PHP_RSHUTDOWN(fnx),
	PHP_MINFO(fnx),
	FNX_VERSION,
	PHP_MODULE_GLOBALS(fnx),
	PHP_GINIT(fnx),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
