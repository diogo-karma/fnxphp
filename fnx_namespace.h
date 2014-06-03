// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_namespace.h 321289 2011-12-21 02:53:29Z nechtan $ */

#ifndef FNX_NAMESPACE_H
#define FNX_NAMESPACE_H

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
#define FNX_BEGIN_ARG_INFO		ZEND_BEGIN_ARG_INFO
#define FNX_BEGIN_ARG_INFO_EX	ZEND_BEGIN_ARG_INFO_EX

#define namespace_switch(n) \
	(FNX_G(use_namespace)? n##_ns : n)

#define FNX_INIT_CLASS_ENTRY(ce, name, name_ns, methods) \
	if(FNX_G(use_namespace)) { \
		INIT_CLASS_ENTRY(ce, name_ns, methods); \
	} else { \
		INIT_CLASS_ENTRY(ce, name, methods); \
	}
#else

#ifdef FNX_HAVE_NAMESPACE
#undef FNX_HAVE_NAMESPACE
#endif

#define namespace_switch(n)	(n)
#define FNX_INIT_CLASS_ENTRY(ce, name, name_ns, methods)  INIT_CLASS_ENTRY(ce, name, methods)
#define FNX_BEGIN_ARG_INFO		static ZEND_BEGIN_ARG_INFO
#define FNX_BEGIN_ARG_INFO_EX	static ZEND_BEGIN_ARG_INFO_EX

#endif

#define FNX_END_ARG_INFO		ZEND_END_ARG_INFO
#define FNX_ARG_INFO			ZEND_ARG_INFO
#define FNX_ARG_OBJ_INFO 	ZEND_ARG_OBJ_INFO
#define FNX_ARG_ARRAY_INFO 	ZEND_ARG_ARRAY_INFO

#endif	/* PHP_FNX_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
