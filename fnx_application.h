// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_application.h 321289 2011-12-21 02:53:29Z nechtan $ */

#ifndef PHP_FNX_APPLICATION_H
#define PHP_FNX_APPLICATION_H

#define FNX_APPLICATION_PROPERTY_NAME_CONFIG		"config"
#define FNX_APPLICATION_PROPERTY_NAME_DISPATCHER	"dispatcher"
#define FNX_APPLICATION_PROPERTY_NAME_RUN			"_running"
#define FNX_APPLICATION_PROPERTY_NAME_APP			"_app"
#define FNX_APPLICATION_PROPERTY_NAME_ENV			"_environ"
#define FNX_APPLICATION_PROPERTY_NAME_MODULES		"_modules"
#define FNX_APPLICATION_PROPERTY_NAME_ERRNO			"_err_no"
#define FNX_APPLICATION_PROPERTY_NAME_ERRMSG		"_err_msg"

extern zend_class_entry *fnx_application_ce;

int fnx_application_is_module_name(char *name, int len TSRMLS_DC);

FNX_STARTUP_FUNCTION(application);
#endif
