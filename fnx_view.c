// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_view.c 321289 2011-12-21 02:53:29Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_exceptions.h"
#include "Zend/zend_alloc.h"

#include "php_fnx.h"
#include "fnx_namespace.h"
#include "fnx_exception.h"
#include "fnx_loader.h"
#include "fnx_view.h"

#include "views/interface.c"
#include "views/simple.c"

#if 0
static fnx_view_struct fnx_buildin_views[] = {
	{"classical", &fnx_view_simple_ce, fnx_view_simple_init},
	{NULL, NULL, NULL}
};
#endif

#if 0
/** {{{ fnx_view_t * fnx_view_instance(fnx_view_t *this_ptr TSRMLS_DC)
*/
fnx_view_t * fnx_view_instance(fnx_view_t *this_ptr TSRMLS_DC) {
	fnx_view_t		*view	= NULL;
	fnx_view_struct 	*p 		= fnx_buildin_views;

	for(;;++p) {
		fnx_current_view = p;
		fnx_view_ce = *(p->ce);
		break;
	}

	fnx_view_ce = *(fnx_current_view->ce);

	MAKE_STD_ZVAL(view);
	object_init_ex(view, *(fnx_current_view->ce));

	if (fnx_current_view->init) {
		fnx_current_view->init(view TSRMLS_CC);
	}
	MAKE_STD_ZVAL(view);
	object_init_ex(view, fnx_view_simple_ce);
	fnx_view_simple_init(view TSRMLS_CC);

	return view;
}
/* }}} */
#endif

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(view) {
	FNX_STARTUP(view_interface);
	FNX_STARTUP(view_simple);

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
