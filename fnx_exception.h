// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_exception.h 325425 2012-04-23 11:39:24Z nechtan $ */

#ifndef FNX_EXCEPTION_H
#define FNX_EXCEPTION_H

#define FNX_MAX_BUILDIN_EXCEPTION	10

#define FNX_ERR_BASE 				512
#define FNX_UERR_BASE				1024
#define FNX_ERR_MASK				127

#define FNX_ERR_STARTUP_FAILED 		512
#define FNX_ERR_ROUTE_FAILED 		513
#define FNX_ERR_DISPATCH_FAILED 	514
#define FNX_ERR_NOTFOUND_MODULE 	515
#define FNX_ERR_NOTFOUND_CONTROLLER 516
#define FNX_ERR_NOTFOUND_ACTION 	517
#define FNX_ERR_NOTFOUND_VIEW 		518
#define FNX_ERR_CALL_FAILED			519
#define FNX_ERR_AUTOLOAD_FAILED 	520
#define FNX_ERR_TYPE_ERROR			521

#define FNX_EXCEPTION_OFFSET(x) (x & FNX_ERR_MASK)

#define FNX_CORRESPOND_ERROR(x) (x>>9L)

#define FNX_EXCEPTION_HANDLE(dispatcher, request, response) \
	if (EG(exception)) { \
		if (FNX_G(catch_exception)) { \
			fnx_dispatcher_exception_handler(dispatcher, request, response TSRMLS_CC); \
		} \
		return NULL; \
	}

#define FNX_EXCEPTION_HANDLE_NORET(dispatcher, request, response) \
	if (EG(exception)) { \
		if (FNX_G(catch_exception)) { \
			fnx_dispatcher_exception_handler(dispatcher, request, response TSRMLS_CC); \
		} \
	}

#define FNX_EXCEPTION_ERASE_EXCEPTION() \
	do { \
		EG(current_execute_data)->opline = EG(opline_before_exception); \
	} while(0)

extern zend_class_entry *fnx_ce_RuntimeException;
extern zend_class_entry *fnx_exception_ce;
extern zend_class_entry *fnx_buildin_exceptions[FNX_MAX_BUILDIN_EXCEPTION];
void fnx_trigger_error(int type TSRMLS_DC, char *format, ...);
void fnx_throw_exception(long code, char *message TSRMLS_DC);

FNX_STARTUP_FUNCTION(exception);

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
