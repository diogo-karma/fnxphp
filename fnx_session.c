// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_session.c 321289 2011-12-21 02:53:29Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_objects.h"
#include "main/SAPI.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_session.h"
#include "fnx_exception.h"

zend_class_entry * fnx_session_ce;
#ifdef HAVE_SPL
extern PHPAPI zend_class_entry * spl_ce_Countable;
#endif

/* {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_session_void_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_session_get_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_session_has_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_session_del_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_session_set_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ inline int fnx_session_start(fnx_session_t *session TSRMLS_DC)
 */
inline int fnx_session_start(fnx_session_t *session TSRMLS_DC) {
	zval *status;

	status = zend_read_property(fnx_session_ce, session, ZEND_STRL(FNX_SESSION_PROPERTY_NAME_STATUS), 1 TSRMLS_CC);
	if (Z_BVAL_P(status)) {
		return 1;
	}

	php_session_start(TSRMLS_C);
	zend_update_property_bool(fnx_session_ce, session, ZEND_STRL(FNX_SESSION_PROPERTY_NAME_STATUS), 1 TSRMLS_CC);
	return 1;
}
/* }}} */

/** {{{ static fnx_session_t * fnx_session_instance(TSRMLS_D)
*/
static fnx_session_t * fnx_session_instance(TSRMLS_D) {
	fnx_session_t *instance;
	zval **sess, *member;
	zend_object *obj;
	zend_property_info *property_info;

	MAKE_STD_ZVAL(instance);
	object_init_ex(instance, fnx_session_ce);

	fnx_session_start(instance TSRMLS_CC);

	if (zend_hash_find(&EG(symbol_table), ZEND_STRS("_SESSION"), (void **)&sess) == FAILURE || Z_TYPE_PP(sess) != IS_ARRAY) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Attempt to start session failed");
		zval_ptr_dtor(&instance);
		return NULL;
	}

	MAKE_STD_ZVAL(member);
	ZVAL_STRING(member, FNX_SESSION_PROPERTY_NAME_SESSION, 0);

	obj = zend_objects_get_address(instance TSRMLS_CC);

	property_info = zend_get_property_info(obj->ce, member, 1 TSRMLS_CC);

	Z_ADDREF_P(*sess);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 3)) || (PHP_MAJOR_VERSION > 5)
	if (!obj->properties) {
		rebuild_object_properties(obj);
	}
#endif
	/** This is ugly , because we can't set a ref property through the stadard APIs */
	zend_hash_quick_update(obj->properties, property_info->name,
			property_info->name_length+1, property_info->h, (void **)sess, sizeof(zval *), NULL);


	zend_update_static_property(fnx_session_ce, ZEND_STRL(FNX_SESSION_PROPERTY_NAME_INSTANCE), instance TSRMLS_CC);

	efree(member);

	return instance;
}
/* }}} */

/** {{{ proto private Fnx_Session::__construct(void)
*/
PHP_METHOD(fnx_session, __construct) {
}
/* }}} */

/** {{{ proto private Fnx_Session::__destruct(void)
*/
PHP_METHOD(fnx_session, __destruct) {
}
/* }}} */

/** {{{ proto private Fnx_Session::__sleep(void)
*/
PHP_METHOD(fnx_session, __sleep) {
}
/* }}} */

/** {{{ proto private Fnx_Session::__wakeup(void)
*/
PHP_METHOD(fnx_session, __wakeup) {
}
/* }}} */

/** {{{ proto private Fnx_Session::__clone(void)
*/
PHP_METHOD(fnx_session, __clone) {
}
/* }}} */

/** {{{ proto public Fnx_Session::getInstance(void)
*/
PHP_METHOD(fnx_session, getInstance) {
	fnx_session_t *instance = zend_read_static_property(fnx_session_ce, ZEND_STRL(FNX_SESSION_PROPERTY_NAME_INSTANCE), 1 TSRMLS_CC);

	if (Z_TYPE_P(instance) != IS_OBJECT || !instanceof_function(Z_OBJCE_P(instance), fnx_session_ce TSRMLS_CC)) {
		if ((instance = fnx_session_instance(TSRMLS_C))) {
			RETURN_ZVAL(instance, 1, 1);
		} else {
			RETURN_NULL();
		}
	} else {
		RETURN_ZVAL(instance, 1, 0);
	}
}
/* }}} */

/** {{{ proto public Fnx_Session::count(void)
*/
PHP_METHOD(fnx_session, count) {
	zval *sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
	RETURN_LONG(zend_hash_num_elements(Z_ARRVAL_P(sess)));
}
/* }}} */

/** {{{ proto public static Fnx_Session::start()
*/
PHP_METHOD(fnx_session, start) {
	fnx_session_start(getThis() TSRMLS_CC);
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public static Fnx_Session::get($name)
*/
PHP_METHOD(fnx_session, get) {
	char *name  = NULL;
	int  len	= 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval **ret, *sess;

		sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
		if (!len) {
			RETURN_ZVAL(sess, 1, 0);
		}

		if (zend_hash_find(Z_ARRVAL_P(sess), name, len + 1, (void **)&ret) == FAILURE ){
			RETURN_NULL();
		}

		RETURN_ZVAL(*ret, 1, 0);
	}
}
/* }}} */

/** {{{ proto public staitc Fnx_Session::set($name, $value)
*/
PHP_METHOD(fnx_session, set) {
	zval *value;
	char *name;
	uint len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
		return;
	} else {
		zval *sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
		Z_ADDREF_P(value);
		if (zend_hash_update(Z_ARRVAL_P(sess), name, len + 1, &value, sizeof(zval *), NULL) == FAILURE) {
			Z_DELREF_P(value);
			RETURN_FALSE;
		}
	}

	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public staitc Fnx_Session::del($name)
*/
PHP_METHOD(fnx_session, del) {
	char *name;
	uint len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		return;
	} else {
		zval *sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);

		if (zend_hash_del(Z_ARRVAL_P(sess), name, len + 1) == SUCCESS) {
			RETURN_ZVAL(getThis(), 1, 0);
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Session::has($name)
*/
PHP_METHOD(fnx_session, has) {
	char *name;
	uint  len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		return;
	} else {
		zval *sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
		RETURN_BOOL(zend_hash_exists(Z_ARRVAL_P(sess), name, len + 1));
	}

}
/* }}} */

/** {{{ proto public Fnx_Session::rewind(void)
*/
PHP_METHOD(fnx_session, rewind) {
	zval *sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
	zend_hash_internal_pointer_reset(Z_ARRVAL_P(sess));
}
/* }}} */

/** {{{ proto public Fnx_Session::current(void)
*/
PHP_METHOD(fnx_session, current) {
	zval *sess, **ppzval;
	sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
	if (zend_hash_get_current_data(Z_ARRVAL_P(sess), (void **)&ppzval) == FAILURE) {
		RETURN_FALSE;
	}

	RETURN_ZVAL(*ppzval, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Session::key(void)
*/
PHP_METHOD(fnx_session, key) {
	zval *sess;
	char *key;
	long index;

	sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
	if (zend_hash_get_current_key(Z_ARRVAL_P(sess), &key, &index, 0) == HASH_KEY_IS_LONG) {
		RETURN_LONG(index);
	} else {
		RETURN_STRING(key, 1);
	}
}
/* }}} */

/** {{{ proto public Fnx_Session::next(void)
*/
PHP_METHOD(fnx_session, next) {
	zval *sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
	zend_hash_move_forward(Z_ARRVAL_P(sess));
}
/* }}} */

/** {{{ proto public Fnx_Session::valid(void)
*/
PHP_METHOD(fnx_session, valid) {
	zval *sess = zend_read_property(fnx_session_ce, getThis(), ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION), 1 TSRMLS_CC);
	RETURN_BOOL(zend_hash_has_more_elements(Z_ARRVAL_P(sess)) == SUCCESS);
}
/* }}} */

/** {{{ fnx_session_methods
*/
zend_function_entry fnx_session_methods[] = {
	PHP_ME(fnx_session, __construct, NULL, ZEND_ACC_CTOR|ZEND_ACC_PRIVATE)
	PHP_ME(fnx_session, __clone, NULL, ZEND_ACC_CLONE|ZEND_ACC_PRIVATE)
	PHP_ME(fnx_session, __sleep, NULL, ZEND_ACC_PRIVATE)
	PHP_ME(fnx_session, __wakeup, NULL, ZEND_ACC_PRIVATE)
	PHP_ME(fnx_session, getInstance, fnx_session_void_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(fnx_session, start, fnx_session_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, get, fnx_session_get_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, has, fnx_session_has_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, set, fnx_session_set_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, del, fnx_session_del_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, count, fnx_session_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, rewind, fnx_session_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, next, fnx_session_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, current, fnx_session_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, key, fnx_session_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_session, valid, fnx_session_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, offsetGet, get, fnx_session_get_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, offsetSet, set, fnx_session_set_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, offsetExists, has, fnx_session_has_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, offsetUnset, del, fnx_session_del_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, __get, get, fnx_session_get_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, __isset, has, fnx_session_has_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, __set, set, fnx_session_set_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_session, __unset, del, fnx_session_del_arginfo, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(session) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Session", "Fnx\\Session", fnx_session_methods);

	fnx_session_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_session_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

#ifdef HAVE_SPL
	zend_class_implements(fnx_session_ce TSRMLS_CC, 3, zend_ce_iterator, zend_ce_arrayaccess, spl_ce_Countable);
#else
	zend_class_implements(fnx_session_ce TSRMLS_CC, 2, zend_ce_iterator, zend_ce_arrayaccess);
#endif

	zend_declare_property_null(fnx_session_ce, ZEND_STRL(FNX_SESSION_PROPERTY_NAME_INSTANCE), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(fnx_session_ce, ZEND_STRL(FNX_SESSION_PROPERTY_NAME_SESSION),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(fnx_session_ce, ZEND_STRL(FNX_SESSION_PROPERTY_NAME_STATUS),   0, ZEND_ACC_PROTECTED TSRMLS_CC);

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
