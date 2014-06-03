// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_controller.c 325274 2012-04-18 08:12:55Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_API.h"
#include "Zend/zend_interfaces.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_request.h"
#include "fnx_response.h"
#include "fnx_dispatcher.h"
#include "fnx_view.h"
#include "fnx_exception.h"
#include "fnx_action.h"
#include "fnx_controller.h"

zend_class_entry * fnx_controller_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_controller_void_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_controller_initview_arginfo, 0, 0, 0)
    ZEND_ARG_ARRAY_INFO(0, options, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_controller_getiarg_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_controller_setvdir_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, view_directory)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_controller_forward_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, module)
    ZEND_ARG_INFO(0, controller)
    ZEND_ARG_INFO(0, action)
    ZEND_ARG_ARRAY_INFO(0, paramters, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_controller_redirect_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, url)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_controller_render_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, tpl)
    ZEND_ARG_ARRAY_INFO(0, parameters, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_controller_display_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, tpl)
    ZEND_ARG_ARRAY_INFO(0, parameters, 1)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ zval * fnx_controller_render(fnx_controller_t *instance, char *action_name, int len, zval *var_array TSRMLS_DC)
 */
static zval * fnx_controller_render(fnx_controller_t *instance, char *action_name, int len, zval *var_array TSRMLS_DC) {
	char 	*path, *view_ext, *self_name, *tmp;
	zval 	*name, *param, *ret = NULL;
	int 	path_len;
	fnx_view_t *view;

	view   	  = zend_read_property(fnx_controller_ce, instance, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_VIEW), 1 TSRMLS_CC);
	name	  = zend_read_property(fnx_controller_ce, instance, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_NAME), 1 TSRMLS_CC);
	view_ext  = FNX_G(view_ext);

	self_name = zend_str_tolower_dup(Z_STRVAL_P(name), Z_STRLEN_P(name));

	tmp = self_name;
 	while (*tmp != '\0') {
		if (*tmp == '_') {
			*tmp = DEFAULT_SLASH;
		}
		tmp++;
	}

	action_name = estrndup(action_name, len);

	tmp = action_name;
 	while (*tmp != '\0') {
		if (*tmp == '_') {
			*tmp = DEFAULT_SLASH;
		}
		tmp++;
	}

	path_len  = spprintf(&path, 0, "%s%c%s.%s", self_name, DEFAULT_SLASH, action_name, view_ext);

	efree(self_name);
	efree(action_name);

	MAKE_STD_ZVAL(param);
	ZVAL_STRING(param, path, 0);

	if (var_array) {
		zend_call_method_with_2_params(&view, Z_OBJCE_P(view), NULL, "render", &ret, param, var_array);
	} else {
		zend_call_method_with_1_params(&view, Z_OBJCE_P(view), NULL, "render", &ret, param);
	}

	zval_dtor(param);
	efree(param);

	if (!ret || (Z_TYPE_P(ret) == IS_BOOL && !Z_BVAL_P(ret))) {
		return NULL;
	}

	return ret;
}
/* }}} */

/** {{{ static int fnx_controller_display(zend_class_entry *ce, fnx_controller_t *instance, char *action_name, int len, zval *var_array TSRMLS_DC)
 */
static int fnx_controller_display(zend_class_entry *ce, fnx_controller_t *instance, char *action_name, int len, zval *var_array TSRMLS_DC) {
	char *path, *view_ext, *self_name, *tmp;
	zval *name, *param, *ret = NULL;
	int  path_len;

	fnx_view_t	*view;

	view   	  = zend_read_property(ce, instance, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_VIEW), 1 TSRMLS_CC);
	name	  = zend_read_property(ce, instance, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_NAME), 1 TSRMLS_CC);
	view_ext  = FNX_G(view_ext);

	self_name = zend_str_tolower_dup(Z_STRVAL_P(name), Z_STRLEN_P(name));

	tmp = self_name;
 	while (*tmp != '\0') {
		if (*tmp == '_') {
			*tmp = DEFAULT_SLASH;
		}
		tmp++;
	}

	action_name = estrndup(action_name, len);

	tmp = action_name;
 	while (*tmp != '\0') {
		if (*tmp == '_') {
			*tmp = DEFAULT_SLASH;
		}
		tmp++;
	}

	path_len  = spprintf(&path, 0, "%s%c%s.%s", self_name, DEFAULT_SLASH, action_name, view_ext);

	efree(self_name);
	efree(action_name);

	MAKE_STD_ZVAL(param);
	ZVAL_STRING(param, path, 0);

	if (var_array) {
		zend_call_method_with_2_params(&view, Z_OBJCE_P(view), NULL, "display", &ret, param, var_array);
	} else {
		zend_call_method_with_1_params(&view, Z_OBJCE_P(view), NULL, "display", &ret, param);
	}

	zval_dtor(param);
	efree(param);

	if (!ret) {
		return 0;
	}

	if ((Z_TYPE_P(ret) == IS_BOOL && !Z_BVAL_P(ret))) {
		zval_ptr_dtor(&ret);
		return 0;
	}

	return 1;
}
/* }}} */

/** {{{ int fnx_controller_construct(zend_class_entry *ce, fnx_controller_t *self, fnx_request_t *request, fnx_response_t *responseew_t *view, zval *args TSRMLS_DC)
 */
int fnx_controller_construct(zend_class_entry *ce, fnx_controller_t *self, fnx_request_t *request, fnx_response_t *response, fnx_view_t *view, zval *args TSRMLS_DC) {
	zval *module;

	if (args) {
		zend_update_property(ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_ARGS), args TSRMLS_CC);
	}

	module = zend_read_property(fnx_request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), 1 TSRMLS_CC);

	zend_update_property(ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_REQUEST), request TSRMLS_CC);
	zend_update_property(ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_RESPONSE), response TSRMLS_CC);
	zend_update_property(ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_MODULE), module TSRMLS_CC);
	zend_update_property(ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_VIEW), view TSRMLS_CC);

	if (!instanceof_function(ce, fnx_action_ce TSRMLS_CC)
			&& zend_hash_exists(&(ce->function_table), ZEND_STRS("main"))) {
		zend_call_method_with_0_params(&self, ce, NULL, "main", NULL);
	}

	return 1;
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract:: main()
*/
PHP_METHOD(fnx_controller, main) {
}

/* }}} */

/** {{{ proto protected Fnx_Controller_Abstract::__construct(Fnx_Request_Abstract $request, Fnx_Response_abstrct $response, Fnx_View_Interface $view, array $invokeArgs = NULL)
*/
PHP_METHOD(fnx_controller, __construct) {
	fnx_request_t 	*request;
	fnx_response_t	*response;
	fnx_view_t		*view;
	zval 			*invoke_arg = NULL;
	fnx_controller_t *self = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ooo|z",
				&request, fnx_request_ce, &response, fnx_response_ce, &view, fnx_view_interface_ce, &invoke_arg) == FAILURE) {
		return;
	} else	{
		if(!fnx_controller_construct(fnx_controller_ce, self, request, response, view, invoke_arg TSRMLS_CC)) {
			RETURN_FALSE;
		}
	}
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::getView(void)
*/
PHP_METHOD(fnx_controller, getView) {
	fnx_view_t *view = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_VIEW), 1 TSRMLS_CC);
	RETURN_ZVAL(view, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::getRequest(void)
*/
PHP_METHOD(fnx_controller, getReq) {
	fnx_request_t *request = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_REQUEST), 1 TSRMLS_CC);
	RETURN_ZVAL(request, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::getResponse(void)
*/
PHP_METHOD(fnx_controller, getResponse) {
	fnx_view_t *response = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_RESPONSE), 1 TSRMLS_CC);
	RETURN_ZVAL(response, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::initView(array $options = NULL)
*/
PHP_METHOD(fnx_controller, initView) {
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::getInvokeArg(string $name)
 */
PHP_METHOD(fnx_controller, getInvokeArg) {
	char *name;
	uint len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",  &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	if (len) {
		zval **ppzval, *args;
		args = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_ARGS), 1 TSRMLS_CC);

		if (ZVAL_IS_NULL(args)) {
			RETURN_NULL();
		}

		if (zend_hash_find(Z_ARRVAL_P(args), name, len + 1, (void **)&ppzval) == SUCCESS) {
			RETURN_ZVAL(*ppzval, 1, 0);
		}
	}
	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::getInvokeArgs(void)
 */
PHP_METHOD(fnx_controller, getInvokeArgs) {
	zval *args = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_ARGS), 1 TSRMLS_CC);
	RETURN_ZVAL(args, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::getModuleName(void)
 */
PHP_METHOD(fnx_controller, getModuleName) {
	zval *module = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_MODULE), 1 TSRMLS_CC);
	RETURN_ZVAL(module, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::setViewpath(string $view_directory)
*/
PHP_METHOD(fnx_controller, setViewpath) {
	zval 		*path;
	fnx_view_t 	*view;
	zend_class_entry *view_ce;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &path) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(path) != IS_STRING) {
		RETURN_FALSE;
	}

	view = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_VIEW), 1 TSRMLS_CC);
	if ((view_ce = Z_OBJCE_P(view)) == fnx_view_simple_ce) {
		zend_update_property(view_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), path TSRMLS_CC);
	} else {
		zend_call_method_with_1_params(&view, view_ce, NULL, "setscriptpath", NULL, path);
	}

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::getViewpath(void)
*/
PHP_METHOD(fnx_controller, getViewpath) {
	zend_class_entry *view_ce;
	zval *view = zend_read_property(fnx_controller_ce, getThis(), ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_VIEW), 1 TSRMLS_CC);
	if ((view_ce = Z_OBJCE_P(view)) == fnx_view_simple_ce) {
		zval *tpl_dir = zend_read_property(view_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), 1 TSRMLS_CC);
		RETURN_ZVAL(tpl_dir, 1, 0);
	} else {
		zval *ret;
		zend_call_method_with_0_params(&view, view_ce, NULL, "getscriptpath", &ret);
		RETURN_ZVAL(ret, 1, 1);
	}
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::forward($module, $controller, $action, $args = NULL)
*/
PHP_METHOD(fnx_controller, forward) {
	zval *controller, *module, *action, *args, *parameters;
	fnx_request_t *request;
	zend_class_entry *request_ce;

	fnx_controller_t *self = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|zzz", &module, &controller, &action, &args) == FAILURE) {
		return;
	}

	request    = zend_read_property(fnx_controller_ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_REQUEST), 1 TSRMLS_CC);
	parameters = zend_read_property(fnx_controller_ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_ARGS), 1 TSRMLS_CC);

	if (Z_TYPE_P(request) != IS_OBJECT
			|| !instanceof_function((request_ce = Z_OBJCE_P(request)), fnx_request_ce TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (ZVAL_IS_NULL(parameters)) {
		MAKE_STD_ZVAL(parameters);
		array_init(parameters);
	}

	switch (ZEND_NUM_ARGS()) {
		case 1:
			if (Z_TYPE_P(module) != IS_STRING) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Expect a string action name");
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), module TSRMLS_CC);
			break;
		case 2:
			if (Z_TYPE_P(controller) ==  IS_STRING) {
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), module TSRMLS_CC);
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), controller TSRMLS_CC);
			} else if (Z_TYPE_P(controller) == IS_ARRAY) {
				zend_hash_copy(Z_ARRVAL_P(parameters), Z_ARRVAL_P(controller), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), module TSRMLS_CC);
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), parameters TSRMLS_CC);
			} else {
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			break;
		case 3:
			if (Z_TYPE_P(action) == IS_STRING) {
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), module TSRMLS_CC);
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), controller TSRMLS_CC);
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), action TSRMLS_CC);
			} else if (Z_TYPE_P(action) == IS_ARRAY) {
				zend_hash_copy(Z_ARRVAL_P(parameters), Z_ARRVAL_P(action), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), module TSRMLS_CC);
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), controller TSRMLS_CC);
				zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), parameters TSRMLS_CC);
			} else {
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			break;
		case 4:
			if (Z_TYPE_P(args) != IS_ARRAY) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Parameters must be an array");
				zval_dtor(parameters);
				efree(parameters);
				RETURN_FALSE;
			}
			zend_hash_copy(Z_ARRVAL_P(parameters), Z_ARRVAL_P(args), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
			zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_MODULE), module TSRMLS_CC);
			zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_CONTROLLER), controller TSRMLS_CC);
			zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_ACTION), action TSRMLS_CC);
			zend_update_property(request_ce, request, ZEND_STRL(FNX_REQUEST_PROPERTY_NAME_PARAMS), parameters TSRMLS_CC);
			break;
	}

	(void)fnx_request_set_dispatched(request, 0 TSRMLS_CC);
	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Controller_Abstract::redirect(string $url)
*/
PHP_METHOD(fnx_controller, redirect) {
	char 			*location;
	uint 			location_len;
	fnx_response_t 	*response;
	fnx_controller_t *self = getThis();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &location, &location_len) == FAILURE) {
		return;
	}

	response = zend_read_property(fnx_controller_ce, self, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_RESPONSE), 1 TSRMLS_CC);

	(void)fnx_response_set_redirect(response, location, location_len TSRMLS_CC);

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto protected Fnx_Controller_Abstract::render(string $action, array $var_array = NULL)
*/
PHP_METHOD(fnx_controller, render) {
	char *action_name;
	uint action_name_len;
	zval *var_array	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &action_name, &action_name_len, &var_array) == FAILURE) {
		return;
	} else {
		zval *output = fnx_controller_render(getThis(), action_name, action_name_len, var_array TSRMLS_CC);
		if (output) {
			RETURN_ZVAL(output, 1, 0);
		} else {
			RETURN_FALSE;
		}
	}
}
/* }}} */

/** {{{ proto protected Fnx_Controller_Abstract::display(string $action, array $var_array = NULL)
*/
PHP_METHOD(fnx_controller, display) {
	char *action_name;
	uint action_name_len;
	zval *var_array	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &action_name, &action_name_len, &var_array) == FAILURE) {
		return;
	} else {
		RETURN_BOOL(fnx_controller_display(fnx_controller_ce, getThis(), action_name, action_name_len, var_array TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto private Fnx_Controller_Abstract::__clone()
*/
PHP_METHOD(fnx_controller, __clone) {
}
/* }}} */

/** {{{ fnx_controller_methods
*/
zend_function_entry fnx_controller_methods[] = {
	PHP_ME(fnx_controller, render,	    fnx_controller_render_arginfo, 	ZEND_ACC_PROTECTED|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, display,	    fnx_controller_display_arginfo, ZEND_ACC_PROTECTED|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, getReq,	fnx_controller_void_arginfo, 	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, getResponse,	fnx_controller_void_arginfo, 	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, getModuleName,fnx_controller_void_arginfo, 	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, getView,		fnx_controller_void_arginfo, 	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, initView,	fnx_controller_initview_arginfo,ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, setViewpath,	fnx_controller_setvdir_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, getViewpath,	fnx_controller_void_arginfo, 	ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, forward,	   	fnx_controller_forward_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, redirect,    fnx_controller_redirect_arginfo,ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, getInvokeArgs,fnx_controller_void_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, getInvokeArg, fnx_controller_getiarg_arginfo,ZEND_ACC_PUBLIC|ZEND_ACC_FINAL)
	PHP_ME(fnx_controller, __construct,	NULL, 							ZEND_ACC_CTOR|ZEND_ACC_FINAL|ZEND_ACC_PUBLIC)
	PHP_ME(fnx_controller, __clone, 	NULL, 							ZEND_ACC_PRIVATE|ZEND_ACC_FINAL)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(controller) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Controller", "Fnx\\Controller", fnx_controller_methods);
	fnx_controller_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_controller_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	zend_declare_property_null(fnx_controller_ce, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_ACTIONS),	ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(fnx_controller_ce, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_MODULE), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_controller_ce, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_NAME), 	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_controller_ce, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_REQUEST),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_controller_ce, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_RESPONSE),	ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_controller_ce, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_ARGS),		ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_controller_ce, ZEND_STRL(FNX_CONTROLLER_PROPERTY_NAME_VIEW),		ZEND_ACC_PROTECTED TSRMLS_CC);

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
