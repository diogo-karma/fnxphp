// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: fnx_loader.c 325432 2012-04-24 09:06:40Z nechtan $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_alloc.h"
#include "ext/standard/php_smart_str.h"
#include "TSRM/tsrm_virtual_cwd.h"

#include "php_fnx.h"
#include "fnx_application.h"
#include "fnx_namespace.h"
#include "fnx_request.h"
#include "fnx_loader.h"
#include "fnx_exception.h"

zend_class_entry *fnx_loader_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(fnx_loader_void_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_loader_getinstance_arginfo, 0, 0, 0)
    ZEND_ARG_INFO(0, local_library_path)
    ZEND_ARG_INFO(0, global_library_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_loader_autoloader_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, class_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_loader_regnamespace_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, name_prefix)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_loader_islocalname_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, class_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_loader_import_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_loader_setlib_arginfo, 0, 0, 1)
    ZEND_ARG_INFO(0, library_path)
    ZEND_ARG_INFO(0, is_global)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(fnx_loader_getlib_arginfo, 0, 0, 0)
    ZEND_ARG_INFO(0, is_global)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int fnx_loader_register(TSRMLS_D)
*/
int fnx_loader_register(fnx_loader_t *loader TSRMLS_DC) {
	zval *autoload, *method, *function, *ret = NULL;
	zval **params[1] = {&autoload};

	MAKE_STD_ZVAL(autoload);
	array_init(autoload);

	MAKE_STD_ZVAL(method);
	ZVAL_STRING(method, FNX_AUTOLOAD_FUNC_NAME, 1);

	zend_hash_next_index_insert(Z_ARRVAL_P(autoload), &loader, sizeof(fnx_loader_t *), NULL);
	zend_hash_next_index_insert(Z_ARRVAL_P(autoload), &method, sizeof(zval *), NULL);

	MAKE_STD_ZVAL(function);
	ZVAL_STRING(function, FNX_SPL_AUTOLOAD_REGISTER_NAME, 0);

	do {
		zend_fcall_info fci = {
			sizeof(fci),
			EG(function_table),
			function,
			NULL,
			&ret,
			1,
			(zval ***)params,
			NULL,
			1
		};

		if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
			if (ret) {
				zval_ptr_dtor(&ret);
			}
			efree(function);
			zval_ptr_dtor(&autoload);
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to register autoload function %s", FNX_AUTOLOAD_FUNC_NAME);
			return 0;
		}

		/*{{{ no use anymore
		if (0 && !FNX_G(use_spl_autoload)) {
			zend_function *reg_function;
			zend_internal_function override_function = {
				ZEND_INTERNAL_FUNCTION,
				FNX_AUTOLOAD_FUNC_NAME,
				NULL,
				ZEND_ACC_PUBLIC,
				NULL,
				1,
				0,
				NULL,
				0,
				0,
				ZEND_FN(fnx_override_spl_autoload),
				NULL
			};
			zend_internal_function *internal_function = (zend_internal_function *)&override_function;
			internal_function->type 	= ZEND_INTERNAL_FUNCTION;
			internal_function->module 	= NULL;
			internal_function->handler 	= ZEND_FN(fnx_override_spl_autoload);
			internal_function->function_name = FNX_AUTOLOAD_FUNC_NAME;
			internal_function->scope 	=  NULL;
			internal_function->prototype = NULL;
			internal_function->arg_info  = NULL;
			internal_function->num_args  = 1;
			internal_function->required_num_args = 0;
			internal_function->pass_rest_by_reference = 0;
			internal_function->return_reference = 0;
			internal_function->fn_flags = ZEND_ACC_PUBLIC;
			function_add_ref((zend_function*)&override_function);
			//zend_register_functions
			if (zend_hash_update(EG(function_table), FNX_SPL_AUTOLOAD_REGISTER_NAME,
						sizeof(FNX_SPL_AUTOLOAD_REGISTER_NAME), &override_function, sizeof(zend_function), (void **)&reg_function) == FAILURE) {
				FNX_DEBUG("register autoload failed");
				 //no big deal
			}
		}
		}}} */

		if (ret) {
			zval_ptr_dtor(&ret);
		}
		efree(function);
		zval_ptr_dtor(&autoload);
	} while (0);
	return 1;
}
/* }}} */

/** {{{ inline int fnx_loader_is_category(char *class, uint class_len, char *category, uint category_len TSRMLS_DC)
 */
inline int fnx_loader_is_category(char *class, uint class_len, char *category, uint category_len TSRMLS_DC) {
	uint separator_len = strlen(FNX_G(name_separator));

	if (FNX_G(name_suffix)) {
		if (class_len > category_len && strncmp(class + class_len - category_len, category, category_len) == 0) {
			if (!separator_len || strncmp(class + class_len - category_len - separator_len, FNX_G(name_separator), separator_len) == 0) {
				return 1;
			}
		}
	} else {
		if (strncmp(class, category, category_len) == 0) {
			if (!separator_len || strncmp(class + category_len, FNX_G(name_separator), separator_len) == 0) {
				return 1;
			}
		}
	}

	return 0;
}
/* }}} */

/** {{{ int fnx_loader_is_local_namespace(fnx_loader_t *loader, char *class_name, int len TSRMLS_DC)
 */
int fnx_loader_is_local_namespace(fnx_loader_t *loader, char *class_name, int len TSRMLS_DC) {
	char *pos, *ns, *prefix = NULL;
	uint prefix_len = 0;
	zval *namespaces = zend_read_property(fnx_loader_ce, loader, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_NAMESPACE), 1 TSRMLS_CC);

	if (ZVAL_IS_NULL(namespaces)) {
		return 0;
	}

	pos = Z_STRVAL_P(namespaces);
	ns	= Z_STRVAL_P(namespaces);

	pos = strstr(class_name, "_");
    if (pos) {
		prefix_len 	= pos - class_name;
		prefix 		= estrndup(class_name, prefix_len);
	}
#ifdef FNX_HAVE_NAMESPACE
	else if ((pos = strstr(class_name, "\\"))) {
		prefix_len 	= pos - class_name;
		prefix 		= estrndup(class_name, prefix_len);
	}
#endif

	if (!prefix) {
		return 0;
	}

	while ((pos = strstr(ns, prefix))) {
		if ((pos == ns) && *(pos + prefix_len) == DEFAULT_DIR_SEPARATOR) {
			efree(prefix);
			return 1;
		} else if (*(pos - 1) == DEFAULT_DIR_SEPARATOR && *(pos + prefix_len) == DEFAULT_DIR_SEPARATOR) {
			efree(prefix);
			return 1;
		}
		ns = pos + prefix_len;
	}

	efree(prefix);
	return 0;
}
/* }}} */

/** {{{ fnx_loader_t * fnx_loader_instance(fnx_loader_t *this_ptr, char *library_path, char *global_path TSRMLS_DC)
 */
fnx_loader_t * fnx_loader_instance(fnx_loader_t *this_ptr, char *library_path, char *global_path TSRMLS_DC) {
	fnx_loader_t *instance;
	zval *glibrary, *library;

	instance = zend_read_static_property(fnx_loader_ce, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_INSTANCE), 1 TSRMLS_CC);

	if (IS_OBJECT == Z_TYPE_P(instance)) {
	/* unecessary since there is no set_router things
	   && instanceof_function(Z_OBJCE_P(instance), fnx_loader_ce TSRMLS_CC)) {
	 */
		if (library_path) {
			MAKE_STD_ZVAL(library);
			ZVAL_STRING(library, library_path, 1);
			zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), library TSRMLS_CC);
			zval_ptr_dtor(&library);
		}

		if (global_path) {
			MAKE_STD_ZVAL(glibrary);
			ZVAL_STRING(glibrary, global_path, 1);
			zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), glibrary TSRMLS_CC);
			zval_ptr_dtor(&glibrary);
		}
		return instance;
	}

	if (!global_path && !library_path) {
		return NULL;
	}

	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_loader_ce);
	}

	zend_update_property_null(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_NAMESPACE) TSRMLS_CC);

	if (library_path && global_path) {
		MAKE_STD_ZVAL(glibrary);
		MAKE_STD_ZVAL(library);
		ZVAL_STRING(glibrary, global_path, 1);
		ZVAL_STRING(library, library_path, 1);
		zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), library TSRMLS_CC);
		zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), glibrary TSRMLS_CC);
		zval_ptr_dtor(&library);
		zval_ptr_dtor(&glibrary);
	} else if (!global_path) {
		MAKE_STD_ZVAL(library);
		ZVAL_STRING(library, library_path, 1);
		zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), library TSRMLS_CC);
		zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), library TSRMLS_CC);
		zval_ptr_dtor(&library);
	} else {
		MAKE_STD_ZVAL(glibrary);
		ZVAL_STRING(glibrary, global_path, 1);
		zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), glibrary TSRMLS_CC);
		zend_update_property(fnx_loader_ce, instance, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), glibrary TSRMLS_CC);
		zval_ptr_dtor(&glibrary);
	}

	if (!fnx_loader_register(instance TSRMLS_CC)) {
		return NULL;
	}

	zend_update_static_property(fnx_loader_ce, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_INSTANCE), instance TSRMLS_CC);

	return instance;
}
/* }}} */

/** {{{ int fnx_loader_compose(char *path, int lenA, int use_path TSRMLS_DC)
*/
int fnx_loader_compose(char *path, int len, int use_path TSRMLS_DC) {
	zend_file_handle file_handle;

	if (php_stream_open_for_zend_ex(path, &file_handle, ENFORCE_SAFE_MODE|IGNORE_URL_WIN|STREAM_OPEN_FOR_INCLUDE TSRMLS_CC) == SUCCESS) {
		/* if (zend_stream_open(file_path, &file_handle TSRMLS_CC) == SUCCESS) { */
		zend_op_array 	*new_op_array;
		uint 			dummy = 1;

		if (!file_handle.opened_path) {
			file_handle.opened_path = estrndup(path, len);
		}

		if (zend_hash_update(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path) + 1, (void *)&dummy, sizeof(int), NULL) == SUCCESS) {
			new_op_array = zend_compile_file(&file_handle, ZEND_REQUIRE TSRMLS_CC);
			zend_destroy_file_handle(&file_handle TSRMLS_CC);
		} else {
			new_op_array = NULL;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
			zend_file_handle_dtor(&file_handle TSRMLS_CC);
#else
			zend_file_handle_dtor(&file_handle);
#endif
		}

		if (new_op_array) {
			zval *result;

			FNX_STORE_EG_ENVIRON();

			EG(return_value_ptr_ptr) 	= &result;
			EG(active_op_array) 		= new_op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
			if (!EG(active_symbol_table)) {
				zend_rebuild_symbol_table(TSRMLS_C);
			}
#endif
			zend_execute(new_op_array TSRMLS_CC);

			destroy_op_array(new_op_array TSRMLS_CC);
			efree(new_op_array);

			if (!EG(exception)) {
				if (EG(return_value_ptr_ptr)) {
					zval_ptr_dtor(EG(return_value_ptr_ptr));
				}
			}

			FNX_RESTORE_EG_ENVIRON();
		}
	} else {
		return 0;
	}

	return 1;
}
/* }}} */

/** {{{ int fnx_loader_import(char *path, int len, int use_path TSRMLS_DC)
*/
int fnx_loader_import(char *path, int len, int use_path TSRMLS_DC) {
	zend_file_handle file_handle;

	if (php_stream_open_for_zend_ex(path, &file_handle, ENFORCE_SAFE_MODE|IGNORE_URL_WIN|STREAM_OPEN_FOR_INCLUDE TSRMLS_CC) == SUCCESS) {
		/* if (zend_stream_open(file_path, &file_handle TSRMLS_CC) == SUCCESS) { */
		zend_op_array 	*new_op_array;
		uint 			dummy = 1;

		if (!file_handle.opened_path) {
			file_handle.opened_path = estrndup(path, len);
		}

		if (zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path) + 1, (void *)&dummy, sizeof(int), NULL) == SUCCESS) {
			new_op_array = zend_compile_file(&file_handle, ZEND_REQUIRE TSRMLS_CC);
			zend_destroy_file_handle(&file_handle TSRMLS_CC);
		} else {
			new_op_array = NULL;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
			zend_file_handle_dtor(&file_handle TSRMLS_CC);
#else
			zend_file_handle_dtor(&file_handle);
#endif
		}

		if (new_op_array) {
			zval *result;

			FNX_STORE_EG_ENVIRON();

			EG(return_value_ptr_ptr) = &result;
			EG(active_op_array) 	 = new_op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
			if (!EG(active_symbol_table)) {
				zend_rebuild_symbol_table(TSRMLS_C);
			}
#endif
			zend_execute(new_op_array TSRMLS_CC);

			destroy_op_array(new_op_array TSRMLS_CC);
			efree(new_op_array);
			if (!EG(exception)) {
				if (EG(return_value_ptr_ptr)) {
					zval_ptr_dtor(EG(return_value_ptr_ptr));
				}
			}
			FNX_RESTORE_EG_ENVIRON();
		}
	} else {
		return 0;
	}

	return 1;
}
/* }}} */

/** {{{ int fnx_internal_autoload(char * file_name, uint name_len, char **directory TSRMLS_DC)
 */
int fnx_internal_autoload(char *file_name, uint name_len, char **directory TSRMLS_DC) {
	zval *library_dir, *global_dir;
	char *q, *p, *seg;
	uint seg_len, directory_len, status;
	char *ext = FNX_G(ext);
	smart_str buf = {0};

	if (NULL == *directory) {
		char *library_path;
		uint  library_path_len;
		fnx_loader_t *loader;

		loader = fnx_loader_instance(NULL, NULL, NULL TSRMLS_CC);

		if (!loader) {
			/* since only call from userspace can cause loader is NULL, exception throw will works well */
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s need to be initialize first", fnx_loader_ce->name);
			return 0;
		} else {
			library_dir = zend_read_property(fnx_loader_ce, loader, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), 1 TSRMLS_CC);
			global_dir	= zend_read_property(fnx_loader_ce, loader, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), 1 TSRMLS_CC);

			if (fnx_loader_is_local_namespace(loader, file_name, name_len TSRMLS_CC)) {
				library_path = Z_STRVAL_P(library_dir);
				library_path_len = Z_STRLEN_P(library_dir);
			} else {
				library_path = Z_STRVAL_P(global_dir);
				library_path_len = Z_STRLEN_P(global_dir);
			}
		}

		if (NULL == library_path) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s requires %s(which set the library_directory) to be initialized first", fnx_loader_ce->name, fnx_application_ce->name);
			return 0;
		}

		smart_str_appendl(&buf, library_path, library_path_len);
	} else {
		smart_str_appendl(&buf, *directory, strlen(*directory));
		efree(*directory);
	}

	directory_len = buf.len;

	/* aussume all the path is not end in slash */
	smart_str_appendc(&buf, DEFAULT_SLASH);

	p = file_name;
	q = p;

	while (1) {
		while(++q && *q != '_' && *q != '\0');

		if (*q != '\0') {
			seg_len	= q - p;
			seg	 	= estrndup(p, seg_len);
			smart_str_appendl(&buf, seg, seg_len);
			efree(seg);
			smart_str_appendc(&buf, DEFAULT_SLASH);
			p 		= q + 1;
		} else {
			break;
		}
	}

	if (FNX_G(lowcase_path)) {
		/* all path of library is lowercase */
		zend_str_tolower(buf.c + directory_len, buf.len - directory_len);
	}

	smart_str_appendl(&buf, p, strlen(p));
	smart_str_appendc(&buf, '.');
	smart_str_appendl(&buf, ext, strlen(ext));

	smart_str_0(&buf);

	if (directory) {
		*(directory) = estrndup(buf.c, buf.len);
	}

	status = fnx_loader_import(buf.c, buf.len, 0 TSRMLS_CC);
	smart_str_free(&buf);

	if (!status)
	   	return 0;

	return 1;
}
/* }}} */

/** {{{ int fnx_loader_register_namespace_single(fnx_loader_t *loader, char *prefix, uint len TSRMLS_DC)
 */
int fnx_loader_register_namespace_single(fnx_loader_t *loader, char *prefix, uint len TSRMLS_DC) {
	zval *namespaces;
	smart_str buf = {NULL, 0, 0};

	namespaces = zend_read_property(fnx_loader_ce, loader, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_NAMESPACE), 1 TSRMLS_CC);

	if (Z_TYPE_P(namespaces) == IS_NULL) {
		smart_str_appendc(&buf, DEFAULT_DIR_SEPARATOR);
	} else {
		smart_str_appendl(&buf, Z_STRVAL_P(namespaces), Z_STRLEN_P(namespaces));
		efree(Z_STRVAL_P(namespaces));
	}

	smart_str_appendl(&buf, prefix, len);
	smart_str_appendc(&buf, DEFAULT_DIR_SEPARATOR);

	ZVAL_STRINGL(namespaces, buf.c, buf.len, 1);

	smart_str_free(&buf);

	return 1;
}
/* }}} */

/** {{{ int fnx_loader_register_namespace_multi(fnx_loader_t *loader, zval *prefixes TSRMLS_DC)
 */
int fnx_loader_register_namespace_multi(fnx_loader_t *loader, zval *prefixes TSRMLS_DC) {
	zval *namespaces, **ppzval;
	HashTable *ht;
	smart_str buf = {0};

	namespaces = zend_read_property(fnx_loader_ce, loader, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_NAMESPACE), 1 TSRMLS_CC);

	if (Z_TYPE_P(namespaces) == IS_NULL) {
		smart_str_appendc(&buf, DEFAULT_DIR_SEPARATOR);
	} else {
		smart_str_appendl(&buf, Z_STRVAL_P(namespaces), Z_STRLEN_P(namespaces));
	}

	ht = Z_ARRVAL_P(prefixes);
	for(zend_hash_internal_pointer_reset(ht);
			zend_hash_has_more_elements(ht) == SUCCESS;
			zend_hash_move_forward(ht)) {

		if (zend_hash_get_current_data(ht, (void**)&ppzval) == FAILURE) {
			continue;
		} else {
			smart_str_appendl(&buf, Z_STRVAL_PP(ppzval), Z_STRLEN_PP(ppzval));
			smart_str_appendc(&buf, DEFAULT_DIR_SEPARATOR);
		}
	}

	ZVAL_STRINGL(namespaces, buf.c, buf.len, 1);

	smart_str_free(&buf);

	return 1;
}
/* }}} */

/** {{{ proto private Fnx_Loader::__construct(void)
*/
PHP_METHOD(fnx_loader, __construct) {
}
/* }}} */

/** {{{ proto private Fnx_Loader::__sleep(void)
*/
PHP_METHOD(fnx_loader, __sleep) {
}
/* }}} */

/** {{{ proto private Fnx_Loader::__wakeup(void)
*/
PHP_METHOD(fnx_loader, __wakeup) {
}
/* }}} */

/** {{{ proto private Fnx_Loader::__clone(void)
*/
PHP_METHOD(fnx_loader, __clone) {
}
/* }}} */

/** {{{ proto public Fnx_Loader::registerLocalNamespace(mixed $namespace)
*/
PHP_METHOD(fnx_loader, registerLocalNamespace) {
	zval *namespaces;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &namespaces) == FAILURE) {
		return;
	}

	if (IS_STRING == Z_TYPE_P(namespaces)) {
		if (fnx_loader_register_namespace_single(getThis(), Z_STRVAL_P(namespaces), Z_STRLEN_P(namespaces) TSRMLS_CC)) {
			RETURN_ZVAL(getThis(), 1, 0);
		}
	} else if (IS_ARRAY == Z_TYPE_P(namespaces)) {
		if(fnx_loader_register_namespace_multi(getThis(), namespaces TSRMLS_CC)) {
			RETURN_ZVAL(getThis(), 1, 0);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid parameters provided, must be a string, or an array");
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_Loader::getLocalNamespace(void)
*/
PHP_METHOD(fnx_loader, getLocalNamespace) {
	zval *namespaces = zend_read_property(fnx_loader_ce, getThis(), ZEND_STRL(FNX_LOADER_PROPERTY_NAME_NAMESPACE), 1 TSRMLS_CC);
	RETURN_ZVAL(namespaces, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Loader::clearLocalNamespace(void)
*/
PHP_METHOD(fnx_loader, clearLocalNamespace) {
	zend_update_property_null(fnx_loader_ce, getThis(), ZEND_STRL(FNX_LOADER_PROPERTY_NAME_NAMESPACE) TSRMLS_CC);

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_Loader::isLocalName(string $class_name)
*/
PHP_METHOD(fnx_loader, isLocalName) {
	zval *name;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &name) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(name) != IS_STRING) {
		RETURN_FALSE;
	}

	RETURN_BOOL(fnx_loader_is_local_namespace(getThis(), Z_STRVAL_P(name), Z_STRLEN_P(name) TSRMLS_CC));
}
/* }}} */

/** {{{ proto public Fnx_Loader::setLibraryPath(string $path, $global = FALSE)
*/
PHP_METHOD(fnx_loader, setLibraryPath) {
	char *library;
	uint len;
	zend_bool global = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &library, &len, &global) == FAILURE) {
		return;
	}

	if (!global) {
		zend_update_property_stringl(fnx_loader_ce, getThis(), ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), library, len TSRMLS_CC);
	} else {
		zend_update_property_stringl(fnx_loader_ce, getThis(), ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), library, len TSRMLS_CC);
	}

	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_Loader::getLibraryPath($global = FALSE)
*/
PHP_METHOD(fnx_loader, getLibraryPath) {
	zval *library;
	zend_bool global = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|b", &global) == FAILURE) {
		return;
	}

	if (!global) {
		library = zend_read_property(fnx_loader_ce, getThis(), ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), 1 TSRMLS_CC);
	} else {
		library = zend_read_property(fnx_loader_ce, getThis(), ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), 1 TSRMLS_CC);
	}

	RETURN_ZVAL(library, 1, 0);
}
/* }}} */

/** {{{ proto public static Fnx_Loader::import($file)
*/
PHP_METHOD(fnx_loader, import) {
	char *file;
	uint len, need_free = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s" ,&file, &len) == FAILURE) {
		return;
	}

	if (!len) {
		RETURN_FALSE;
	} else {
		int  retval = 0;

		if (!IS_ABSOLUTE_PATH(file, len)) {
			fnx_loader_t *loader = fnx_loader_instance(NULL, NULL, NULL TSRMLS_CC);
			if (!loader) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s need to be initialize first", fnx_loader_ce->name);
				RETURN_FALSE;
			} else {
				zval *library = zend_read_property(fnx_loader_ce, loader, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), 1 TSRMLS_CC);
				len = spprintf(&file, 0, "%s%c%s", Z_STRVAL_P(library), DEFAULT_SLASH, file);
				need_free = 1;
			}
		}

		retval = (zend_hash_exists(&EG(included_files), file, len + 1));
		if (retval) {
			if (need_free) {
				efree(file);
			}
			RETURN_TRUE;
		}

		retval = fnx_loader_import(file, len, 0 TSRMLS_CC);
		if (need_free) {
			efree(file);
		}

		RETURN_BOOL(retval);
	}
}
/* }}} */

/** {{{ proto public Fnx_Loader::autoload($class_name)
*/
PHP_METHOD(fnx_loader, autoload) {
	char *class_name, *origin_classname, *app_directory, *directory = NULL, *file_name = NULL;
#ifdef FNX_HAVE_NAMESPACE
	char *origin_lcname = NULL;
#endif
	uint separator_len, class_name_len, file_name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &class_name, &class_name_len) == FAILURE) {
		return;
	}

	separator_len = strlen(FNX_G(name_separator));
	app_directory = FNX_G(directory);
	origin_classname = class_name;

	do {
		if (!class_name_len) {
			break;
		}
#ifdef FNX_HAVE_NAMESPACE
		{
			int pos = 0;
			origin_lcname = estrndup(class_name, class_name_len);
			class_name 	  = origin_lcname;
			while (pos < class_name_len) {
				if (*(class_name + pos) == '\\') {
					*(class_name + pos) = '_';
				}
				pos += 1;
			}
		}
#endif

		if (strncmp(class_name, FNX_LOADER_RESERVERD, FNX_LOADER_LEN_RESERVERD) == 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "You should not use '%s' as class name prefix", FNX_LOADER_RESERVERD);
		}

		if (fnx_loader_is_category(class_name, class_name_len, FNX_LOADER_MODEL, FNX_LOADER_LEN_MODEL TSRMLS_CC)) {
			/* this is a model class */
			spprintf(&directory, 0, "%s/%s", app_directory, FNX_MODEL_DIRECTORY_NAME);
			file_name_len = class_name_len - separator_len - FNX_LOADER_LEN_MODEL;

			if (FNX_G(name_suffix)) {
				file_name = estrndup(class_name, file_name_len);
			} else {
				file_name = estrdup(class_name + FNX_LOADER_LEN_MODEL + separator_len);
			}

			break;
		}

		if (fnx_loader_is_category(class_name, class_name_len, FNX_LOADER_PLUGIN, FNX_LOADER_LEN_PLUGIN TSRMLS_CC)) {
			/* this is a plugin class */
			spprintf(&directory, 0, "%s/%s", app_directory, FNX_PLUGIN_DIRECTORY_NAME);
			file_name_len = class_name_len - separator_len - FNX_LOADER_LEN_PLUGIN;

			if (FNX_G(name_suffix)) {
				file_name = estrndup(class_name, file_name_len);
			} else {
				file_name = estrdup(class_name + FNX_LOADER_LEN_PLUGIN + separator_len);
			}

			break;
		}

		if (fnx_loader_is_category(class_name, class_name_len, FNX_LOADER_CONTROLLER, FNX_LOADER_LEN_CONTROLLER TSRMLS_CC)) {
			/* this is a controller class */
			spprintf(&directory, 0, "%s/%s", app_directory, FNX_CONTROLLER_DIRECTORY_NAME);
			file_name_len = class_name_len - separator_len - FNX_LOADER_LEN_CONTROLLER;

			if (FNX_G(name_suffix)) {
				file_name = estrndup(class_name, file_name_len);
			} else {
				file_name = estrdup(class_name + FNX_LOADER_LEN_CONTROLLER + separator_len);
			}

			break;
		}


/* {{{ This only effects internally */
		if (FNX_G(st_compatible) && (strncmp(class_name, FNX_LOADER_DAO, FNX_LOADER_LEN_DAO) == 0
					|| strncmp(class_name, FNX_LOADER_SERVICE, FNX_LOADER_LEN_SERVICE) == 0)) {
			/* this is a model class */
			spprintf(&directory, 0, "%s/%s", app_directory, FNX_MODEL_DIRECTORY_NAME);
		}
/* }}} */

		file_name_len = class_name_len;
		file_name     = class_name;

	} while(0);

	if (!app_directory && directory) {
		efree(directory);
#ifdef FNX_HAVE_NAMESPACE
		if (origin_lcname) {
			efree(origin_lcname);
		}
#endif
		if (file_name != class_name) {
			efree(file_name);
		}

		php_error_docref(NULL TSRMLS_CC, E_WARNING,
				"Couldn't load a framework MVC class without an %s initializing", fnx_application_ce->name);
		RETURN_FALSE;
	}

	if (!FNX_G(use_spl_autoload)) {
		/** directory might be NULL since we passed a NULL */
		if (fnx_internal_autoload(file_name, file_name_len, &directory TSRMLS_CC)) {
			if (zend_hash_exists(EG(class_table), zend_str_tolower_dup(origin_classname, class_name_len), class_name_len + 1)) {
#ifdef FNX_HAVE_NAMESPACE
				if (origin_lcname) {
					efree(origin_lcname);
				}
#endif
				if (directory) {
					efree(directory);
				}

				if (file_name != class_name) {
					efree(file_name);
				}

				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_STRICT, "Could not find class %s in %s", class_name, directory);
			}
		}  else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Could not find script %s", directory);
		}

#ifdef FNX_HAVE_NAMESPACE
		if (origin_lcname) {
			efree(origin_lcname);
		}
#endif
		if (directory) {
			efree(directory);
		}
		if (file_name != class_name) {
			efree(file_name);
		}
		RETURN_TRUE;
	} else {
		char *lower_case_name = zend_str_tolower_dup(origin_classname, class_name_len);
		if (fnx_internal_autoload(file_name, file_name_len, &directory TSRMLS_CC) &&
				zend_hash_exists(EG(class_table), lower_case_name, class_name_len + 1)) {
#ifdef FNX_HAVE_NAMESPACE
			if (origin_lcname) {
				efree(origin_lcname);
			}
#endif
			if (directory) {
				efree(directory);
			}
			if (file_name != class_name) {
				efree(file_name);
			}

			efree(lower_case_name);
			RETURN_TRUE;
		}
#ifdef FNX_HAVE_NAMESPACE
		if (origin_lcname) {
			efree(origin_lcname);
		}
#endif
		if (directory) {
			efree(directory);
		}
		if (file_name != class_name) {
			efree(file_name);
		}
		efree(lower_case_name);
		RETURN_FALSE;
	}
}
/* }}} */

/** {{{ proto public Fnx_Loader::getInstance($library = NULL, $global_library = NULL)
*/
PHP_METHOD(fnx_loader, getInstance) {
	char *library	 	= NULL;
	char *global	 	= NULL;
	int	 library_len 	= 0;
	int  global_len	 	= 0;
	fnx_loader_t *loader;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &library, &library_len, &global, &global_len) == FAILURE) {
		return;
	} 

	loader = fnx_loader_instance(NULL, library, global TSRMLS_CC);
	if (loader)
		RETURN_ZVAL(loader, 1, 0);

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto private Fnx_Loader::__desctruct(void)
*/
PHP_METHOD(fnx_loader, __destruct) {
}
/* }}} */

/** {{{ proto fnx_override_spl_autoload($class_name)
*/
PHP_FUNCTION(fnx_override_spl_autoload) {
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s is disabled by ap.use_spl_autoload", FNX_SPL_AUTOLOAD_REGISTER_NAME);
	RETURN_BOOL(0);
}
/* }}} */

/** {{{ fnx_loader_methods
*/
zend_function_entry fnx_loader_methods[] = {
	PHP_ME(fnx_loader, __construct, 			fnx_loader_void_arginfo, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR)
	PHP_ME(fnx_loader, __clone,					NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CLONE)
	PHP_ME(fnx_loader, __sleep,					NULL, ZEND_ACC_PRIVATE)
	PHP_ME(fnx_loader, __wakeup,				NULL, ZEND_ACC_PRIVATE)
	PHP_ME(fnx_loader, autoload,				fnx_loader_autoloader_arginfo,  ZEND_ACC_PUBLIC)
	PHP_ME(fnx_loader, getInstance,				fnx_loader_getinstance_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(fnx_loader, registerLocalNamespace,	fnx_loader_regnamespace_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_loader, getLocalNamespace,		fnx_loader_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_loader, clearLocalNamespace,		fnx_loader_void_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_loader, isLocalName,				fnx_loader_islocalname_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_loader, import,					fnx_loader_import_arginfo, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(fnx_loader, setLibraryPath,			fnx_loader_setlib_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_loader, getLibraryPath,			fnx_loader_getlib_arginfo, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(loader) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_Loader",  "Fnx\\Loader", fnx_loader_methods);
	fnx_loader_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);
	fnx_loader_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(fnx_loader_ce, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_NAMESPACE), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_loader_ce, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_LIBRARY), 	 ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_loader_ce, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_GLOBAL_LIB), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_loader_ce, ZEND_STRL(FNX_LOADER_PROPERTY_NAME_INSTANCE),	 ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
