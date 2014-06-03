// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_session.h 315615 2011-08-27 14:14:48Z nechtan $ */

#ifndef FNX_SESSION_H
#define FNX_SESSION_H

#define FNX_SESSION_PROPERTY_NAME_STATUS	    "_started"
#define FNX_SESSION_PROPERTY_NAME_SESSION	"_session"
#define FNX_SESSION_PROPERTY_NAME_INSTANCE	"_instance"

extern zend_class_entry *fnx_session_ce;

PHPAPI void php_session_start(TSRMLS_D);
FNX_STARTUP_FUNCTION(session);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
