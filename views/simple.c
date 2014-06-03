// Copyright (C) 2011 Dg Nechtan <dnechtan@gmail.com>, MIT

/* $Id: simple.c 325383 2012-04-21 02:01:49Z nechtan $ */

#include "main/php_output.h"

#define VIEW_BUFFER_BLOCK_SIZE	4096
#define VIEW_BUFFER_SIZE_MASK 	4095

zend_class_entry *fnx_view_simple_ce;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
struct _fnx_view_simple_buffer {
	char *buffer;
	unsigned long size;
	unsigned long len;
	struct _fnx_view_simple_buffer *prev;
};

typedef struct _fnx_view_simple_buffer fnx_view_simple_buffer;

typedef int(*fnx_body_write_func)(const char *str, uint str_length TSRMLS_DC);

/** {{{ MACROS
 */
#define FNX_REDIRECT_OUTPUT_BUFFER(seg) \
	do { \
		if (!FNX_G(owrite_handler)) { \
			FNX_G(owrite_handler) = OG(php_body_write); \
		} \
		OG(php_body_write) = fnx_view_simple_render_write; \
		old_scope = EG(scope); \
		EG(scope) = fnx_view_simple_ce; \
		seg = (fnx_view_simple_buffer *)emalloc(sizeof(fnx_view_simple_buffer)); \
		memset(seg, 0, sizeof(fnx_view_simple_buffer)); \
		seg->prev  	 = FNX_G(buffer);\
		FNX_G(buffer) = seg; \
		FNX_G(buf_nesting)++;\
	} while (0)

#define FNX_RESTORE_OUTPUT_BUFFER(seg) \
	do { \
		OG(php_body_write) 	= (fnx_body_write_func)FNX_G(owrite_handler); \
		EG(scope) 			= old_scope; \
		FNX_G(buffer)  		= seg->prev; \
		if (!(--FNX_G(buf_nesting))) { \
			if (FNX_G(buffer)) { \
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Fnx output buffer collapsed"); \
			} else { \
				FNX_G(owrite_handler) = NULL; \
			} \
		} \
		if (seg->size) { \
			efree(seg->buffer); \
		} \
		efree(seg); \
	} while (0)
/* }}} */
#endif

/** {{{ ARG_INFO */
ZEND_BEGIN_ARG_INFO_EX(fnx_view_simple_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, tempalte_dir)
	ZEND_ARG_ARRAY_INFO(0, options, 1)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(fnx_view_simple_get_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(fnx_view_simple_isset_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(fnx_view_simple_assign_by_ref_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(1, value)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(fnx_view_simple_eval_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, tpl_str)
	ZEND_ARG_INFO(0, vars)
ZEND_END_ARG_INFO();

ZEND_BEGIN_ARG_INFO_EX(fnx_view_simple_clear_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO();
/* }}} */

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
/** {{{ static int fnx_view_simple_render_write(const char *str, uint str_length TSRMLS_DC)
*/
static int fnx_view_simple_render_write(const char *str, uint str_length TSRMLS_DC) {
	char *target;
	fnx_view_simple_buffer *buffer = FNX_G(buffer);

	if (!buffer->size) {
		buffer->size   = (str_length | VIEW_BUFFER_SIZE_MASK) + 1;
		buffer->len	   = str_length;
		buffer->buffer = emalloc(buffer->size);
		target = buffer->buffer;
	} else {
		size_t len = buffer->len + str_length;

		if (buffer->size < len + 1) {
			buffer->size   = (len | VIEW_BUFFER_SIZE_MASK) + 1;
			buffer->buffer = erealloc(buffer->buffer, buffer->size);
			if (!buffer->buffer) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Fnx output buffer collapsed");
			}
		}

		target = buffer->buffer + buffer->len;
		buffer->len = len;
	}

	memcpy(target, str, str_length);
	target[str_length] = '\0';

	return str_length;
}
/* }}} */
#endif

static int fnx_view_simple_valid_var_name(char *var_name, int len) /* {{{ */
{
	int i, ch;

	if (!var_name)
		return 0;

	/* These are allowed as first char: [a-zA-Z_\x7f-\xff] */
	ch = (int)((unsigned char *)var_name)[0];
	if (var_name[0] != '_' &&
			(ch < 65  /* A    */ || /* Z    */ ch > 90)  &&
			(ch < 97  /* a    */ || /* z    */ ch > 122) &&
			(ch < 127 /* 0x7f */ || /* 0xff */ ch > 255)
	   ) {
		return 0;
	}

	/* And these as the rest: [a-zA-Z0-9_\x7f-\xff] */
	if (len > 1) {
		for (i = 1; i < len; i++) {
			ch = (int)((unsigned char *)var_name)[i];
			if (var_name[i] != '_' &&
					(ch < 48  /* 0    */ || /* 9    */ ch > 57)  &&
					(ch < 65  /* A    */ || /* Z    */ ch > 90)  &&
					(ch < 97  /* a    */ || /* z    */ ch > 122) &&
					(ch < 127 /* 0x7f */ || /* 0xff */ ch > 255)
			   ) {
				return 0;
			}
		}
	}
	return 1;
}
/* }}} */

/** {{{ static int fnx_view_simple_extract(zval *tpl_vars, zval *vars TSRMLS_DC)
*/
static int fnx_view_simple_extract(zval *tpl_vars, zval *vars TSRMLS_DC) {
	zval **entry;
	char *var_name;
	long num_key;
	uint var_name_len;
	HashPosition pos;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
	if (!EG(active_symbol_table)) {
		/*zend_rebuild_symbol_table(TSRMLS_C);*/
		return 1;
	}
#endif

	if (tpl_vars && Z_TYPE_P(tpl_vars) == IS_ARRAY) {
		for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(tpl_vars), &pos);
				zend_hash_get_current_data_ex(Z_ARRVAL_P(tpl_vars), (void **)&entry, &pos) == SUCCESS;
				zend_hash_move_forward_ex(Z_ARRVAL_P(tpl_vars), &pos)) {
			if (zend_hash_get_current_key_ex(Z_ARRVAL_P(tpl_vars), &var_name, &var_name_len, &num_key, 0, &pos) != HASH_KEY_IS_STRING) {
				continue;
			}

			/* GLOBALS protection */
			if (var_name_len == sizeof("GLOBALS") && !strcmp(var_name, "GLOBALS")) {
				continue;
			}

			if (var_name_len == sizeof("this")  && !strcmp(var_name, "this") && EG(scope) && EG(scope)->name_length != 0) {
				continue;
			}


			if (fnx_view_simple_valid_var_name(var_name, var_name_len - 1)) {
				ZEND_SET_SYMBOL_WITH_LENGTH(EG(active_symbol_table), var_name, var_name_len,
						*entry, Z_REFCOUNT_P(*entry) + 1, PZVAL_IS_REF(*entry));
			}
		}
	}

	if (vars && Z_TYPE_P(vars) == IS_ARRAY) {
		for(zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(vars), &pos);
				zend_hash_get_current_data_ex(Z_ARRVAL_P(vars), (void **)&entry, &pos) == SUCCESS;
				zend_hash_move_forward_ex(Z_ARRVAL_P(vars), &pos)) {
			if (zend_hash_get_current_key_ex(Z_ARRVAL_P(vars), &var_name, &var_name_len, &num_key, 0, &pos) != HASH_KEY_IS_STRING) {
				continue;
			}

			/* GLOBALS protection */
			if (var_name_len == sizeof("GLOBALS") && !strcmp(var_name, "GLOBALS")) {
				continue;
			}

			if (var_name_len == sizeof("this")  && !strcmp(var_name, "this") && EG(scope) && EG(scope)->name_length != 0) {
				continue;
			}

			if (fnx_view_simple_valid_var_name(var_name, var_name_len - 1)) {
				ZEND_SET_SYMBOL_WITH_LENGTH(EG(active_symbol_table), var_name, var_name_len,
						*entry, Z_REFCOUNT_P(*entry) + 1, 0 /**PZVAL_IS_REF(*entry)*/);
			}
		}
	}

	return 1;
}
/* }}} */

/** {{{ fnx_view_t * fnx_view_simple_instance(fnx_view_t *view, zval *tpl_dir, zval *options TSRMLS_DC)
*/
fnx_view_t * fnx_view_simple_instance(fnx_view_t *view, zval *tpl_dir, zval *options TSRMLS_DC) {
	zval *instance, *tpl_vars;

	instance = view;
	if (!instance) {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, fnx_view_simple_ce);
	}

	MAKE_STD_ZVAL(tpl_vars);
	array_init(tpl_vars);
	zend_update_property(fnx_view_simple_ce, instance, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), tpl_vars TSRMLS_CC);
	zval_ptr_dtor(&tpl_vars);

	if (tpl_dir && Z_TYPE_P(tpl_dir) == IS_STRING && IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl_dir), Z_STRLEN_P(tpl_dir))) {
		zend_update_property(fnx_view_simple_ce, instance, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), tpl_dir TSRMLS_CC);
	} else {
		zend_update_property_stringl(fnx_view_simple_ce, instance, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), "", 0 TSRMLS_CC);
	}

	if (options && IS_ARRAY == Z_TYPE_P(options)) {
		zend_update_property(fnx_view_simple_ce, instance, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_OPTS), options TSRMLS_CC);
	}

	return instance;
}
/* }}} */

/** {{{ int fnx_view_simple_render(fnx_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC)
*/
int fnx_view_simple_render(fnx_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC) {
	zval *tpl_vars;
	char *script;
	uint len;
	HashTable *calling_symbol_table;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	zend_class_entry *old_scope;
	fnx_view_simple_buffer *buffer;
	zend_bool short_open_tag = 0;
#endif

	ZVAL_NULL(ret);

	tpl_vars = zend_read_property(fnx_view_simple_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (EG(active_symbol_table)) {
		calling_symbol_table = EG(active_symbol_table);
	} else {
		calling_symbol_table = NULL;
	}

	ALLOC_HASHTABLE(EG(active_symbol_table));
	zend_hash_init(EG(active_symbol_table), 0, NULL, ZVAL_PTR_DTOR, 0);

	(void)fnx_view_simple_extract(tpl_vars, vars TSRMLS_CC);

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	FNX_REDIRECT_OUTPUT_BUFFER(buffer);
	{
		zval **short_tag;
		zval *options = zend_read_property(fnx_view_simple_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_OPTS), 1 TSRMLS_CC);
		if (IS_ARRAY != Z_TYPE_P(options)
				|| (zend_hash_find(Z_ARRVAL_P(options), ZEND_STRS("short_tag"), (void **)&short_tag) == FAILURE)
				|| zend_is_true(*short_tag)) {
			short_open_tag = CG(short_tags);
			CG(short_tags) = 1;
		}
	}
#else
	if (php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS TSRMLS_CC) == FAILURE) {
		php_error_docref("ref.outcontrol" TSRMLS_CC, E_WARNING, "failed to create buffer");
		return 0;
	}
#endif

	if (IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl), Z_STRLEN_P(tpl))) {
		script 	= Z_STRVAL_P(tpl);
		len 	= Z_STRLEN_P(tpl);
		if (fnx_loader_compose(script, len + 1, 0 TSRMLS_CC) == 0) {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
			FNX_RESTORE_OUTPUT_BUFFER(buffer);
			CG(short_tags) = short_open_tag;
#else
			php_output_end(TSRMLS_C);
#endif
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}

			fnx_trigger_error(FNX_ERR_NOTFOUND_VIEW TSRMLS_CC, "Unable to find template %s", script);
			return 0;
		}
	} else {
		zval *tpl_dir = zend_read_property(fnx_view_simple_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), 1 TSRMLS_CC);

		if (ZVAL_IS_NULL(tpl_dir)) {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
			FNX_RESTORE_OUTPUT_BUFFER(buffer);
			CG(short_tags) = short_open_tag;
#else
			php_output_end(TSRMLS_C);
#endif

			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}

			fnx_trigger_error(FNX_ERR_NOTFOUND_VIEW TSRMLS_CC,
				   	"Could not determine the view script path, you should call %s::setScriptPath to specific it",
					fnx_view_simple_ce->name);
			return 0;
		}

		len = spprintf(&script, 0, "%s%c%s", Z_STRVAL_P(tpl_dir), DEFAULT_SLASH, Z_STRVAL_P(tpl));

		if (fnx_loader_compose(script, len + 1, 0 TSRMLS_CC) == 0) {
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
			FNX_RESTORE_OUTPUT_BUFFER(buffer);
			CG(short_tags) = short_open_tag;
#else
			php_output_end(TSRMLS_C);
#endif
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}

			fnx_trigger_error(FNX_ERR_NOTFOUND_VIEW TSRMLS_CC, "Unable to find template %s" , script);
			efree(script);
			return 0;
		}
		efree(script);
	}

	if (calling_symbol_table) {
		zend_hash_destroy(EG(active_symbol_table));
		FREE_HASHTABLE(EG(active_symbol_table));
		EG(active_symbol_table) = calling_symbol_table;
	}

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	CG(short_tags) = short_open_tag;
	if (buffer->len) {
		ZVAL_STRINGL(ret, buffer->buffer, buffer->len, 1);
	}
#else
	if (php_output_get_contents(ret TSRMLS_CC) == FAILURE) {
		php_output_end(TSRMLS_C);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to fetch ob content");
		return 0;
	}

	if (php_output_discard(TSRMLS_C) != SUCCESS ) {
		return 0;
	}
#endif

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	FNX_RESTORE_OUTPUT_BUFFER(buffer);
#endif
	return 1;
}
/* }}} */

/** {{{ int fnx_view_simple_display(fnx_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC)
*/
int fnx_view_simple_display(fnx_view_t *view, zval *tpl, zval *vars, zval *ret TSRMLS_DC) {
	zval *tpl_vars;
	char *script;
	uint len;
	zend_class_entry *old_scope;
	HashTable *calling_symbol_table;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	zend_bool short_open_tag = 0;
#endif

	ZVAL_NULL(ret);

	tpl_vars = zend_read_property(fnx_view_simple_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (EG(active_symbol_table)) {
		calling_symbol_table = EG(active_symbol_table);
	} else {
		calling_symbol_table = NULL;
	}

	ALLOC_HASHTABLE(EG(active_symbol_table));
	zend_hash_init(EG(active_symbol_table), 0, NULL, ZVAL_PTR_DTOR, 0);

	(void)fnx_view_simple_extract(tpl_vars, vars TSRMLS_CC);

	old_scope = EG(scope);
	EG(scope) = fnx_view_simple_ce;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	{
		zval **short_tag;
		zval *options = zend_read_property(fnx_view_simple_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_OPTS), 1 TSRMLS_CC);
		if (IS_ARRAY != Z_TYPE_P(options)
				|| (zend_hash_find(Z_ARRVAL_P(options), ZEND_STRS("short_tag"), (void **)&short_tag) == FAILURE)
				|| zend_is_true(*short_tag)) {
			short_open_tag = CG(short_tags);
			CG(short_tags) = 1;
		}
	}
#endif

	if (IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl), Z_STRLEN_P(tpl))) {
		script 	= Z_STRVAL_P(tpl);
		len 	= Z_STRLEN_P(tpl);
		if (fnx_loader_compose(script, len + 1, 0 TSRMLS_CC) == 0) {
			fnx_trigger_error(FNX_ERR_NOTFOUND_VIEW TSRMLS_CC, "Unable to find template %s" , script);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
			CG(short_tags) = short_open_tag;
#endif
			EG(scope) = old_scope;
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}
			return 0;
		}
	} else {
		zval *tpl_dir = zend_read_property(fnx_view_simple_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), 1 TSRMLS_CC);

		if (ZVAL_IS_NULL(tpl_dir)) {
			fnx_trigger_error(FNX_ERR_NOTFOUND_VIEW TSRMLS_CC,
					"Could not determine the view script path, you should call %s::setScriptPath to specific it", fnx_view_simple_ce->name);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
			CG(short_tags) = short_open_tag;
#endif
			EG(scope) = old_scope;
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}
			return 0;
		}

		len = spprintf(&script, 0, "%s%c%s", Z_STRVAL_P(tpl_dir), DEFAULT_SLASH, Z_STRVAL_P(tpl));
		if (fnx_loader_compose(script, len + 1, 0 TSRMLS_CC) == 0) {
			fnx_trigger_error(FNX_ERR_NOTFOUND_VIEW TSRMLS_CC, "Unable to find template %s", script);
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
			CG(short_tags) = short_open_tag;
#endif
			efree(script);
			EG(scope) = old_scope;
			if (calling_symbol_table) {
				zend_hash_destroy(EG(active_symbol_table));
				FREE_HASHTABLE(EG(active_symbol_table));
				EG(active_symbol_table) = calling_symbol_table;
			}
			return 0;
		}
		efree(script);
	}

	EG(scope) = old_scope;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	CG(short_tags) = short_open_tag;
#endif
	if (calling_symbol_table) {
		zend_hash_destroy(EG(active_symbol_table));
		FREE_HASHTABLE(EG(active_symbol_table));
		EG(active_symbol_table) = calling_symbol_table;
	}

	return 1;
}
/* }}} */

/** {{{ int fnx_view_simple_eval(fnx_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC)
*/
int fnx_view_simple_eval(fnx_view_t *view, zval *tpl, zval * vars, zval *ret TSRMLS_DC) {
	zval *tpl_vars;
	char *script;
	uint len;
	HashTable *calling_symbol_table;
#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	zend_class_entry *old_scope;
	fnx_view_simple_buffer *buffer;
#endif

	ZVAL_NULL(ret);

	tpl_vars = zend_read_property(fnx_view_simple_ce, view, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (EG(active_symbol_table)) {
		calling_symbol_table = EG(active_symbol_table);
	} else {
		calling_symbol_table = NULL;
	}

	ALLOC_HASHTABLE(EG(active_symbol_table));
	zend_hash_init(EG(active_symbol_table), 0, NULL, ZVAL_PTR_DTOR, 0);

	(void)fnx_view_simple_extract(tpl_vars, vars TSRMLS_CC);

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	FNX_REDIRECT_OUTPUT_BUFFER(buffer);
#else
	if (php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS TSRMLS_CC) == FAILURE) {
		php_error_docref("ref.outcontrol" TSRMLS_CC, E_WARNING, "failed to create buffer");
		return 0;
	}
#endif

	if (Z_STRLEN_P(tpl)) {
		zend_op_array *new_op_array;
		char *eval_desc = zend_make_compiled_string_description("template code" TSRMLS_CC);
		new_op_array = zend_compile_string(tpl, eval_desc TSRMLS_CC);
		efree(eval_desc);

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
	}

	if (calling_symbol_table) {
		zend_hash_destroy(EG(active_symbol_table));
		FREE_HASHTABLE(EG(active_symbol_table));
		EG(active_symbol_table) = calling_symbol_table;
	}

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	if (buffer->len) {
		ZVAL_STRINGL(ret, buffer->buffer, buffer->len, 1);
	}
#else
	if (php_output_get_contents(ret TSRMLS_CC) == FAILURE) {
		php_output_end(TSRMLS_C);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to fetch ob content");
		return 0;
	}

	if (php_output_discard(TSRMLS_C) != SUCCESS ) {
		return 0;
	}
#endif

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 4))
	FNX_RESTORE_OUTPUT_BUFFER(buffer);
#endif
	return 1;
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::__construct(string $tpl_dir, array $options = NULL)
*/
PHP_METHOD(fnx_view_simple, __construct) {
	zval *tpl_dir, *options = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &tpl_dir, &options) == FAILURE) {
		return;
	}

	fnx_view_simple_instance(getThis(), tpl_dir, options TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::__isset($name)
*/
PHP_METHOD(fnx_view_simple, __isset) {
	char *name;
	uint len;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &len) == FAILURE) {
		return;
	} else {
		zval *tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
		RETURN_BOOL(zend_hash_exists(Z_ARRVAL_P(tpl_vars), name, len + 1));
	}
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::setScriptPath(string $tpl_dir)
*/
PHP_METHOD(fnx_view_simple, setScriptPath) {
	zval *tpl_dir;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &tpl_dir) == FAILURE) {
		return;
	}

	if (Z_TYPE_P(tpl_dir) == IS_STRING && IS_ABSOLUTE_PATH(Z_STRVAL_P(tpl_dir), Z_STRLEN_P(tpl_dir))) {
		zend_update_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), tpl_dir TSRMLS_CC);
		RETURN_ZVAL(getThis(), 1, 0);
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::getScriptPath(void)
*/
PHP_METHOD(fnx_view_simple, getScriptPath) {
	zval *tpl_dir = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR), 1 TSRMLS_CC);
	RETURN_ZVAL(tpl_dir, 1, 0);
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::compose(string $script, zval *args)
*/
PHP_METHOD(fnx_view_simple, compose) {
	char *script;
	uint len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &script, &len) == FAILURE) {
		return;
	}

	if (!len) {
		RETURN_FALSE;
	}

}
/* }}} */

/** {{{ proto public Fnx_View_Simple::assign(mixed $value, mixed $value = null)
*/
PHP_METHOD(fnx_view_simple, assign) {
	uint argc = ZEND_NUM_ARGS();
	zval *tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (argc == 1) {
		zval *value;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
			return;
		}

		if (Z_TYPE_P(value) == IS_ARRAY) {
			zend_hash_copy(Z_ARRVAL_P(tpl_vars), Z_ARRVAL_P(value), (copy_ctor_func_t) zval_add_ref, NULL, sizeof(zval *));
			RETURN_TRUE;
		}
		RETURN_FALSE;
	} else if (argc == 2) {
		zval *value;
		char *name;
		uint len;
		if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
			return;
		}

		Z_ADDREF_P(value);
		if (zend_hash_update(Z_ARRVAL_P(tpl_vars), name, len + 1, &value, sizeof(zval *), NULL) == SUCCESS) {
			RETURN_TRUE;
		}
	} else {
		WRONG_PARAM_COUNT;
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::assignRef(mixed $value, mixed $value)
*/
PHP_METHOD(fnx_view_simple, assignRef) {
	char * name; int len;
	zval * value, * tpl_vars;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &len, &value) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);

	Z_ADDREF_P(value);
	if (zend_hash_update(Z_ARRVAL_P(tpl_vars), name, len + 1, &value, sizeof(zval *), NULL) == SUCCESS) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::get($name)
*/
PHP_METHOD(fnx_view_simple, get) {
	char *name;
	uint len = 0;
	zval *tpl_vars, **ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE) {
		return;
	}

	tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);

	if (tpl_vars && Z_TYPE_P(tpl_vars) == IS_ARRAY) {
		if (len) {
			if (zend_hash_find(Z_ARRVAL_P(tpl_vars), name, len + 1, (void **) &ret) == SUCCESS) {
				RETURN_ZVAL(*ret, 1, 0);
			} 
		} else {
			RETURN_ZVAL(tpl_vars, 1, 0);
		}
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::render(string $tpl, array $vars = NULL)
*/
PHP_METHOD(fnx_view_simple, render) {
	zval *tpl, *vars = NULL, *tpl_vars;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &tpl, &vars) == FAILURE) {
		return;
	}

	tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (!fnx_view_simple_render(getThis(), tpl, vars, return_value TSRMLS_CC)) {
		RETURN_FALSE;
	}
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::eval(string $tpl_content, array $vars = NULL)
*/
PHP_METHOD(fnx_view_simple, eval) {
	zval *tpl, *vars = NULL, *tpl_vars;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &tpl, &vars) == FAILURE) {
		return;
	}

	tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (!fnx_view_simple_eval(getThis(), tpl, vars, return_value TSRMLS_CC)) {
		RETURN_FALSE;
	}
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::display(string $tpl, array $vars = NULL)
*/
PHP_METHOD(fnx_view_simple, display) {
	zval *tpl, *tpl_vars, *vars = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &tpl, &vars) == FAILURE) {
		return;
	}

	tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (!fnx_view_simple_display(getThis(), tpl, vars, return_value TSRMLS_CC)) {
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/** {{{ proto public Fnx_View_Simple::clear(string $name)
*/
PHP_METHOD(fnx_view_simple, clear) {
	char *name;
	zval *tpl_vars;
	uint len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &len) == FAILURE) {
		return;
	}

	tpl_vars = zend_read_property(fnx_view_simple_ce, getThis(), ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), 1 TSRMLS_CC);
	if (tpl_vars && Z_TYPE_P(tpl_vars) == IS_ARRAY) {
		if (len) {
			zend_symtable_del(Z_ARRVAL_P(tpl_vars), name, len + 1);
		} else {
			zend_hash_clean(Z_ARRVAL_P(tpl_vars));
		}
	} 
	RETURN_ZVAL(getThis(), 1, 0);
}
/* }}} */

/** {{{ fnx_view_simple_methods
*/
zend_function_entry fnx_view_simple_methods[] = {
	PHP_ME(fnx_view_simple, __construct, fnx_view_simple_construct_arginfo, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, __isset, fnx_view_simple_isset_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, get, fnx_view_simple_get_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, assign, fnx_view_assign_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, render, fnx_view_render_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, eval,  fnx_view_simple_eval_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, display, fnx_view_display_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, assignRef, fnx_view_simple_assign_by_ref_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, clear, fnx_view_simple_clear_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, setScriptPath, fnx_view_setpath_arginfo, ZEND_ACC_PUBLIC)
	PHP_ME(fnx_view_simple, getScriptPath, fnx_view_getpath_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_view_simple, __get, get, fnx_view_simple_get_arginfo, ZEND_ACC_PUBLIC)
	PHP_MALIAS(fnx_view_simple, __set, assign, fnx_view_assign_arginfo, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ FNX_STARTUP_FUNCTION
*/
FNX_STARTUP_FUNCTION(view_simple) {
	zend_class_entry ce;

	FNX_INIT_CLASS_ENTRY(ce, "Fnx_View_Classic", "Fnx\\View\\Classic", fnx_view_simple_methods);
	fnx_view_simple_ce = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);

	zend_declare_property_null(fnx_view_simple_ce, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLVARS), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_view_simple_ce, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_TPLDIR),  ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(fnx_view_simple_ce, ZEND_STRL(FNX_VIEW_PROPERTY_NAME_OPTS),  ZEND_ACC_PROTECTED TSRMLS_CC);

	fnx_view_simple_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;
	zend_class_implements(fnx_view_simple_ce TSRMLS_CC, 1, fnx_view_interface_ce);

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

