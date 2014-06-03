// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_response.c 324384 2012-03-20 11:50:17Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_response.h"
#include "fnx_exception.h"

zend_class_entry *fnx_response_ce;

#include "response/http.c"
#include "response/cli.c"

/** {{{ fnx_response_t * fnx_response_instance(fnx_response_t *this_ptr, char *sapi_name TSRMLS_DC)
 */
fnx_response_t * fnx_response_instance(fnx_response_t *this_ptr, char *sapi_name TSRMLS_DC) {
	zval 		  		*header;
	zend_class_entry 	*ce;
	fnx_response_t 		*instance;

	if (strncasecmp(sapi_name, "cli", 3)) {
		ce = fnx_response_http_ce;
	} else {
		ce = fnx_response_cli_ce;
	}

	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, ce);
	}

	MAKE_STD_ZVAL(header);
	array_init(header);
	zend_update_property(ce, instance, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_HEADER), header TSRMLS_CC);
	zval_ptr_dtor(&header);

	zend_update_property_string(ce, instance, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), "" TSRMLS_CC);

	return instance;
}
/* }}} */

/** {{{ static int fnx_response_set_body(fnx_response_t *response, char *name, int name_len, char *body, long body_len TSRMLS_DC)
 */
#if 0
static int fnx_response_set_body(fnx_response_t *response, char *name, int name_len, char *body, long body_len TSRMLS_DC) {
	zval *zbody;
	zend_class_entry *response_ce;

	if (!body_len) {
		return 1;
	}

	response_ce = Z_OBJCE_P(response);

	zbody = zend_read_property(response_ce, response, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), 1 TSRMLS_CC);

	zval_dtor(zbody);
	efree(zbody);

	MAKE_STD_ZVAL(zbody);
	ZVAL_STRINGL(zbody, body, body_len, 1);

	zend_update_property(response_ce, response, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), zbody TSRMLS_CC);

	return 1;
}
#endif
/* }}} */

/** {{{ int fnx_response_alter_body(fnx_response_t *response, char *name, int name_len, char *body, long body_len, int flag TSRMLS_DC)
 */
int fnx_response_alter_body(fnx_response_t *response, char *name, int name_len, char *body, long body_len, int flag TSRMLS_DC) {
	zval *zbody;
	char *obody;

	if (!body_len) {
		return 1;
	}

	zbody = zend_read_property(fnx_response_ce, response, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), 1 TSRMLS_CC);
	obody = Z_STRVAL_P(zbody);

	switch (flag) {
		case FNX_RESPONSE_PREPEND:
			Z_STRLEN_P(zbody) = spprintf(&Z_STRVAL_P(zbody), 0, "%s%s", body, obody);
			break;
		case FNX_RESPONSE_APPEND:
			Z_STRLEN_P(zbody) = spprintf(&Z_STRVAL_P(zbody), 0, "%s%s", obody, body);
			break;
		case FNX_RESPONSE_REPLACE:
		default:
			ZVAL_STRINGL(zbody, body, body_len, 1);
			break;
	}

	efree(obody);

	return 1;
}
/* }}} */

/** {{{ int fnx_response_clear_body(fnx_response_t *response TSRMLS_DC)
 */
int fnx_response_clear_body(fnx_response_t *response TSRMLS_DC) {
	zend_update_property_string(fnx_response_ce, response, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), "" TSRMLS_CC);
	return 1;
}
/* }}} */

/** {{{ int fnx_response_set_redirect(fnx_response_t *response, char *url, int len TSRMLS_DC)
 */
int fnx_response_set_redirect(fnx_response_t *response, char *url, int len TSRMLS_DC) {
	sapi_header_line ctr = {0};

	ctr.line_len 		= spprintf(&(ctr.line), 0, "%s %s", "Location:", url);
	ctr.response_code 	= 0;
	if (sapi_header_op(SAPI_HEADER_REPLACE, &ctr TSRMLS_CC) == SUCCESS) {
		efree(ctr.line);
		return 1;
	}
	efree(ctr.line);
	return 0;
}
/* }}} */

/** {{{ int fnx_response_send(fnx_response_t *response TSRMLS_DC)
 */
int fnx_response_send(fnx_response_t *response TSRMLS_DC) {
	zval *body = zend_read_property(fnx_response_ce, response, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), 1 TSRMLS_CC);

	if (IS_STRING == Z_TYPE_P(body) && Z_STRLEN_P(body) > 0) {
		if (php_write(Z_STRVAL_P(body), Z_STRLEN_P(body) TSRMLS_CC) == FAILURE) {
			return 0;
		}
	}
	return 1;
}
/* }}} */

/** {{{ zval * fnx_response_get_body(fnx_response_t *response TSRMLS_DC)
 */
zval * fnx_response_get_body(fnx_response_t *response TSRMLS_DC) {
	zval *body = zend_read_property(fnx_response_ce, response, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), 1 TSRMLS_CC);
	return body;
}
/* }}} */

/** {{{ proto private Fnx_Response_Abstract::__construct()
*/
PHP_METHOD(fnx_response, __construct) {
	(void)fnx_response_instance(getThis(), sapi_module.name TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::__desctruct(void)
*/
PHP_METHOD(fnx_response, __destruct) {
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::appenBody($body, $name = NULL)
*/
PHP_METHOD(fnx_response, appendBody) {
	char		  	*body, *name = NULL;
	uint			body_len, name_len = 0;
	fnx_response_t 	*self;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &body, &body_len, &name, &name_len) == FAILURE) {
		return;
	}

	self = getThis();

	if (fnx_response_alter_body(self, name, name_len, body, body_len, FNX_RESPONSE_APPEND TSRMLS_CC)) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::prependBody($body, $name = NULL)
*/
PHP_METHOD(fnx_response, prependBody) {
	char		  	*body, *name = NULL;
	uint			body_len, name_len = 0;
	fnx_response_t 	*self;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &body, &body_len, &name, &name_len) == FAILURE) {
		return;
	}

	self = getThis();

	if (fnx_response_alter_body(self, name, name_len, body, body_len, FNX_RESPONSE_PREPEND TSRMLS_CC)) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::setHeader($name, $value, $replace = 0)
*/
PHP_METHOD(fnx_response, setHeader) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto parotected Fnx_Response_Abstract::setAllHeaders(void)
*/
PHP_METHOD(fnx_response, setAllHeaders) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::getHeader(void)
*/
PHP_METHOD(fnx_response, getHeader) {
	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::clearHeaders(void)
*/
PHP_METHOD(fnx_response, clearHeaders) {
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::setRedirect(string $url)
*/
PHP_METHOD(fnx_response, setRedirect) {
	char *url;
	uint  url_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &url, &url_len) == FAILURE) {
		return;
	}

	if (!url_len) {
		RETURN_FALSE;
	}

	RETURN_BOOL(fnx_response_set_redirect(getThis(), url, url_len TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::setBody($body, $name = NULL)
*/
PHP_METHOD(fnx_response, setBody) {
	char		  	*body, *name = NULL;
	uint			body_len, name_len = 0;
	fnx_response_t 	*self;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &body, &body_len, &name, &name_len) == FAILURE) {
		return;
	}

	self = getThis();

	if (fnx_response_alter_body(self, name, name_len, body, body_len, FNX_RESPONSE_REPLACE TSRMLS_CC)) {
		RETURN_ZVAL(self, 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::clearBody(void)
*/
PHP_METHOD(fnx_response, clearBody) {
	if (fnx_response_clear_body(getThis() TSRMLS_CC)) {
		RETURN_ZVAL(getThis(), 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::getBody(void)
 */
PHP_METHOD(fnx_response, getBody) {
	zval *body;

	body = fnx_response_get_body(getThis() TSRMLS_CC);

	RETURN_ZVAL(body, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::response(void)
 */
PHP_METHOD(fnx_response, response) {
	RETURN_BOOL(fnx_response_send(getThis() TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::__toString(void)
 */
PHP_METHOD(fnx_response, __toString) {
	zval *body = zend_read_property(fnx_response_ce, getThis(), ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), 1 TSRMLS_CC);
	RETURN_ZVAL(body, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Response_Abstract::__clone(void)
*/
PHP_METHOD(fnx_response, __clone) {
}
/* }}} */

/** {{{ fnx_response_methods
*/
zend_function_entry fnx_response_methods[] = {
	PHP_ME(fnx_response, __construct, 	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(fnx_response, __destruct,  	NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(fnx_response, __clone,		NULL, ZEND_ACC_PRIVATE)
	PHP_ME(fnx_response, __toString,		NULL, ZEND_ACC_PRIVATE)
	PHP_ME(fnx_response, setBody,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, appendBody,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, prependBody,	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, clearBody,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, getBody,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, setHeader,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, setAllHeaders,	NULL, ZEND_ACC_PROTECTED)
	PHP_ME(fnx_response, getHeader,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, clearHeaders,   NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, setRedirect,	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_response, response,		NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(response) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Response", "Fnx\\Response", fnx_response_methods);

	fnx_response_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_response_ce->ce_flags |= ZEND_ACC_EXPLICIT_ABSTRACT_CLASS;

	zend_declare_property_null(fnx_response_ce, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_HEADER), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_response_ce, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_BODY), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(fnx_response_ce, ZEND_STRL(FNX_RESPONSE_PROPERTY_NAME_HEADEREXCEPTION), 0, ZEND_ACC_PROTECTED TSRMLS_CC);

	FNX_STARTUP(response_http);
	FNX_STARTUP(response_cli);

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
