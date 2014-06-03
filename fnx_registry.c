// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_registry.c 321289 2011-12-21 02:53:29Z nechtan $  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_registry.h"

zend_class_entry *fnx_registry_ce;

/* {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_registry_get_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_registry_has_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_registry_del_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_registry_set_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ fnx_registry_t *fnx_registry_instance(fnx_registry_t *this_ptr TSRMLS_DC)
*/
fnx_registry_t *fnx_registry_instance(fnx_registry_t *this_ptr TSRMLS_DC) {
	fnx_registry_t *instance = zend_read_static_property(fnx_registry_ce, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_INSTANCE), 1 TSRMLS_CC);

	if (Z_TYPE_P(instance) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(instance), fnx_registry_ce TSRMLS_CC)) {
		zval *regs;

		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_registry_ce);

		MAKE_STD_ZVAL(regs);
		array_init(regs);
		zend_update_property(fnx_registry_ce, instance, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_ENTRYS), regs TSRMLS_CC);
		zend_update_static_property(fnx_registry_ce, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_INSTANCE), instance TSRMLS_CC);
		zval_ptr_dtor(&regs);
		zval_ptr_dtor(&instance);
	}

	return instance;
}
/* }}} */

/** {{{ int fnx_registry_is_set(char *name, int len TSRMLS_DC)
 */
int fnx_registry_is_set(char *name, int len TSRMLS_DC) {
	fnx_registry_t 	*registry;
	zval 			*entrys;

	registry = fnx_registry_instance(NULL TSRMLS_CC);
	entrys	 = zend_read_property(fnx_registry_ce, registry, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_ENTRYS), 1 TSRMLS_CC);
	return zend_hash_exists(Z_ARRVAL_P(entrys), name, len + 1);
}
/* }}} */

/** {{{ proto private Fnx_Registry::__construct(void)
*/
PHP_METHOD(fnx_registry, __construct) {
}
/* }}} */

/** {{{ proto private Fnx_Registry::__clone(void)
*/
PHP_METHOD(fnx_registry, __clone) {
}
/* }}} */

/** {{{ proto public static Fnx_Registry::get($name)
*/
PHP_METHOD(fnx_registry, get) {
	char *name;
	uint  len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		return;
	} else {
		zval **ppzval, *entrys;
		fnx_registry_t 	*registry;

		registry = fnx_registry_instance(NULL TSRMLS_CC);
		entrys	 = zend_read_property(fnx_registry_ce, registry, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_ENTRYS), 1 TSRMLS_CC);

		if (entrys && Z_TYPE_P(entrys) == IS_ARRAY) {
			if (zend_hash_find(Z_ARRVAL_P(entrys), name, len + 1, (void **) &ppzval) == SUCCESS) {
				RETURN_ZVAL(*ppzval, 1, 0);
			}
		}
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public staitc Fnx_Registry::set($name, $value)
*/
PHP_METHOD(fnx_registry, set) {
	zval *value;
	char *name;
	uint len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
		return;
	} else {
		fnx_registry_t	*registry;
		zval			*entrys;

		registry = fnx_registry_instance(NULL TSRMLS_CC);
		entrys 	 = zend_read_property(fnx_registry_ce, registry, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_ENTRYS), 1 TSRMLS_CC);

		Z_ADDREF_P(value);
		if (zend_hash_update(Z_ARRVAL_P(entrys), name, len + 1, &value, sizeof(zval *), NULL) == SUCCESS) {
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public staitc Fnx_Registry::del($name)
*/
PHP_METHOD(fnx_registry, del) {
	char *name;
	uint len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		return;
	} else {
		fnx_registry_t	*registry;
		zval *entrys;

		registry = fnx_registry_instance(NULL TSRMLS_CC);
		entrys 	 = zend_read_property(fnx_registry_ce, registry, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_ENTRYS), 1 TSRMLS_CC);

		zend_hash_del(Z_ARRVAL_P(entrys), name, len + 1);
	}

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Registry::has($name)
*/
PHP_METHOD(fnx_registry, has) {
	char *name;
	uint len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		return;
	} else {
		RETURN_BOOL(fnx_registry_is_set(name, len TSRMLS_CC));
	}
}
/* }}} */

/** {{{ proto public Fnx_Registry::getInstance(void)
*/
PHP_METHOD(fnx_registry, getInstance) {
	fnx_registry_t *registry = fnx_registry_instance(NULL TSRMLS_CC);
	RETURN_ZVAL(registry, 1, 0);
}
/* }}} */

/** {{{ fnx_registry_methods
*/
zend_function_entry fnx_registry_methods[] = {
	PHP_ME(fnx_registry, __construct, 	NULL, ZEND_ACC_CTOR|ZEND_ACC_PRIVATE)
	PHP_ME(fnx_registry, __clone, 		NULL, ZEND_ACC_CLONE|ZEND_ACC_PRIVATE)
	PHP_ME(fnx_registry, get, fnx_registry_get_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(fnx_registry, has, fnx_registry_has_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(fnx_registry, set, fnx_registry_set_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(fnx_registry, del, fnx_registry_del_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(registry) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Vars", "Fnx\\Vars", fnx_registry_methods);

	fnx_registry_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_registry_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(fnx_registry_ce, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_INSTANCE), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(fnx_registry_ce, ZEND_STRL(FNX_REGISTRY_PROPERTY_NAME_ENTRYS),  ZEND_ACC_PROTECTED TSRMLS_CC);

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
