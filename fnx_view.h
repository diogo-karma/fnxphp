// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_view.h 325381 2012-04-21 01:51:59Z nechtan $ */

#ifndef FNX_VIEW_H
#define FNX_VIEW_H

#define fnx_view_instance fnx_view_simple_instance
#define fnx_view_ce		 fnx_view_simple_ce

#define FNX_VIEW_PROPERTY_NAME_TPLVARS 	"_tpl_vars"
#define FNX_VIEW_PROPERTY_NAME_TPLDIR	"_tpl_dir"
#define FNX_VIEW_PROPERTY_NAME_OPTS 	"_options"

extern zend_class_entry *fnx_view_interface_ce;
extern zend_class_entry *fnx_view_simple_ce;

fnx_view_t * fnx_view_instance(fnx_view_t * this_ptr, zval *tpl_dir, zval *options TSRMLS_DC);

FNX_STARTUP_FUNCTION(view);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
