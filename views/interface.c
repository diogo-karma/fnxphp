// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: interface.c 321289 2011-12-21 02:53:29Z nechtan $ */

zend_class_entry *fnx_view_interface_ce;

/* {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_view_assign_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_view_display_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, tpl)
	ZEND_ARG_INFO(0, tpl_vars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_view_render_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, tpl)
	ZEND_ARG_INFO(0, tpl_vars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_view_setpath_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, template_dir)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_view_getpath_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ fnx_view_interface_methods
 */
zend_function_entry fnx_view_interface_methods[] = {
	ZEND_ABSTRACT_ME(fnx_view, assign,  fnx_view_assign_arginfo)
	ZEND_ABSTRACT_ME(fnx_view, display, fnx_view_display_arginfo)
	ZEND_ABSTRACT_ME(fnx_view, render, fnx_view_render_arginfo)
	ZEND_ABSTRACT_ME(fnx_view, setScriptPath, fnx_view_setpath_arginfo)
	ZEND_ABSTRACT_ME(fnx_view, getScriptPath, fnx_view_getpath_arginfo)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
 */
FNX_STARTUP_FUNCTION(view_interface) {
	zend_class_entry ce;
	FNX_INIT_CLASS_ENTRY(ce, "Fnx_View_Interface", "Fnx\\View_Interface", fnx_view_interface_methods);
	fnx_view_interface_ce = zend_register_internal_interface(&ce TSRMLS_CC);

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

