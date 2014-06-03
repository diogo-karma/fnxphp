// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_request.c 325277 2012-04-18 09:06:41Z nechtan $*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/php_string.h"

#include "php_fnx.h"
#include "fnx_request.h"
#include "fnx_namespace.h"
#include "fnx_exception.h"

#include "requests/simple.c"
#include "requests/http.c"

zend_class_entry *fnx_request_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_request_void_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_set_routed_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, flag)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_set_module_name_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, module)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_set_controller_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, controller)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_set_action_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, action)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_set_baseuir_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, uir)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_set_request_uri_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, uir)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_set_param_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_get_param_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, default)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_getserver_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, default)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_request_getenv_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, default)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ fnx_request_t * fnx_request_instance(zval *this_ptr, char *other TSRMLS_DC)
*/
fnx_request_t * fnx_request_instance(fnx_request_t *this_ptr, char *other TSRMLS_DC) {
	fnx_request_t *instance = fnx_request_http_instance(this_ptr, NULL, other TSRMLS_CC);
	return instance;
}
/* }}} */

/** {{{ int fnx_request_set_base_uri(fnx_request_t *request, char *base_uri, char *request_uri TSRMLS_DC)
*/
int fnx_request_set_base_uri(fnx_request_t *request, char *base_uri, char *request_uri TSRMLS_DC) {
	char *basename = NULL;
	uint basename_len = 0;

	if (!base_uri) {
		zval 	*script_filename;
		char 	*file_name, *ext = FNX_G(ext);
		size_t 	file_name_len;
		uint  	ext_len;

		ext_len	= strlen(ext);

		script_filename = fnx_request_query(FNX_GLOBAL_VARS_SERVER, ZEND_STRL("SCRIPT_FILENAME") TSRMLS_CC);

		do {
			if (script_filename && IS_STRING == Z_TYPE_P(script_filename)) {
				zval *script_name, *phpself_name, *orig_name;

				script_name = fnx_request_query(FNX_GLOBAL_VARS_SERVER, ZEND_STRL("SCRIPT_NAME") TSRMLS_CC);
				php_basename(Z_STRVAL_P(script_filename), Z_STRLEN_P(script_filename), ext, ext_len, &file_name, &file_name_len TSRMLS_CC);
				if (script_name && IS_STRING == Z_TYPE_P(script_name)) {
					char 	*script;
					size_t 	script_len;

					php_basename(Z_STRVAL_P(script_name), Z_STRLEN_P(script_name),
							NULL, 0, &script, &script_len TSRMLS_CC);

					if (strncmp(file_name, script, file_name_len) == 0) {
						basename 	 = Z_STRVAL_P(script_name);
						basename_len = Z_STRLEN_P(script_name);
						efree(file_name);
						efree(script);
						break;
					}
					efree(script);
				}

				phpself_name = fnx_request_query(FNX_GLOBAL_VARS_SERVER, ZEND_STRL("PHP_SELF") TSRMLS_CC);
				if (phpself_name && IS_STRING == Z_TYPE_P(phpself_name)) {
					char 	*phpself;
					size_t	phpself_len;

					php_basename(Z_STRVAL_P(phpself_name), Z_STRLEN_P(phpself_name), NULL, 0, &phpself, &phpself_len TSRMLS_CC);
					if (strncmp(file_name, phpself, file_name_len) == 0) {
						basename	 = Z_STRVAL_P(phpself_name);
						basename_len = Z_STRLEN_P(phpself_name);
						efree(file_name);
						efree(phpself);
						break;
					}
					efree(phpself);
				}

				orig_name = fnx_request_query(FNX_GLOBAL_VARS_SERVER, ZEND_STRL("ORIG_SCRIPT_NAME") TSRMLS_CC);
				if (orig_name && IS_STRING == Z_TYPE_P(orig_name)) {
					char 	*orig;
					size_t	orig_len;
					php_basename(Z_STRVAL_P(orig_name), Z_STRLEN_P(orig_name), NULL, 0, &orig, &orig_len TSRMLS_CC);
					if (strncmp(file_name, orig, file_name_len) == 0) {
						basename 	 = Z_STRVAL_P(orig_name);
						basename_len = Z_STRLEN_P(orig_name);
						efree(file_name);
						efree(orig);
						break;
					}
					efree(orig);
				}
				efree(file_name);
			}
		} while (0);

		if (basename && strstr(request_uri, basename) == request_uri) {
			if (*(basename + basename_len - 1) == '/') {
				--basename_len;
			}
			zend_update_property_stringl(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), basename, basename_len TSRMLS_CC);
			return 1;
		} else if (basename) {
			char 	*dir;
			size_t  dir_len;

			dir_len = php_dirname(basename, basename_len);
			if (*(basename + dir_len - 1) == '/') {
				--dir_len;
			}

			if (dir_len) {
				dir = estrndup(basename, dir_len);
				if (strstr(request_uri, dir) == request_uri) {
					zend_update_property_string(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), dir TSRMLS_CC);
					efree(dir);
					return 1;
				}
				efree(dir);
			}
		}
		zend_update_property_string(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), "" TSRMLS_CC);
		return 1;
	} else {
		zend_update_property_string(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), base_uri TSRMLS_CC);
		return 1;
	}
}
/* }}} */

/** {{{ zval * fnx_request_query(uint type, char * name, uint len TSRMLS_DC)
*/
zval * fnx_request_query(uint type, char * name, uint len TSRMLS_DC) {
	zval 		**carrier, **ret;

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4)
	zend_bool 	jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));
#else
	zend_bool 	jit_initialization = PG(auto_globals_jit);
#endif

	/* for phpunit test requirements */
#if PHP_FNX_DEBUG
	switch (type) {
		case FNX_GLOBAL_VARS_POST:
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_POST"), (void **)&carrier);
			break;
		case FNX_GLOBAL_VARS_GET:
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_GET"), (void **)&carrier);
			break;
		case FNX_GLOBAL_VARS_COOKIE:
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_COOKIE"), (void **)&carrier);
			break;
		case FNX_GLOBAL_VARS_SERVER:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
			}
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_SERVER"), (void **)&carrier);
			break;
		case FNX_GLOBAL_VARS_ENV:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_ENV") TSRMLS_CC);
			}
			carrier = &PG(http_globals)[FNX_GLOBAL_VARS_ENV];
			break;
		case FNX_GLOBAL_VARS_FILES:
			carrier = &PG(http_globals)[FNX_GLOBAL_VARS_FILES];
			break;
		case FNX_GLOBAL_VARS_REQUEST:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_REQUEST") TSRMLS_CC);
			}
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_REQUEST"), (void **)&carrier);
			break;
		default:
			break;
	}
#else
	switch (type) {
		case FNX_GLOBAL_VARS_POST:
		case FNX_GLOBAL_VARS_GET:
		case FNX_GLOBAL_VARS_FILES:
		case FNX_GLOBAL_VARS_COOKIE:
			carrier = &PG(http_globals)[type];
			break;
		case FNX_GLOBAL_VARS_ENV:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_ENV") TSRMLS_CC);
			}
			carrier = &PG(http_globals)[type];
			break;
		case FNX_GLOBAL_VARS_SERVER:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
			}
			carrier = &PG(http_globals)[type];
			break;
		case FNX_GLOBAL_VARS_REQUEST:
			if (jit_initialization) {
				zend_is_auto_global(ZEND_STRL("_REQUEST") TSRMLS_CC);
			}
			(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_REQUEST"), (void **)&carrier);
			break;
		default:
			break;
	}
#endif

	if (!carrier || !(*carrier)) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	if (!len) {
		Z_ADDREF_P(*carrier);
		return *carrier;
	}

	if (zend_hash_find(Z_ARRVAL_PP(carrier), name, len + 1, (void **)&ret) == FAILURE) {
		zval *empty;
		MAKE_STD_ZVAL(empty);
		ZVAL_NULL(empty);
		return empty;
	}

	Z_ADDREF_P(*ret);
	return *ret;
}
/* }}} */

/** {{{inline fnx_request_t * fnx_request_get_method(fnx_request_t *request TSRMLS_DC)
*/
inline fnx_request_t * fnx_request_get_method(fnx_request_t *request TSRMLS_DC) {
	fnx_request_t *method = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_METHOD), 1 TSRMLS_CC);
	return method;
}
/* }}} */

/** {{{ inline fnx_request_t * fnx_request_get_language(fnx_request_t *instance TSRMLS_DC)
*/
inline zval * fnx_request_get_language(fnx_request_t *instance TSRMLS_DC) {
	zval *lang;

	lang = zend_read_property(fnx_request_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_LANG), 1 TSRMLS_CC);

	if (IS_STRING != Z_TYPE_P(lang)) {
		zval * accept_langs = fnx_request_query(FNX_GLOBAL_VARS_SERVER, ZEND_STRL("HTTP_ACCEPT_LANGUAGE") TSRMLS_CC);

		if (IS_STRING != Z_TYPE_P(accept_langs) || !Z_STRLEN_P(accept_langs)) {
			return lang;
		} else {
			char  	*ptrptr, *seg;
			uint	prefer_len = 0;
			double	max_qvlaue = 0;
			char 	*prefer = NULL;
			char  	*langs = estrndup(Z_STRVAL_P(accept_langs), Z_STRLEN_P(accept_langs));

			seg = php_strtok_r(langs, ",", &ptrptr);
			while(seg) {
				char *qvalue;
				while( *(seg) == ' ') seg++ ;
				/* Accept-Language: da, en-gb;q=0.8, en;q=0.7 */
				if ((qvalue = strstr(seg, "q="))) {
					float qval = (float)zend_string_to_double(qvalue + 2, seg - qvalue + 2);
					if (qval > max_qvlaue) {
						max_qvlaue = qval;
						if (prefer) {
							efree(prefer);
						}
						prefer_len = qvalue - seg - 1;
						prefer 	   = estrndup(seg, prefer_len);
					}
				} else {
					if (max_qvlaue < 1) {
						max_qvlaue = 1;
						prefer_len = strlen(seg);
						prefer 	   = estrndup(seg, prefer_len);
					}
				}

				seg = php_strtok_r(NULL, ",", &ptrptr);
			}

			if (prefer) {
				zval *accept_language;
				MAKE_STD_ZVAL(accept_language);
				ZVAL_STRINGL(accept_language,  prefer, prefer_len, 1);
				zend_update_property(fnx_request_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_LANG), accept_language TSRMLS_CC);
				efree(prefer);
				efree(langs);
				return accept_language;
			}
			efree(langs);
		}
	}
	return lang;
}
/* }}} */

/** {{{ inline int fnx_request_is_routed(fnx_request_t *request TSRMLS_DC)
*/
inline int fnx_request_is_routed(fnx_request_t *request TSRMLS_DC) {
	fnx_request_t *routed = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ROUTED), 1 TSRMLS_CC);
	return Z_LVAL_P(routed);
}
/* }}} */

/** {{{ inline int fnx_request_is_dispatched(fnx_request_t *request TSRMLS_DC)
*/
inline int fnx_request_is_dispatched(fnx_request_t *request TSRMLS_DC) {
	zval *dispatched = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_STATE), 1 TSRMLS_CC);
	return Z_LVAL_P(dispatched);
}
/* }}} */

/** {{{ inline int fnx_request_set_dispatched(fnx_request_t *instance, int flag TSRMLS_DC)
*/
inline int fnx_request_set_dispatched(fnx_request_t *instance, int flag TSRMLS_DC) {
	zend_update_property_bool(fnx_request_ce, instance, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_STATE), flag TSRMLS_CC);
	return 1;
}
/* }}} */

/** {{{ inline int fnx_request_set_routed(fnx_request_t *request, int flag TSRMLS_DC)
*/
inline int fnx_request_set_routed(fnx_request_t *request, int flag TSRMLS_DC) {
	zend_update_property_bool(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ROUTED), flag TSRMLS_CC);
	return 1;
}
/* }}} */

/** {{{ inline int fnx_request_set_params_single(fnx_request_t *request, char *key, int len, zval *value TSRMLS_DC)
*/
inline int fnx_request_set_params_single(fnx_request_t *request, char *key, int len, zval *value TSRMLS_DC) {
	zval *params = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), 1 TSRMLS_CC);

	if (zend_hash_update(Z_ARRVAL_P(params), key, len+1, &value, sizeof(zval *), NULL) == SUCCESS) {
		Z_ADDREF_P(value);
		return 1;
	}

	return 0;
}
/* }}} */

/** {{{ inline int fnx_request_set_params_multi(fnx_request_t *request, zval *values TSRMLS_DC)
*/
inline int fnx_request_set_params_multi(fnx_request_t *request, zval *values TSRMLS_DC) {
	zval *params = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), 1 TSRMLS_CC);
	if (values && Z_TYPE_P(values) == IS_ARRAY) {
		zend_hash_copy(Z_ARRVAL_P(params), Z_ARRVAL_P(values), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
		return 1;
	}
	return 0;
}
/* }}} */

/** {{{ inline zval * fnx_request_get_param(fnx_request_t *request, char *key, int len TSRMLS_DC)
*/
inline zval * fnx_request_get_param(fnx_request_t *request, char *key, int len TSRMLS_DC) {
	zval **ppzval;
	zval *params = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), 1 TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(params), key, len + 1, (void **) &ppzval) == SUCCESS) {
		return *ppzval;
	}
	return NULL;
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isGet(void)
*/
FNX_REQUEST_IS_METHOD(Get);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isPost(void)
*/
FNX_REQUEST_IS_METHOD(Post);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isPut(void)
*/
FNX_REQUEST_IS_METHOD(Put);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isHead(void)
*/
FNX_REQUEST_IS_METHOD(Head);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isOptions(void)
*/
FNX_REQUEST_IS_METHOD(Options);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isCli(void)
*/
FNX_REQUEST_IS_METHOD(Cli);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isAjax(void)
*/
PHP_METHOD(fnx_request, isAjax) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getEnv(mixed $name, mixed $default = NULL)
*/
FNX_REQUEST_METHOD(fnx_request, Env, 	FNX_GLOBAL_VARS_ENV);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getServer(mixed $name, mixed $default = NULL)
*/
FNX_REQUEST_METHOD(fnx_request, Server, FNX_GLOBAL_VARS_SERVER);
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getModuleName(void)
*/
PHP_METHOD(fnx_request, getModuleName) {
	zval *module = zend_read_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), 1 TSRMLS_CC);
	RETVAL_ZVAL(module, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getControllerName(void)
*/
PHP_METHOD(fnx_request, getControllerName) {
	zval *controller = zend_read_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), 1 TSRMLS_CC);
	RETVAL_ZVAL(controller, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getActionName(void)
*/
PHP_METHOD(fnx_request, getActionName) {
	zval *action = zend_read_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), 1 TSRMLS_CC);
	RETVAL_ZVAL(action, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setModuleName(string $module)
*/
PHP_METHOD(fnx_request, setModuleName) {
	zval *module;
	fnx_request_t *self	= getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &module) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(module) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expect a string module name");
		RETURN_FALSE;
	}

	zend_update_property(fnx_request_ce, self, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), module TSRMLS_CC);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setControllerName(string $controller)
*/
PHP_METHOD(fnx_request, setControllerName) {
	zval *controller;
	fnx_request_t *self	= getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &controller) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(controller) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expect a string controller name");
		RETURN_FALSE;
	}

	zend_update_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), controller TSRMLS_CC);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setActionName(string $action)
*/
PHP_METHOD(fnx_request, setActionName) {
	zval *action;
	zval *self	 = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &action) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(action) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expect a string action name");
		RETURN_FALSE;
	}

	zend_update_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), action TSRMLS_CC);

	RETURN_ZVAL(self, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setParam(mixed $value)
*/
PHP_METHOD(fnx_request, setParam) {
	uint argc;
	fnx_request_t *self	= getThis();

	argc = ZEND_NUM_ARGS();

	if (1 == argc) {
		zval *value ;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
			return;
		}
		if (value && Z_TYPE_P(value) == IS_ARRAY) {
			if (fnx_request_set_params_multi(self, value TSRMLS_CC)) {
				RETURN_ZVAL(self, 1, 0);
			}
		}
	} else if (2 == argc) {
		zval *value;
		char *name;
		uint len;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
			return;
		}

		if (fnx_request_set_params_single(getThis(), name, len, value TSRMLS_CC)) {
			RETURN_ZVAL(self, 1, 0);
		}
	} else {
		WRONG_PARAM_COUNT;
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getParam(string $name, $mixed $default = NULL)
*/
PHP_METHOD(fnx_request, getParam) {
	char *name;
	uint len;
	zval *def = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &name, &len, &def) == FAILURE) {
		return;
	} else {
		zval *value = fnx_request_get_param(getThis(), name, len TSRMLS_CC);
		if (value) {
			RETURN_ZVAL(value, 1, 0);
		}
		if (def) {
			RETURN_ZVAL(def, 1, 0);
		}
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getException(void)
*/
PHP_METHOD(fnx_request, getException) {
	zval *exception = zend_read_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_EXCEPTION), 1 TSRMLS_CC);
	if (IS_OBJECT == Z_TYPE_P(exception)
			&& instanceof_function(Z_OBJCE_P(exception),
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
				zend_exception_get_default()
#else
				zend_exception_get_default(TSRMLS_C)
#endif
				TSRMLS_CC)) {
		RETURN_ZVAL(exception, 1, 0);
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getParams(void)
*/
PHP_METHOD(fnx_request, getParams) {
	zval *params = zend_read_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), 1 TSRMLS_CC);
	RETURN_ZVAL(params, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getLanguage(void)
*/
PHP_METHOD(fnx_request, getLanguage) {
	zval *lang = fnx_request_get_language(getThis() TSRMLS_CC);
	RETURN_ZVAL(lang, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getMethod(void)
*/
PHP_METHOD(fnx_request, getMethod) {
	zval *method = fnx_request_get_method(getThis() TSRMLS_CC);
	RETURN_ZVAL(method, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isDispatched(void)
*/
PHP_METHOD(fnx_request, isDispatched) {
	RETURN_BOOL(fnx_request_is_dispatched(getThis() TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setDispatched(void)
*/
PHP_METHOD(fnx_request, setDispatched) {
	RETURN_BOOL(fnx_request_set_dispatched(getThis(), 1 TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setBaseUri(string $name)
*/
PHP_METHOD(fnx_request, setBaseUri) {
	zval *uri;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &uri) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(uri) !=  IS_STRING || !Z_STRLEN_P(uri)) {
		RETURN_FALSE;
	}

	if (fnx_request_set_base_uri(getThis(), Z_STRVAL_P(uri), NULL TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getBaseUri(string $name)
*/
PHP_METHOD(fnx_request, getBaseUri) {
	zval *uri = zend_read_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), 1 TSRMLS_CC);
	RETURN_ZVAL(uri, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::getRequestUri(string $name)
*/
PHP_METHOD(fnx_request, getRequestUri) {
	zval *uri = zend_read_property(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI), 1 TSRMLS_CC);
	RETURN_ZVAL(uri, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setRequestUri(string $name)
*/
PHP_METHOD(fnx_request, setRequestUri) {
	char *uri;
	uint len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &uri, &len) == FAILURE) {
		return;
	}

	zend_update_property_stringl(fnx_request_ce, getThis(), ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI), uri, len TSRMLS_CC);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::isRouted(void)
*/
PHP_METHOD(fnx_request, isRouted) {
	RETURN_BOOL(fnx_request_is_routed(getThis() TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Fnx_Request_Abstract::setRouted(void)
*/
PHP_METHOD(fnx_request, setRouted) {
	if (fnx_request_set_routed(getThis(),  1 TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ fnx_request_methods
*/
zend_function_entry fnx_request_methods[] = {
	PHP_ME(fnx_request, isGet, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isPost,	fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isPut, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isHead, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isOptions, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isCli, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isAjax, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getServer, fnx_request_getserver_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getEnv, fnx_request_getenv_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setParam, fnx_request_set_param_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getParam, fnx_request_get_param_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getParams, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getException, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getModuleName, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getControllerName, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getActionName, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setModuleName, fnx_request_set_module_name_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setControllerName, fnx_request_set_controller_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setActionName, fnx_request_set_action_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getMethod, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getLanguage, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setBaseUri, fnx_request_set_baseuir_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getBaseUri,	fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, getRequestUri, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setRequestUri, fnx_request_set_request_uri_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isDispatched, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setDispatched, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, isRouted, fnx_request_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_request, setRouted, fnx_request_set_routed_arginfo, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(request){
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Request", "Fnx\\Request", fnx_request_methods);
	fnx_request_ce 			= zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_request_ce->ce_flags = ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	zend_declare_property_null(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE),     ZEND_ACC_PUBLIC	TSRMLS_CC);
	zend_declare_property_null(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION),     ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_METHOD),     ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS),  	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_LANG), 		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_EXCEPTION),  ZEND_ACC_PROTECTED TSRMLS_CC);

	zend_declare_property_string(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_BASE), "", ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_string(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_URI),  "", ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_STATE),	0,	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(fnx_request_ce, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ROUTED), 0, 	ZEND_ACC_PROTECTED TSRMLS_CC);

	FNX_STARTUP(request_http);
	FNX_STARTUP(request_simple);

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
