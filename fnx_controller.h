// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_controller.h 324667 2012-03-31 13:55:08Z nechtan $ */

#ifndef FNX_CONTROLLER_H
#define FNX_CONTROLLER_H

#define FNX_CONTROLLER_PROPERTY_NAME_MODULE		"_module"
#define FNX_CONTROLLER_PROPERTY_NAME_NAME		"_name"
#define FNX_CONTROLLER_PROPERTY_NAME_SCRIPT		"_script_path"
#define FNX_CONTROLLER_PROPERTY_NAME_RESPONSE	"_response"
#define FNX_CONTROLLER_PROPERTY_NAME_REQUEST	"_request"
#define FNX_CONTROLLER_PROPERTY_NAME_ARGS		"_invoke_args"
#define FNX_CONTROLLER_PROPERTY_NAME_ACTIONS	"actions"
#define FNX_CONTROLLER_PROPERTY_NAME_VIEW		"_view"

#define FNX_CONTROLLER_PROPERTY_NAME_RENDER     "fnxAutoRender"

extern zend_class_entry *fnx_controller_ce;
int fnx_controller_construct(zend_class_entry *ce, fnx_controller_t *self,
		fnx_request_t *request, fnx_response_t *response, fnx_view_t *view, zval *args TSRMLS_DC);
FNX_STARTUP_FUNCTION(controller);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

