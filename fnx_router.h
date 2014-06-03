// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_router.h 321289 2011-12-21 02:53:29Z nechtan $ */

#ifndef FNX_ROUTER_H
#define FNX_ROUTER_H

#define FNX_ROUTER_DEFAULT_ACTION	 	"index"
#define FNX_ROUTER_DEFAULT_CONTROLLER  	"Index"
#define FNX_ROUTER_DEFAULT_MODULE	  	"Index"
#define FNX_DEFAULT_EXT 		 	   	"php"

#define FNX_ROUTER_PROPERTY_NAME_ROUTERS 		"_routes"
#define FNX_ROUTER_PROPERTY_NAME_CURRENT_ROUTE	"_current"

extern zend_class_entry * fnx_router_ce;

fnx_router_t * fnx_router_instance(fnx_router_t *this_ptr TSRMLS_DC);
zval * fnx_router_parse_parameters(char *uri TSRMLS_DC);
int fnx_router_route(fnx_router_t *router, fnx_request_t *request TSRMLS_DC);

FNX_STARTUP_FUNCTION(router);
#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
