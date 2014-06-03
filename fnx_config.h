// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_config.h 321289 2011-12-21 02:53:29Z nechtan $ */

#ifndef FNX_CONFIG_H
#define FNX_CONFIG_H

#define FNX_EXTRACT_FUNC_NAME			"extract"
#define	FNX_CONFIG_PROPERT_NAME			"_config"
#define FNX_CONFIG_PROPERT_NAME_READONLY "_readonly"

struct _fnx_config_cache {
	long ctime;
	HashTable *data;
};

typedef struct _fnx_config_cache fnx_config_cache;

extern zend_class_entry *fnx_config_ce;

fnx_config_t * fnx_config_instance(fnx_config_t *this_ptr, zval *arg1, zval *arg2 TSRMLS_DC);
void fnx_config_unserialize(fnx_config_t *self, HashTable *data TSRMLS_DC);

#ifndef pestrndup
/* before php-5.2.4, pestrndup is not defined */
#define pestrndup(s, length, persistent) ((persistent)?zend_strndup((s),(length)):estrndup((s),(length)))
#endif

FNX_STARTUP_FUNCTION(config);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
