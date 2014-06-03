// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_registry.h 321289 2011-12-21 02:53:29Z nechtan $ */

#ifndef FNX_REGISTRY_H
#define FNX_REGISTRY_H

#define FNX_REGISTRY_PROPERTY_NAME_ENTRYS 	"_entries"
#define FNX_REGISTRY_PROPERTY_NAME_INSTANCE	"_instance"

extern zend_class_entry *fnx_registry_ce;

FNX_STARTUP_FUNCTION(registry);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
