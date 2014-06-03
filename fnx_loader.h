// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_loader.h 324659 2012-03-31 09:01:41Z nechtan $ */

#ifndef FNX_LOADER_H
#define FNX_LOADER_H

#define FNX_DEFAULT_VIEW_EXT     	  		"phtml"
#define FNX_DEFAULT_LIBRARY_EXT		   		FNX_DEFAULT_CONTROLLER_EXT

#define FNX_LIBRARY_DIRECTORY_NAME    		"library"
#define FNX_CONTROLLER_DIRECTORY_NAME 		"controllers"
#define FNX_PLUGIN_DIRECTORY_NAME 	  		"plugins"
#define FNX_MODULE_DIRECTORY_NAME     		"modules"
#define FNX_VIEW_DIRECTORY_NAME       		"views"
#define FNX_MODEL_DIRECTORY_NAME      		"models"

#define FNX_SPL_AUTOLOAD_REGISTER_NAME 		"spl_autoload_register"
#define FNX_AUTOLOAD_FUNC_NAME 				"autoload"
#define FNX_LOADER_PROPERTY_NAME_INSTANCE	"_instance"
#define FNX_LOADER_PROPERTY_NAME_NAMESPACE	"_local_ns"

#define FNX_LOADER_CONTROLLER				"Controller"
#define FNX_LOADER_LEN_CONTROLLER			10
#define FNX_LOADER_MODEL					"Model"
#define FNX_LOADER_LEN_MODEL				5
#define FNX_LOADER_PLUGIN					"Plugin"
#define FNX_LOADER_LEN_PLUGIN				6
#define FNX_LOADER_RESERVERD				"Fnx_"
#define FNX_LOADER_LEN_RESERVERD			3

/* {{{ This only effects internally */
#define FNX_LOADER_DAO						"Dao_"
#define FNX_LOADER_LEN_DAO					4
#define FNX_LOADER_SERVICE					"Service_"
#define FNX_LOADER_LEN_SERVICE				8
/* }}} */

#define	FNX_LOADER_PROPERTY_NAME_LIBRARY	"_library"
#define FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB "_global_library"

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
#define FNX_STORE_EG_ENVIRON() \
	{ \
		zval ** __old_return_value_pp   = EG(return_value_ptr_ptr); \
		zend_op ** __old_opline_ptr  	= EG(opline_ptr); \
		zend_op_array * __old_op_array  = EG(active_op_array);

#define FNX_RESTORE_EG_ENVIRON() \
		EG(return_value_ptr_ptr) = __old_return_value_pp;\
		EG(opline_ptr)			 = __old_opline_ptr; \
		EG(active_op_array)		 = __old_op_array; \
	}

#else

#define FNX_STORE_EG_ENVIRON() \
	{ \
		zval ** __old_return_value_pp  		   = EG(return_value_ptr_ptr); \
		zend_op ** __old_opline_ptr 		   = EG(opline_ptr); \
		zend_op_array * __old_op_array 		   = EG(active_op_array); \
		zend_function_state * __old_func_state = EG(function_state_ptr);

#define FNX_RESTORE_EG_ENVIRON() \
		EG(return_value_ptr_ptr) = __old_return_value_pp;\
		EG(opline_ptr)			 = __old_opline_ptr; \
		EG(active_op_array)		 = __old_op_array; \
		EG(function_state_ptr)	 = __old_func_state; \
	}

#endif

extern zend_class_entry *fnx_loader_ce;

int fnx_internal_autoload(char *file_name, uint name_len, char **directory TSRMLS_DC);
int fnx_loader_import(char *path, int len, int use_path TSRMLS_DC);
int fnx_loader_compose(char *path, int len, int use_path TSRMLS_DC);
int fnx_register_autoloader(fnx_loader_t *loader TSRMLS_DC);
int fnx_loader_register_namespace_single(fnx_loader_t *loader, char *prefix, uint len TSRMLS_DC);
fnx_loader_t * fnx_loader_instance(fnx_loader_t *this_ptr, char *library_path, char *global_path TSRMLS_DC);

extern PHPAPI int php_stream_open_for_zend_ex(const char *filename, zend_file_handle *handle, int mode TSRMLS_DC);

FNX_STARTUP_FUNCTION(loader);

#endif
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
