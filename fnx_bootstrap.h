// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_bootstrap.h 321289 2011-12-21 02:53:29Z nechtan $ */

#ifndef FNX_BOOTSTRAP_H
#define FNX_BOOTSTRAP_H

#define FNX_DEFAULT_BOOTSTRAP		  	"Bootstrap"
#define FNX_DEFAULT_BOOTSTRAP_LOWER	  	"bootstrap"
#define FNX_DEFAULT_BOOTSTRAP_LEN		10
#define FNX_BOOTSTRAP_INITFUNC_PREFIX  	"_"

extern zend_class_entry *fnx_bootstrap_ce;

FNX_STARTUP_FUNCTION(bootstrap);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
