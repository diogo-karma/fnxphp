// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_application.c 324897 2012-04-06 09:55:01Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "ext/standard/php_var.h"
#include "ext/standard/basic_functions.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_application.h"
#include "fnx_dispatcher.h"
#include "fnx_router.h"
#include "fnx_config.h"
#include "fnx_loader.h"
#include "fnx_request.h"
#include "fnx_bootstrap.h"
#include "fnx_exception.h"

zend_class_entry * fnx_application_ce;

/** {{{ ARG_INFO
 *  */
ZEND_BEGIN_ARG_INFO_EX(fnx_application_void_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, config)
	ZEND_ARG_INFO(0, envrion)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_app_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_execute_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, entry)
	ZEND_ARG_INFO(0, ...)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_getconfig_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_getmodule_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_getdispatch_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_bootstrap_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, bootstrap)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_environ_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_run_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_application_setappdir_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, directory)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int fnx_application_is_module_name(char *name, int len TSRMLS_DC)
*/
int fnx_application_is_module_name(char *name, int len TSRMLS_DC) {
	zval 				*modules, **ppzval;
	HashTable 			*ht;
	fnx_application_t 	*app;

	app = zend_read_static_property(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_APP), 1 TSRMLS_CC);
	if (!app || Z_TYPE_P(app) != IS_OBJECT) {
		return 0;
	}

	modules = zend_read_property(fnx_application_ce, app, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_MODULES), 1 TSRMLS_CC);
	if (!modules || Z_TYPE_P(modules) != IS_ARRAY) {
		return 0;
	}

	ht = Z_ARRVAL_P(modules);
	zend_hash_internal_pointer_reset(ht);
	while (zend_hash_get_current_data(ht, (void **)&ppzval) == SUCCESS) {
		if (Z_TYPE_PP(ppzval) == IS_STRING
				&& strncasecmp(Z_STRVAL_PP(ppzval), name, len) == 0) {
			return 1;
		}
		zend_hash_move_forward(ht);
	}
	return 0;
}
/* }}} */

/** {{{ static int fnx_application_parse_option(zval *options TSRMLS_DC)
*/
static int fnx_application_parse_option(zval *options TSRMLS_DC) {
	HashTable 	*conf;
	zval  		**ppzval, **ppsval, *app;

	conf = HASH_OF(options);
	if (zend_hash_find(conf, ZEND_STRS("application"), (void **)&ppzval) == FAILURE) {
		/* For back compatibilty */
		if (zend_hash_find(conf, ZEND_STRS("fnx"), (void **)&ppzval) == FAILURE) {
			fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "%s", "Expected an array of application configure");
			return FAILURE;
		}
	}

	app = *ppzval;
	if (Z_TYPE_P(app) != IS_ARRAY) {
		fnx_trigger_error(FNX_ERR_TYPE_ERROR TSRMLS_CC, "%s", "Expected an array of application configure");
		return FAILURE;
	}

	if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("directory"), (void **)&ppzval) == FAILURE
			|| Z_TYPE_PP(ppzval) != IS_STRING) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "%s", "Expected a directory entry in application configures");
		return FAILURE;
	}

	if (*(Z_STRVAL_PP(ppzval) + Z_STRLEN_PP(ppzval) - 1) == DEFAULT_SLASH) {
		FNX_G(directory) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval) - 1);
	} else {
		FNX_G(directory) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	}

	if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("ext"), (void **)&ppzval) == SUCCESS
			&& Z_TYPE_PP(ppzval) == IS_STRING) {
		FNX_G(ext) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	} else {
		FNX_G(ext) = FNX_DEFAULT_EXT;
	}

	if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("bootstrap"), (void **)&ppzval) == SUCCESS
			&& Z_TYPE_PP(ppzval) == IS_STRING) {
		FNX_G(bootstrap) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	}

	if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("library"), (void **)&ppzval) == SUCCESS) {
		if (IS_STRING == Z_TYPE_PP(ppzval)) {
			FNX_G(local_library) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
		} else if (IS_ARRAY == Z_TYPE_PP(ppzval)) {
			if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("directory"), (void **)&ppsval) == SUCCESS
					&& Z_TYPE_PP(ppsval) == IS_STRING) {
				FNX_G(local_library) = estrndup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
			}
			if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("namespace"), (void **)&ppsval) == SUCCESS
					&& Z_TYPE_PP(ppsval) == IS_STRING) {
				FNX_G(local_namespace) = estrndup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
			}
		}
	}

	if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("view"), (void **)&ppzval) == FAILURE 
			|| Z_TYPE_PP(ppzval) != IS_ARRAY) {
		FNX_G(view_ext) = FNX_DEFAULT_VIEW_EXT;
	} else {
		if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("ext"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			FNX_G(view_ext) = FNX_DEFAULT_VIEW_EXT;
		} else {
			FNX_G(view_ext) = estrndup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
		}
	}

	if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("baseUri"), (void **)&ppzval) == SUCCESS
			&& Z_TYPE_PP(ppzval) == IS_STRING) {
		FNX_G(base_uri) = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
	}

	if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("dispatcher"), (void **)&ppzval) == FAILURE
			|| Z_TYPE_PP(ppzval) != IS_ARRAY) {
		FNX_G(default_module) = FNX_ROUTER_DEFAULT_MODULE;
		FNX_G(default_controller) = FNX_ROUTER_DEFAULT_CONTROLLER;
		FNX_G(default_action)  = FNX_ROUTER_DEFAULT_ACTION;
	} else {
		if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("defaultModule"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			FNX_G(default_module) = FNX_ROUTER_DEFAULT_MODULE;
		} else {
			FNX_G(default_module) = zend_str_tolower_dup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
			*(FNX_G(default_module)) = toupper(*FNX_G(default_module));
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("defaultController"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			FNX_G(default_controller) = FNX_ROUTER_DEFAULT_CONTROLLER;
		} else {
			FNX_G(default_controller) = zend_str_tolower_dup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
			*(FNX_G(default_controller)) = toupper(*FNX_G(default_controller));
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("defaultAction"), (void **)&ppsval) == FAILURE
				|| Z_TYPE_PP(ppsval) != IS_STRING) {
			FNX_G(default_action)	  = FNX_ROUTER_DEFAULT_ACTION;
		} else {
			FNX_G(default_action) = zend_str_tolower_dup(Z_STRVAL_PP(ppsval), Z_STRLEN_PP(ppsval));
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("throwException"), (void **)&ppsval) == SUCCESS) {
			zval *tmp = *ppsval;
			Z_ADDREF_P(tmp);
			convert_to_boolean_ex(&tmp);
			FNX_G(throw_exception) = Z_BVAL_P(tmp);
			zval_ptr_dtor(&tmp);
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("catchException"), (void **)&ppsval) == SUCCESS) {
			zval *tmp = *ppsval;
			Z_ADDREF_P(tmp);
			convert_to_boolean_ex(&tmp);
			FNX_G(catch_exception) = Z_BVAL_P(tmp);
			zval_ptr_dtor(&tmp);
		}

		if (zend_hash_find(Z_ARRVAL_PP(ppzval), ZEND_STRS("defaultRoute"), (void **)&ppsval) == SUCCESS
				&& Z_TYPE_PP(ppsval) == IS_ARRAY) {
			FNX_G(default_route) = *ppsval;
		}
	}

	do {
		char *ptrptr;
		zval *module, *zmodules;

		MAKE_STD_ZVAL(zmodules);
		array_init(zmodules);
		if (zend_hash_find(Z_ARRVAL_P(app), ZEND_STRS("modules"), (void **)&ppzval) == SUCCESS
				&& Z_TYPE_PP(ppzval) == IS_STRING && Z_STRLEN_PP(ppzval)) {
			char *seg, *modules;
			modules = estrndup(Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
			seg = php_strtok_r(modules, ",", &ptrptr);
			while(seg) {
				if (seg && strlen(seg)) {
					MAKE_STD_ZVAL(module);
					ZVAL_STRINGL(module, seg, strlen(seg), 1);
					zend_hash_next_index_insert(Z_ARRVAL_P(zmodules),
							(void **)&module, sizeof(zval *), NULL);
				}
				seg = php_strtok_r(NULL, ",", &ptrptr);
			}
			efree(modules);
		} else {
			MAKE_STD_ZVAL(module);
			ZVAL_STRING(module, FNX_G(default_module), 1);
			zend_hash_next_index_insert(Z_ARRVAL_P(zmodules), (void **)&module, sizeof(zval *), NULL);
		}
		FNX_G(modules) = zmodules;
	} while (0);

	return SUCCESS;
}
/* }}} */

/** {{{ proto Fnx_Application::__construct(mixed $config, string $environ = FNX_G(environ))
*/
PHP_METHOD(fnx_application, __construct) {
	fnx_config_t 	 	*zconfig;
	fnx_request_t 	 	*request;
	fnx_dispatcher_t	*zdispatcher;
	fnx_application_t	*app, *self;
	fnx_loader_t		*loader;
	zval 				*config;
	zval 				*section = NULL;

	app	 = zend_read_static_property(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_APP), 1 TSRMLS_CC);

#if PHP_FNX_DEBUG
	php_error_docref(NULL, E_STRICT, "Fnx is running in debug mode");
#endif

	if (!ZVAL_IS_NULL(app)) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "Only one application can be initialized");
		RETURN_FALSE;
	}

	self = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &config, &section) == FAILURE) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "%s::__construct expects at least 1 parameter, 0 give", fnx_application_ce->name);
		return;
	}

	if (!section || Z_TYPE_P(section) != IS_STRING || !Z_STRLEN_P(section)) {
		MAKE_STD_ZVAL(section);
		ZVAL_STRING(section, FNX_G(environ), 0);
		zconfig = fnx_config_instance(NULL, config, section TSRMLS_CC);
		efree(section);
	} else {
		zconfig = fnx_config_instance(NULL, config, section TSRMLS_CC);
	}

	if  (zconfig == NULL
			|| Z_TYPE_P(zconfig) != IS_OBJECT
			|| !instanceof_function(Z_OBJCE_P(zconfig), fnx_config_ce TSRMLS_CC)
			|| fnx_application_parse_option(zend_read_property(fnx_config_ce,
				   	zconfig, ZEND_STRL(FNX_CONFIG_PROPERT_NAME), 1 TSRMLS_CC) TSRMLS_CC) == FAILURE) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "Initialization of application config failed");
		RETURN_FALSE;
	}

	request = fnx_request_instance(NULL, FNX_G(base_uri) TSRMLS_CC);
	if (FNX_G(base_uri)) {
		efree(FNX_G(base_uri));
		FNX_G(base_uri) = NULL;
	}

	if (!request) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "Initialization of request failed");
		RETURN_FALSE;
	}

	zdispatcher = fnx_dispatcher_instance(NULL TSRMLS_CC);
	fnx_dispatcher_set_request(zdispatcher, request TSRMLS_CC);
	if (NULL == zdispatcher
			|| Z_TYPE_P(zdispatcher) != IS_OBJECT
			|| !instanceof_function(Z_OBJCE_P(zdispatcher), fnx_dispatcher_ce TSRMLS_CC)) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "Instantiation of application dispatcher failed");
		RETURN_FALSE;
	}

	zend_update_property(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_CONFIG), zconfig TSRMLS_CC);
	zend_update_property(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_DISPATCHER), zdispatcher TSRMLS_CC);

	zval_ptr_dtor(&request);
	zval_ptr_dtor(&zdispatcher);
	zval_ptr_dtor(&zconfig);

	if (FNX_G(local_library)) {
		loader = fnx_loader_instance(NULL, FNX_G(local_library),
				strlen(FNX_G(global_library))? FNX_G(global_library) : NULL TSRMLS_CC);
		efree(FNX_G(local_library));
		FNX_G(local_library) = NULL;
	} else {
		char *local_library;
		spprintf(&local_library, 0, "%s%c%s", FNX_G(directory), DEFAULT_SLASH, FNX_LIBRARY_DIRECTORY_NAME);
		loader = fnx_loader_instance(NULL, local_library,
				strlen(FNX_G(global_library))? FNX_G(global_library) : NULL TSRMLS_CC);
		efree(local_library);
	}

	if (!loader) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "Initialization of application auto loader failed");
		RETURN_FALSE;
	}

	if (FNX_G(local_namespace)) {
		uint i, len;
		char *tmp = FNX_G(local_namespace);
		len  = strlen(tmp);
		if (len) {
			for(i=0; i<len; i++) {
				if (tmp[i] == ',' || tmp[i] == ' ') {
					tmp[i] = DEFAULT_DIR_SEPARATOR;
				}
			}
			fnx_loader_register_namespace_single(loader, tmp, len TSRMLS_CC);
		}
		efree(FNX_G(local_namespace));
		FNX_G(local_namespace) = NULL;
	}

	zend_update_property_bool(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_RUN), 0 TSRMLS_CC);
	zend_update_property_string(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ENV), FNX_G(environ) TSRMLS_CC);

	if (FNX_G(modules)) {
		zend_update_property(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_MODULES), FNX_G(modules) TSRMLS_CC);
		Z_DELREF_P(FNX_G(modules));
		FNX_G(modules) = NULL;
	} else {
		zend_update_property_null(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_MODULES) TSRMLS_CC);
	}

	zend_update_static_property(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_APP), self TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Fnx_Application::__desctruct(void)
*/
PHP_METHOD(fnx_application, __destruct) {
   zend_update_static_property_null(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_APP) TSRMLS_CC);
}
/* }}} */

/** {{{ proto private Fnx_Application::__sleep(void)
*/
PHP_METHOD(fnx_application, __sleep) {
}
/* }}} */

/** {{{ proto private Fnx_Application::__wakeup(void)
*/
PHP_METHOD(fnx_application, __wakeup) {
}
/* }}} */

/** {{{ proto private Fnx_Application::__clone(void)
*/
PHP_METHOD(fnx_application, __clone) {
}
/* }}} */

/** {{{ proto public Fnx_Application::run(void)
*/
PHP_METHOD(fnx_application, make) {
	zval *running;
	fnx_dispatcher_t  *dispatcher;
	fnx_response_t	  *response;
	fnx_application_t *self = getThis();

	running = zend_read_property(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_RUN), 1 TSRMLS_CC);
	if (IS_BOOL == Z_TYPE_P(running)
			&& Z_BVAL_P(running)) {
		fnx_trigger_error(FNX_ERR_STARTUP_FAILED TSRMLS_CC, "An application instance already run");
		RETURN_TRUE;
	}

	ZVAL_BOOL(running, 1);
	zend_update_property(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_RUN), running TSRMLS_CC);

	dispatcher = zend_read_property(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_DISPATCHER), 1 TSRMLS_CC);
	if ((response = fnx_dispatcher_dispatch(dispatcher TSRMLS_CC))) {
		RETURN_ZVAL(response, 1, 1);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Application::execute(callback $func)
 * We can not call to zif_call_user_func on windows, since it was not declared with dllexport
*/
PHP_METHOD(fnx_application, execute) {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
    zval *retval_ptr;
    zend_fcall_info fci;
    zend_fcall_info_cache fci_cache;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f*", &fci, &fci_cache, &fci.params, &fci.param_count) == FAILURE) {
        return;
    }

    fci.retval_ptr_ptr = &retval_ptr;

    if (zend_call_function(&fci, &fci_cache TSRMLS_CC) == SUCCESS && fci.retval_ptr_ptr && *fci.retval_ptr_ptr) {
        COPY_PZVAL_TO_ZVAL(*return_value, *fci.retval_ptr_ptr);
    }

    if (fci.params) {
        efree(fci.params);
    }
#else
    zval ***params;
    zval *retval_ptr;
    char *name;
    int argc = ZEND_NUM_ARGS();

    if (argc < 1) {
        return;
    }

    params = safe_emalloc(sizeof(zval **), argc, 0);
    if (zend_get_parameters_array_ex(1, params) == FAILURE) {
        efree(params);
        RETURN_FALSE;
    }

    if (Z_TYPE_PP(params[0]) != IS_STRING && Z_TYPE_PP(params[0]) != IS_ARRAY) {
        SEPARATE_ZVAL(params[0]);
        convert_to_string_ex(params[0]);
    }

    if (!zend_is_callable(*params[0], 0, &name)) {
        php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "First argument is expected to be a valid callback");
        efree(name);
        efree(params);
        RETURN_NULL();
    }

    if (zend_get_parameters_array_ex(argc, params) == FAILURE) {
        efree(params);
        RETURN_FALSE;
    }

    if (call_user_function_ex(EG(function_table), NULL, *params[0], &retval_ptr, argc - 1, params + 1, 0, NULL TSRMLS_CC) == SUCCESS) {
        if (retval_ptr) {
            COPY_PZVAL_TO_ZVAL(*return_value, retval_ptr);
        }
    } else {
        if (argc > 1) {
            SEPARATE_ZVAL(params[1]);
            convert_to_string_ex(params[1]);
            if (argc > 2) {
                SEPARATE_ZVAL(params[2]);
                convert_to_string_ex(params[2]);
                php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Unable to call %s(%s,%s)", name, Z_STRVAL_PP(params[1]), Z_STRVAL_PP(params[2]));
            } else {
                php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Unable to call %s(%s)", name, Z_STRVAL_PP(params[1]));
            }
        } else {
            php_error_docref1(NULL TSRMLS_CC, name, E_WARNING, "Unable to call %s()", name);
        }
    }

    efree(name);
    efree(params);
#endif
}
/* }}} */

/** {{{ proto public Fnx_Application::app(void)
*/
PHP_METHOD(fnx_application, app) {
	fnx_application_t *app = zend_read_static_property(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_APP), 1 TSRMLS_CC);
	RETVAL_ZVAL(app, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Application::getConfig(void)
*/
PHP_METHOD(fnx_application, getConfig) {
	fnx_config_t *config = zend_read_property(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_CONFIG), 1 TSRMLS_CC);
	RETVAL_ZVAL(config, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Application::getDispatcher(void)
*/
PHP_METHOD(fnx_application, getDispatcher) {
	fnx_dispatcher_t *dispatcher = zend_read_property(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_DISPATCHER), 1 TSRMLS_CC);
	RETVAL_ZVAL(dispatcher, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Application::getModules(void)
*/
PHP_METHOD(fnx_application, getModules) {
	zval *modules = zend_read_property(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_MODULES), 1 TSRMLS_CC);
	RETVAL_ZVAL(modules, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Application::environ(void)
*/
PHP_METHOD(fnx_application, environ) {
	zval *env = zend_read_property(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ENV), 1 TSRMLS_CC);
	RETURN_STRINGL(Z_STRVAL_P(env), Z_STRLEN_P(env), 1);
}
/* }}} */

/** {{{ proto public Fnx_Application::bootstrap(void)
*/
PHP_METHOD(fnx_application, configure) {
	char *bootstrap_path;
	uint len, retval = 1;
	zend_class_entry	**ce;
	fnx_application_t	*self = getThis();

	if (zend_hash_find(EG(class_table), FNX_DEFAULT_BOOTSTRAP_LOWER, FNX_DEFAULT_BOOTSTRAP_LEN, (void **) &ce) != SUCCESS) {
		if (FNX_G(bootstrap)) {
			bootstrap_path  = estrdup(FNX_G(bootstrap));
			len = strlen(FNX_G(bootstrap));
		} else {
			len = spprintf(&bootstrap_path, 0, "%s%c%s.%s", FNX_G(directory), DEFAULT_SLASH, FNX_DEFAULT_BOOTSTRAP, FNX_G(ext));
		}

		if (!fnx_loader_import(bootstrap_path, len + 1, 0 TSRMLS_CC)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Couldn't find bootstrap file %s", bootstrap_path);
			retval = 0;
		} else if (zend_hash_find(EG(class_table), FNX_DEFAULT_BOOTSTRAP_LOWER, FNX_DEFAULT_BOOTSTRAP_LEN, (void **) &ce) != SUCCESS)  {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Couldn't find class %s in %s", FNX_DEFAULT_BOOTSTRAP, bootstrap_path);
			retval = 0;
		} else if (!instanceof_function(*ce, fnx_bootstrap_ce TSRMLS_CC)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expect a %s instance, %s give", fnx_bootstrap_ce->name, (*ce)->name);
			retval = 0;
		}

		efree(bootstrap_path);
	}

	if (!retval) {
		RETURN_FALSE;
	} else {
		zval 			*bootstrap;
		HashTable 		*methods;
		fnx_dispatcher_t *dispatcher;

		MAKE_STD_ZVAL(bootstrap);
		object_init_ex(bootstrap, *ce);
		dispatcher 	= zend_read_property(fnx_application_ce, self, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_DISPATCHER), 1 TSRMLS_CC);

		methods		= &((*ce)->function_table);
		for(zend_hash_internal_pointer_reset(methods);
				zend_hash_has_more_elements(methods) == SUCCESS;
				zend_hash_move_forward(methods)) {
			uint len;
			long idx;
			char *func;
			zend_hash_get_current_key_ex(methods, &func, &len, &idx, 0, NULL);
			/* cann't use ZEND_STRL in strncasecmp, it cause a compile failed in VS2009 */
			if (strncasecmp(func, FNX_BOOTSTRAP_INITFUNC_PREFIX, sizeof(FNX_BOOTSTRAP_INITFUNC_PREFIX)-1)) {
				continue;
			}

			zend_call_method(&bootstrap, *ce, NULL, func, len - 1, NULL, 1, dispatcher, NULL TSRMLS_CC);
			/** an uncaught exception threw in function call */
			if (EG(exception)) {
				zval_dtor(bootstrap);
				efree(bootstrap);
				RETURN_FALSE;
			}
		}

		zval_dtor(bootstrap);
		efree(bootstrap);
	}

	RETVAL_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Application::getLastErrorNo(void)
*/
PHP_METHOD(fnx_application, getLastErrorNo) {
	zval *errcode = zend_read_property(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRNO), 1 TSRMLS_CC);
	RETURN_LONG(Z_LVAL_P(errcode));
}
/* }}} */

/** {{{ proto public Fnx_Application::getLastErrorMsg(void)
*/
PHP_METHOD(fnx_application, getLastErrorMsg) {
	zval *errmsg = zend_read_property(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRMSG), 1 TSRMLS_CC);
	RETURN_STRINGL(Z_STRVAL_P(errmsg), Z_STRLEN_P(errmsg), 1);
}
/* }}} */

/** {{{ proto public Fnx_Application::clearLastError(void)
*/
PHP_METHOD(fnx_application, clearLastError) {
	zend_update_property_long(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRNO), 0 TSRMLS_CC);
	zend_update_property_string(fnx_application_ce, getThis(), ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRMSG), "" TSRMLS_CC);

	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Application::setAppDirectory(string $directory)
*/
PHP_METHOD(fnx_application, setAppDirectory) {
	int	 len;
	char *directory;
	fnx_dispatcher_t *self = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &directory, &len) == FAILURE) {
		return;
	}

	if (!len || !IS_ABSOLUTE_PATH(directory, len)) {
		RETURN_FALSE;
	}

	efree(FNX_G(directory));

	FNX_G(directory) = estrndup(directory, len);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Application::getAppDirectory(void)
*/
PHP_METHOD(fnx_application, getAppDirectory) {
	RETURN_STRING(FNX_G(directory), 1);
}
/* }}} */

/** {{{ fnx_application_methods
*/
zend_function_entry fnx_application_methods[] = {
	PHP_ME(fnx_application, __construct, 	fnx_application_construct_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_application, make, 	 	 	fnx_application_run_arginfo, 		ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, execute,	  	fnx_application_execute_arginfo, 	ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, app, 	 	 	fnx_application_app_arginfo, 		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(fnx_application, environ, 	  	fnx_application_environ_arginfo, 	ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, configure,   	fnx_application_bootstrap_arginfo,  ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, getConfig,   	fnx_application_getconfig_arginfo, 	ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, getModules,   	fnx_application_getmodule_arginfo,  ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, getDispatcher, 	fnx_application_getdispatch_arginfo,ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, setAppDirectory,fnx_application_setappdir_arginfo,  ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, getAppDirectory,fnx_application_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, getLastErrorNo, fnx_application_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, getLastErrorMsg,fnx_application_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, clearLastError, fnx_application_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_application, __destruct,		NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(fnx_application, __clone,		NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CLONE)
	PHP_ME(fnx_application, __sleep,		NULL, ZEND_ACC_PRIVATE)
	PHP_ME(fnx_application, __wakeup,		NULL, ZEND_ACC_PRIVATE)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(application) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_App", "Fnx\\App", fnx_application_methods);

	fnx_application_ce 			  = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_application_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_CONFIG), 	ZEND_ACC_PROTECTED 	TSRMLS_CC);
	zend_declare_property_null(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_DISPATCHER), ZEND_ACC_PROTECTED 	TSRMLS_CC);
	zend_declare_property_null(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_APP), 	 	ZEND_ACC_STATIC|ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_MODULES), 	ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_declare_property_bool(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_RUN),	 0, ZEND_ACC_PROTECTED 	TSRMLS_CC);
	zend_declare_property_string(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ENV), FNX_G(environ), ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_declare_property_long(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRNO), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_string(fnx_application_ce, ZEND_STRL(FNX_APPLICATION_PROPERTY_NAME_ERRMSG), "", ZEND_ACC_PROTECTED TSRMLS_CC);

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
