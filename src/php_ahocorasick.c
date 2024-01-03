/*
 * php_ahocorasick.c: PHP Aho Corasick extension file
 *
    Copyright 2010-2013 Ph4r05 <ph4r05@gmail.com>

    This software is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this software.  If not, see <http://www.gnu.org/licenses/>.
  
    This code uses thirdparty code:
        MultiFast (http://sourceforge.net/projects/multifast/?source=dlp)
*/

/**
 * Sources:
 *  http://www.phpinternalsbook.com/zvals/memory_management.html
 *  http://docstore.mik.ua/orelly/webprog/php/ch14_06.htm
 *
 *  https://wiki.php.net/phpng-upgrading
 *  https://phpinternals.net/docs/zval_copy
 *  https://nikic.github.io/2015/05/05/Internal-value-representation-in-PHP-7-part-1.html
 *  https://nikic.github.io/2015/06/19/Internal-value-representation-in-PHP-7-part-2.html
 *  https://github.com/php/php-src/blob/c8efaea1e3f93b5b836a38c6985b67983b1dc95a/Zend/zend_types.h#L364
 *  https://github.com/php/php-src/blob/master/Zend/zend_types.h
 *  https://github.com/copentop/php-7.2.5/blob/924c4cd50b01efebbb93438392c9e1916a568f02/php-7.2.5/ext/sodium/libsodium.c
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_ahocorasick.h"
#include "ext/standard/php_string.h"
#include "php_variables.h"
#include "php_globals.h"
#include "TSRM.h"
#include "zend_exceptions.h"
#include <stdio.h>
#include <stdlib.h>

/* For PHP 8 compatibility */
#ifndef TSRMLS_CC
#define TSRMLS_CC
#endif
#ifndef TSRMLS_DC
#define TSRMLS_DC
#endif

// counter for aho struct resources
int le_ahocorasick_master;
#if PHP7
static zend_class_entry *aho_exception_ce;
#endif
static char exception_buffer[8192];


ZEND_DECLARE_MODULE_GLOBALS(ahocorasick)

#if PHP_VERSION_ID < 80000

#include "php_ahocorasick_legacy_arginfo.h"

#else
#include "php_ahocorasick_arginfo.h"
#endif

        zend_module_entry ahocorasick_module_entry = {
                STANDARD_MODULE_HEADER,
                PHP_AHOCORASICK_EXTNAME,
                ext_functions,
                PHP_MINIT(ahocorasick),
                PHP_MSHUTDOWN(ahocorasick),
                PHP_RINIT(ahocorasick),
                NULL,
                NULL,
                PHP_AHOCORASICK_VERSION,
                STANDARD_MODULE_PROPERTIES
        };

#ifdef COMPILE_DL_AHOCORASICK
        ZEND_GET_MODULE(ahocorasick)
#endif

//PHP_INI_BEGIN()
        //PHP_INI_ENTRY("ahocorasick.greeting", "Hello World", PHP_INI_ALL, NULL)
        //STD_PHP_INI_ENTRY("helloahocorasick.direction", "1", PHP_INI_ALL, OnUpdateBool, direction, zend_ahocorasick_globals, ahocorasick_globals)
//PHP_INI_END()


        static char *php_aho_type_str(int tp){
    static char typebuff[128];
    switch (tp) {
#if PHP7
        case IS_UNDEF:
            return "undef";
        case IS_NULL:
            return "null";
        case IS_FALSE:
            return "false";
        case IS_TRUE:
            return "true";
        case IS_LONG:
            return "long";
        case IS_DOUBLE:
            return "double";
        case IS_STRING:
            return "string";
        case IS_ARRAY:
            return "array";
        case IS_OBJECT:
            return "object";
        case IS_RESOURCE:
            return "resource";
        case IS_REFERENCE:
            return "reference";
#endif
        default:
            sprintf(typebuff, "%d", tp);
            return typebuff;
    }
}

/**
 * register some global variables here
 * @param ahocorasick_globals
static void php_ahocorasick_init_globals(zend_ahocorasick_globals *ahocorasick_globals)
{
        return;
}
 */

/**
 * Finalizes searching trie if it was not finalized.
 */
static inline int php_ahocorasick_finalize(ahocorasick_master_t *ahoMaster) {
    if (ahoMaster == NULL
        || ahoMaster->init_ok != 1
        || ahoMaster->ac_finalized == 1) {
        return 0;
    }

    ahoMaster->ac_finalized = 1;
    //*** 5. Finalize automata (no more patterns will be added).
    ac_trie_finalize(ahoMaster->acap);
    return 1;
}

/**
 * Resets all pattern fields. Does not perform any deallocation.
 */
static inline int php_ahocorasick_reset_pattern(ahocorasick_pattern_t *tmpStruct) {
    if (tmpStruct == NULL) {
        return -1;
    }

    tmpStruct->ignoreCase = 0;
    tmpStruct->key = NULL;
    COMPAT_ZVAL_UNDEF(tmpStruct->zKey);
    tmpStruct->keyId = 0;
    tmpStruct->keyType = AC_PATTID_TYPE_DEFAULT;
    tmpStruct->value = NULL;
    COMPAT_ZVAL_UNDEF(tmpStruct->zVal);
    COMPAT_ZVAL_UNDEF(tmpStruct->auxObj);
    return 0;
}

/**
 * Deallocates all memory related to the given pattern.
 */
static inline int php_ahocorasick_dealloc_pattern(ahocorasick_pattern_t *tmpStruct) {
    if (tmpStruct == NULL) {
        return -1;
    }

    if (!COMPAT_Z_ISUNDEF(tmpStruct->auxObj)) {
        zval_ptr_dtor(&(tmpStruct->auxObj));
        COMPAT_ZVAL_UNDEF(tmpStruct->auxObj);
    }

    if (tmpStruct->key != NULL && !COMPAT_Z_ISUNDEF(tmpStruct->zKey)) {
        zval_ptr_dtor(&(tmpStruct->zKey));
        tmpStruct->key = NULL;
        COMPAT_ZVAL_UNDEF(tmpStruct->zKey);
    }

    if (tmpStruct->value != NULL && !COMPAT_Z_ISUNDEF(tmpStruct->zVal)) {
        zval_ptr_dtor(&(tmpStruct->zVal));
        tmpStruct->value = NULL;
        COMPAT_ZVAL_UNDEF(tmpStruct->zVal);
    }

    php_ahocorasick_reset_pattern(tmpStruct);
    return 0;
}

/**
 * Reads single pattern definition, construct ahocorasick_pattern_t representation of pattern.
 */
static inline int
php_ahocorasick_process_pattern(zend_long pidx, ahocorasick_pattern_t *tmpStruct, HashTable *arr_hash_sub TSRMLS_DC) {
    php_ahocorasick_reset_pattern(tmpStruct);

    // iterate over sub array
    int returnCode = 0;
    unsigned long allKeys = 0;
    COMPAT_ZVAL *data_sub;
    zend_long num_key;
    zend_string *key;
    zend_bool has_exception = 0;

#define PATTERN_EXCEPTION() do{  \
    has_exception=1;   \
    returnCode=-5;     \
} while(0); break

    COMPAT_ZEND_HASH_FOREACH_KEY_VAL(arr_hash_sub, num_key, key, data_sub)
    {
        (void) num_key;
        unsigned long keyFound = 0;
        if (returnCode != 0 || has_exception) {
            break;
        }
#if !PHP7
        unsigned int key_len;
        unsigned long index;
        if (zend_hash_get_current_key_ex(arr_hash_sub, &key, &key_len, &index, 0, &pointer) != HASH_KEY_IS_STRING) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid structure (bad sub-array key)! Cannot initialize."
                                                        "Pattern idx: %ld", (long)pidx);
            returnCode = -1;
            break;
        }
#endif

        // determine known keys
        if (!key) {
            keyFound |= 2;
        } else if (COMPAT_STR_EQUALS_CI("key", key)) {
            keyFound |= 1;
        } else if (COMPAT_STR_EQUALS_CI("value", key)) {
            keyFound |= 2;
        } else if (COMPAT_STR_EQUALS_CI("ignoreCase", key)) {
            keyFound |= 4;
        } else if (COMPAT_STR_EQUALS_CI("id", key)) {
            keyFound |= 8;
        } else if (COMPAT_STR_EQUALS_CI("aux", key)) {
            keyFound |= 0x10;
        } else {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                             "Invalid structure (unrecognized sub-array key)! "
                             "Only allowed are: {key, id, value, aux, ignoreCase}. Cannot initialize. "
                             "Pattern index: %ld", (long) pidx);
            returnCode = -2;
            break;
        }
        allKeys |= keyFound;

        // Numeric identifier
        if ((keyFound & 0x8) > 0) {
            if (COMPAT_Z_TYPE_P(*data_sub) != IS_LONG) {
                sprintf(exception_buffer,
                        "Invalid type of pattern ID given (long required), type: %s, pattern index: %ld",
                        php_aho_type_str(COMPAT_Z_TYPE_P(*data_sub)), (long) pidx);
                PATTERN_EXCEPTION();
            }

            long keyId = COMPAT_Z_LVAL(*data_sub);
            tmpStruct->keyId = keyId;
            tmpStruct->keyType = AC_PATTID_TYPE_NUMBER;
        }

        // Aux object
        if ((keyFound & 0x10) > 0) {
            // No copying using same reference.
            tmpStruct->auxObj = *data_sub;
            COMPAT_ZVAL_COPY(&(tmpStruct->auxObj), data_sub);  // soft-copy
        }

        // ignoreCase - deprecated.
        if ((keyFound & 0x4) > 0) {
            tmpStruct->ignoreCase = 0;
        }

        // key/value present -> process
        if ((keyFound & 0x3) > 0) {
            char *stmp = NULL;

            if (COMPAT_Z_TYPE_P(*data_sub) != IS_STRING) {
                sprintf(exception_buffer, "Pattern %s has to be a string, type: %s, pattern index: %ld",
                        keyFound == 0x1 ? "key" : "value",
                        php_aho_type_str(COMPAT_Z_TYPE_P(*data_sub)),
                        (long) pidx);
                PATTERN_EXCEPTION();
            }

            // Avoid string copy, use reference counting.
            stmp = COMPAT_Z_STRVAL(*data_sub);
            if (keyFound == 0x1) {
                COMPAT_ZVAL_COPY(&(tmpStruct->zKey), data_sub);  // soft-copy
                tmpStruct->key = stmp;
                tmpStruct->keyType = AC_PATTID_TYPE_STRING;
            } else if (keyFound == 0x2) {
                // value
                COMPAT_ZVAL_COPY(&(tmpStruct->zVal), data_sub);  // soft-copy
                tmpStruct->value = stmp;
                tmpStruct->valueLen = COMPAT_Z_STRLEN(*data_sub);
            }
        }
    }
    COMPAT_ZEND_HASH_FOREACH_END();

    // sanity check, if failed, return false
    if (returnCode == 0 && tmpStruct->value == NULL) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "No value was specified for pattern index: %ld", (long) pidx);
        returnCode = -2;
    }

    // numeric key and string identifier are mutually exclusive
    if (returnCode == 0 && (allKeys & 0x1) && (allKeys & 0x8)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
                         "Pattern can have either numeric or string identifier, not both! Pattern index: %ld",
                         (long) pidx);
        returnCode = -3;
    }

    // Deprecate ignoreCase option
    if (allKeys & 0x4) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
                         "ignoreCase attribute is deprecated and is ignored. Pattern index: %ld", (long) pidx);
    }

    // If everything went well, we can return successfully.
    if (returnCode == 0) {
        return 0;
    }

    // Otherwise deallocate this entry.
    php_ahocorasick_dealloc_pattern(tmpStruct);
    if (has_exception) {
#if PHP7
        zend_throw_exception(aho_exception_ce, exception_buffer, 0 TSRMLS_CC);
#else
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "%s", exception_buffer);
#endif
    }
    return returnCode;
#undef PATTERN_EXCEPTION
}

/**
 * Adds the given list to the pattern list
 */
static inline int php_ahocorasick_add_patterns(ahocorasick_master_t *master, ahocorasick_pattern_t *tmpStruct,
                                               ahocorasick_pattern_t *tmpStructLast, long sublistSize) {
    if (master == NULL || tmpStruct == NULL) {
        return -1;
    }

    tmpStruct->prev = NULL;
    tmpStructLast->next = master->patterns;

    if (master->patterns) {
        master->patterns->prev = tmpStructLast;
    }
    master->patterns = tmpStruct;
    master->pattern_count += sublistSize;
    return 0;
}

/**
 * Adds given pattern to the doubly linked list. Does not copy memory, embbeds given structure directly to the list.
 */
static inline int php_ahocorasick_add_pattern(ahocorasick_master_t *master, ahocorasick_pattern_t *tmpStruct) {
    return php_ahocorasick_add_patterns(master, tmpStruct, tmpStruct, 1);
}

/**
 * Releases all associated memory in linked list of patterns
 */
static inline int php_ahocorasick_release_patterns(ahocorasick_master_t *master) {
    if (master == NULL) {
        return -1;
    }

    ahocorasick_pattern_t *p0 = master->patterns;
    while (p0) {
        ahocorasick_pattern_t *next = p0->next;
        php_ahocorasick_dealloc_pattern(p0);
        efree(p0);
        p0 = next;
    }

    master->patterns = NULL;
    master->pattern_count = 0;

    return 0;
}

/**
 * Reads array of patterns, adds them to the search trie.
 */
static inline int php_ahocorasick_process_patterns(ahocorasick_master_t *master, HashTable *arr_hash TSRMLS_DC) {
    int pattern_processing_status = 0;
    COMPAT_ZVAL *data;
    zend_long curIdx = 0;
    zend_string *key;
    ahocorasick_pattern_t *p0 = NULL;
    ahocorasick_pattern_t *p1 = NULL;
    ahocorasick_pattern_t *prevPattern = NULL;
    ahocorasick_pattern_t *lastPattern = NULL;

    // iterate input initialized array
    COMPAT_ZEND_HASH_FOREACH_KEY_VAL(arr_hash, curIdx, key, data)
    {
        (void) key;
        // check structure
        if (COMPAT_Z_TYPE_PP(data) != IS_ARRAY) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid pattern structure! Cannot initialize.");
            pattern_processing_status = -4;
            break;
        }

        // now we know that element is another array - iterate over it again and gain needed info
        ahocorasick_pattern_t *tmpStruct = (ahocorasick_pattern_t *) emalloc(sizeof(ahocorasick_pattern_t));
        if (curIdx == 0) {
            lastPattern = tmpStruct;
        }

        // Construct as a doubly linked list.
        tmpStruct->prev = NULL;
        tmpStruct->next = prevPattern;
        if (prevPattern) {
            prevPattern->prev = tmpStruct;
        }
        prevPattern = tmpStruct;

        // iterate over sub array
        HashTable *arr_hash_sub = COMPAT_Z_ARRVAL_P(data);
        int status_code = php_ahocorasick_process_pattern(curIdx, tmpStruct, arr_hash_sub TSRMLS_CC);
        if (status_code != 0) {
            pattern_processing_status = -1;
            break;
        }
    }
    COMPAT_ZEND_HASH_FOREACH_END();

    // if processing failed, free memory.
    if (pattern_processing_status != 0) {
        p0 = prevPattern;
        while (p0) {
            p1 = p0->next;
            php_ahocorasick_dealloc_pattern(p0);
            efree(p0);
            p0 = p1;
        }

        return pattern_processing_status;
    }

    // Nothing to process.
    if (prevPattern == NULL) {
        return 0;
    }

    //
    // now is everything OK (input data parsed properly) -> initialize AHO automata
    //

    // Add all patterns at once to the internal data structures
    php_ahocorasick_add_patterns(master, prevPattern, lastPattern, curIdx);

    p0 = prevPattern;
    while (p0) {
        AC_PATTERN_t tmp_patt;
        p1 = p0->next;

        // Construct search pattern for AhoCorasick library.
        // search string
        tmp_patt.ptext.astring = p0->value;
        tmp_patt.ptext.length = p0->valueLen;

        //The replacement pattern is not applicable in this program, so better
        //to initialize it with 0/
        tmp_patt.rtext.astring = NULL;
        tmp_patt.rtext.length = 0;

        // search value key
        tmp_patt.id.type = p0->keyType;
        if (p0->keyType == AC_PATTID_TYPE_NUMBER) {
            tmp_patt.id.u.number = p0->keyId;
        } else if (p0->keyType == AC_PATTID_TYPE_STRING) {
            tmp_patt.id.u.stringy = p0->key;
        }

        // Aux object holds the whole pattern in our representation.
        tmp_patt.aux = (void *) p0;

        // add this pattern to trie. copy pattern to internal memory.
        ac_trie_add(master->acap, &tmp_patt, 1);
        p0 = p1;
    }

    return pattern_processing_status;
}

/**
 * Destructor of ahocorasick_pattern_t resource
 */
static void php_ahocorasick_pattern_t_master_dtor(COMPAT_RESOURCE_PARAM(rsrc)) {
    ahocorasick_master_t *aho = (ahocorasick_master_t *) rsrc->ptr;
    if (aho == NULL) {
        return;
    }

    if (aho->patterns != NULL) {
        // release automata here
        if (aho->acap != NULL) {
            ac_trie_release(aho->acap);
        }

        php_ahocorasick_release_patterns(aho);
    }

    // release holder structure
    efree(aho);
}

#ifdef AHOCORASICK_USE_LOWER
/**
 * Invokes PHP function that converts string to lower case.
 * Calling mb_strtolower didn't work properly because of unavailability
 * of php_unicode.h, thus we are using call_user_function.
 *
 * Calls PHP function mb_strtolower from user space
 */
static char * php_ahocorasick_mb_strtolower(char * input TSRMLS_DC){
    zval ret, function_name, *params[1];

    // construct function to call
    COMPAT_ZVAL_STRING(&function_name, "mb_strtolower");
    // construct parameter to pass to target function
    COMPAT_MAKE_STD_ZVAL(params[0]);
    COMPAT_ZVAL_STRING(params[0], input); // TODO, 1?
    // invoke target function
    if (call_user_function(EG(function_table), NULL, &function_name, &ret, 1, params TSRMLS_CC) == SUCCESS){
        return Z_STRVAL(ret);
    } else {
        return NULL;
    }
}
#endif


//TODO 匹配处理器
/**
 * AhoCorasick callback handler - MATCH_CALBACK_t type
 */
static int php_ahocorasick_match_handler(AC_MATCH_t *m, void *param) {
    // variable to hold sub array - one found result
    COMPAT_ZVAL mysubarray;
    unsigned int j;

    /* example of sending parameter to call-back function */
    struct ahocorasick_callback_payload_t *myp = (struct ahocorasick_callback_payload_t *) param;
    if (COMPAT_Z_ISUNDEF(myp->resultArray)) {
        // invalid condition - result array not initialized
        return 0;
    }

//    sprintf
//    (myp->resultArray->val, "%s%s", myp->resultArray->val, m->patterns[j].key
//    );

    // PIxU-1
//    printf("m=%s, m=%s, m=%s, param=%s, param=%s, size=%d\n", m, &m, *m, param, &param, *param, m->size);
//    printf("m=%s, m=%s, m=%s, param=%s, param=%s, size=%d\n", m, &m, *m, param, &param, *param, m->size);

//    printf("m=%s, m=%s, m=%s, param=%s, param=%s, size=%d\n", m, &m, *m, param, &param, &param, m->size);


    printf("m=%p, param=%p\n", (void *) m, param);
    printf("*m=%ld, *param=%s\n", m->size, (char *) param);

    for (j = 0; j < m->size; j++) {
        // dump found matches to result array
        ahocorasick_pattern_t *curPattern = (ahocorasick_pattern_t *) m->patterns[j].aux;
        if (curPattern == NULL) {
            continue;
        }

        // TODO 判断前后字符是否为标点符号或者空格，如果不是，则跳过
//        printf("pos=%ld, start_postion=%ld, value=%s\n", m->position,
//               (m->position - COMPAT_Z_STRLEN_PP(COMPAT_Z_ARREF(curPattern->zVal))), curPattern->zVal);

        // TODO 判断前后字符是否为标点符号或者空格
//        if (m->position > 0 && m->position < COMPAT_Z_STRLEN_PP(z)){
//            zval * prevChar = &COMPAT_Z_STRVAL_PP(z)[m->position - 1];
//            zval * nextChar = &COMPAT_Z_STRVAL_PP(z)[m->position + COMPAT_Z_STRLEN_PP(COMPAT_Z_ARREF(curPattern->zVal))];
//            if (Z_TYPE_PP(prevChar) == IS_STRING && Z_TYPE_PP(nextChar) == IS_STRING){
//            }
//        }

        COMPAT_ALLOC_INIT_ZVAL(mysubarray);
        array_init(COMPAT_Z_ARREF(mysubarray));
        add_assoc_long(COMPAT_Z_ARREF(mysubarray), "pos", m->position);

        if (m->patterns[j].id.type == AC_PATTID_TYPE_STRING) {
            COMPAT_ADD_ASSOC_ZVAL(mysubarray, "key", curPattern->zKey);

        } else if (m->patterns[j].id.type == AC_PATTID_TYPE_NUMBER) {
            add_assoc_long(COMPAT_Z_ARREF(mysubarray), "keyIdx", m->patterns[j].id.u.number);

        }

        if (!COMPAT_Z_ISUNDEF(curPattern->auxObj)) {
            COMPAT_ADD_ASSOC_ZVAL(mysubarray, "aux", curPattern->auxObj);
        }

        // add_assoc_long(COMPAT_Z_ARREF(mysubarray), "start_postion",
        //                (m->position - COMPAT_Z_STRLEN_PP(COMPAT_Z_ARREF(curPattern->zVal))));
        add_assoc_long(COMPAT_Z_ARREF(mysubarray), "start_position",
                       (m->position - COMPAT_Z_STRLEN_PP(COMPAT_Z_ARREF(curPattern->zVal))));

        COMPAT_ADD_ASSOC_ZVAL(mysubarray, "value", curPattern->zVal);

        // TODO 添加到聚合数组
        // add to aggregate array
        add_next_index_zval(COMPAT_Z_ARREF(myp->resultArray), COMPAT_Z_ARREF(mysubarray));
    }

    // return 1 if we want to find just first
    // to find all return 0
    return myp->retVal == 0 ? 0 : 1;
}

PHP_RINIT_FUNCTION(ahocorasick) {
        return SUCCESS;
}

PHP_MINIT_FUNCTION(ahocorasick) {
        // destruction of ahocorasick_pattern_t master
        le_ahocorasick_master = zend_register_list_destructors_ex(php_ahocorasick_pattern_t_master_dtor, NULL,
                                                                  PHP_AHOSTRUCT_MASTER_RES_NAME, module_number);

#if PHP7
        zend_class_entry ce;
        INIT_CLASS_ENTRY(ce, "AhoException", NULL);
        aho_exception_ce = zend_register_internal_class_ex(&ce, zend_ce_exception);
#endif

        //ZEND_INIT_MODULE_GLOBALS(ahocorasick, php_ahocorasick_init_globals, NULL);
        //REGISTER_INI_ENTRIES();
        return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(ahocorasick) {
        //UNREGISTER_INI_ENTRIES();
        return SUCCESS;
}

/**
 * Returns whether current AhoCorasick resource is valid
 * @param 
 * @return 
 */
PHP_FUNCTION(ahocorasick_isValid) {
        ahocorasick_master_t * ahoMaster = NULL;

#if PHP7
        zval *zid;
        ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(zid)
        ZEND_PARSE_PARAMETERS_END();

        if (Z_RES_TYPE_P(zid) != le_ahocorasick_master){
            RETURN_FALSE;
        }

        // fetch resource passed as parameter
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(Z_RES_P(zid), PHP_AHOSTRUCT_MASTER_RES_NAME, le_ahocorasick_master);

#else
        zval *zval_aho_master = NULL;
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zval_aho_master) == FAILURE) {
            RETURN_NULL();
        }

        // fetch resource passed as parameter
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(&zval_aho_master TSRMLS_CC, -1, NULL, NULL, 1, le_ahocorasick_master);
#endif

        if (ahoMaster==NULL || ahoMaster->init_ok != 1){
            RETURN_FALSE;
        } else {
            RETURN_TRUE;
        }
}

/**
 * Basic strtolower matcher.
 * Returns array of keys matched (strings).
 * 
 * @param 
 * @return 
 */
PHP_FUNCTION(ahocorasick_match) {
#ifdef AHOCORASICK_USE_LOWER
        char *lowered;
#endif
        char *normal;
//    TODO 匹配所有关键词
        zend_bool findAll = 1;
//    zend_bool findAll = 0;
        ahocorasick_master_t * ahoMaster = NULL;
        AC_TEXT_t tmp_text;

#if PHP7
        zend_string *uservar;
        zval       *zid;
        ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(uservar)
        Z_PARAM_RESOURCE(zid)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(findAll)
        ZEND_PARSE_PARAMETERS_END();
        // fetch resource passed as parameter
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(Z_RES_P(zid), PHP_AHOSTRUCT_MASTER_RES_NAME, le_ahocorasick_master);
#else
        zval *uservar=NULL;
        zval *zval_aho_master = NULL;
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zz|b", &uservar, &zval_aho_master, &findAll) == FAILURE) {
            RETURN_NULL();
        }

        // fetch resource passed as parameter
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(&zval_aho_master TSRMLS_CC, -1, NULL, NULL, 1, le_ahocorasick_master);
#endif

        if (ahoMaster==NULL){
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid resource.");
            RETURN_FALSE;
        }

        if (ahoMaster->init_ok != 1){
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not initialized.");
            RETURN_FALSE;
        }

        // finalize trie if not finalized already
        php_ahocorasick_finalize(ahoMaster);

        normal = COMPAT_Z_STRVAL_P(uservar);

        // prints to stdout
        // PHPWRITE(ZSTR_VAL(uservar), ZSTR_LEN(uservar));

#ifdef AHOCORASICK_USE_LOWER
        // at first, obtain also lower case variant
        // strtolower is disabled now, exact match is required
#if PHP7
        lowered = php_ahocorasick_mb_strtolower(Z_STR_P(uservar) TSRMLS_CC);
#else
        lowered = php_ahocorasick_mb_strtolower(Z_STRVAL_P(uservar) TSRMLS_CC);
#endif
        tmp_text.astring = lowered;
#else
        //*** 6. Set input text
        tmp_text.astring = normal;
#endif
        tmp_text.length = COMPAT_Z_STRLEN_P(uservar);

        /* Sending parameter to call-back function */
        // initialize return array
        array_init(return_value);
        struct ahocorasick_callback_payload_t my_param;
        my_param.retVal = 0;

#if PHP7
        my_param.resultArray = *return_value;
#else
        my_param.resultArray = return_value;
#endif

        // find all defined
        my_param.retVal = findAll ? 0:1;

        //*** 7. Do search
//    printf("TODO Searching...tmp_text=%s, ahoMaster=%s \n", tmp_text, tmp_text);

        // 将结构体成员转换为 char *
//                char *text_str = "";  // 假设这里是将 tmp_text 转换为字符串的代码
//    char *my_param = "";  // 假设这里是将 tmp_text 转换为字符串的代码
        // TODO Searching...tmp_text=Apple, I like Apples and Bananas
        printf("TODO Searching...tmp_text=%s \n", tmp_text.astring);
//                printf("TODO Searching...tmp_text=%s \n", tmp_text);

        printf("ahoMaster->acap: %p\n", ahoMaster->acap);
        // printf("tmp_text: %s\n", tmp_text);
        // printf("php_ahocorasick_match_handler: %p\n", php_ahocorasick_match_handler);
        // printf("php_ahocorasick_match_handler: %s\n", php_ahocorasick_match_handler);
        // printf("my_param: %p\n", my_param);
        // printf("my_param: %s\n", my_param);

        ac_trie_search(ahoMaster->acap, &tmp_text, 0, php_ahocorasick_match_handler, (void *)(&my_param));
}

/**
 * De-initializes AhoCorasick master resource
 * 
 * @param 
 * @return 
 */
PHP_FUNCTION(ahocorasick_deinit) {
        ahocorasick_master_t * ahoMaster = NULL;

#if PHP7
        zval *zid;
        ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(zid)
        ZEND_PARSE_PARAMETERS_END();

        if (Z_RES_TYPE_P(zid) != le_ahocorasick_master){
            RETURN_FALSE;
        }

        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(Z_RES_P(zid), PHP_AHOSTRUCT_MASTER_RES_NAME, le_ahocorasick_master);
#else
        zval *zval_aho_master;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zval_aho_master) == FAILURE) {
          RETURN_NULL();
        }
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(&zval_aho_master TSRMLS_CC, -1, NULL, NULL, 1, le_ahocorasick_master);
#endif

        if (ahoMaster==NULL){
            RETURN_FALSE;
        }

        php_ahocorasick_finalize(ahoMaster);
        ahoMaster->init_ok = 0;

#if PHP7
        zend_list_close(Z_RES_P(zid));
#else
        zend_list_delete(Z_LVAL_P(zval_aho_master));
#endif
        RETURN_TRUE;
}

/**
 * Initializes AhoCorasick search structure with passed data
 * @param 
 * @return
 */
PHP_FUNCTION(ahocorasick_init) {
        zval * arr;
        HashTable *arr_hash;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &arr) == FAILURE) {
            RETURN_NULL();
        }

        // Get matching patterns
        arr_hash = Z_ARRVAL_P(arr);

        // create resource in holder structure, fill with data, return
        ahocorasick_master_t * ahomaster = emalloc(sizeof(ahocorasick_master_t));
        ahomaster->acap = ac_trie_create();
        ahomaster->ac_finalized = 0;
        ahomaster->init_ok = 0;
        ahomaster->patterns = NULL;
        ahomaster->pattern_count = 0;

        int pattern_processing_status = php_ahocorasick_process_patterns(ahomaster, arr_hash TSRMLS_CC);
        if (pattern_processing_status != 0){
            php_ahocorasick_release_patterns(ahomaster);
            ac_trie_release(ahomaster->acap);
            efree(ahomaster);
            RETURN_FALSE;
        }

        // pass ACAP object - holding aho automaton
        ahomaster->init_ok = 1;
        // register this resource for ZEND engine
#if PHP7
        ZVAL_RES(return_value, zend_register_resource(ahomaster, le_ahocorasick_master));
#else
        ZEND_REGISTER_RESOURCE(return_value, ahomaster, le_ahocorasick_master);
#endif
        // ahocorasick_pattern_t build OK.
        // Keep in mind that we are not freeing strings allocated in memory, it is
        // still used internally in aho structure, this free is postponed to releasing
        // aho structure.
}

/**
 * Finalizes aho corasick search structure
 * @param
 * @return
 */
PHP_FUNCTION(ahocorasick_finalize) {
        ahocorasick_master_t * ahoMaster = NULL;

#if PHP7
        zval *zid;
        ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_RESOURCE(zid)
        ZEND_PARSE_PARAMETERS_END();
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(Z_RES_P(zid), PHP_AHOSTRUCT_MASTER_RES_NAME, le_ahocorasick_master);

#else
        zval *zval_aho_master;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &zval_aho_master) == FAILURE) {
            RETURN_NULL();
        }
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(&zval_aho_master TSRMLS_CC, -1, NULL, NULL, 1, le_ahocorasick_master);
#endif

        // fetch resource passed as parameter
        if (ahoMaster==NULL){
            RETURN_FALSE;
        } else {
            if (php_ahocorasick_finalize(ahoMaster)) {
                RETURN_TRUE;
            } else {
                RETURN_FALSE;
            }
        }
}

/**
 * Adds search patterns to the non-finalized search trie.
 * @param
 * @return
 */
PHP_FUNCTION(ahocorasick_add_patterns) {
        zval * arr;
        ahocorasick_master_t *ahoMaster = NULL;
        HashTable *arr_hash = NULL;

#if PHP7
        zval *zid;
        ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_RESOURCE(zid)
        Z_PARAM_ARRAY(arr)
        ZEND_PARSE_PARAMETERS_END();

        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(Z_RES_P(zid), PHP_AHOSTRUCT_MASTER_RES_NAME, le_ahocorasick_master);

#else
        zval *zval_aho_master;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "za", &zval_aho_master, &arr) == FAILURE) {
            RETURN_NULL();
        }

        // fetch resource passed as parameter
        ahoMaster = (ahocorasick_master_t*) zend_fetch_resource(&zval_aho_master TSRMLS_CC, -1, NULL, NULL, 1, le_ahocorasick_master);
#endif

        arr_hash = Z_ARRVAL_P(arr);
        if (ahoMaster == NULL || ahoMaster->init_ok != 1){
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot add a new pattern, not initialized");
            RETURN_FALSE;
        }

        if (ahoMaster->ac_finalized){
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot add a new pattern to finalized search structure");
            RETURN_FALSE;
        }

        int pattern_processing_status = php_ahocorasick_process_patterns(ahoMaster, arr_hash TSRMLS_CC);
        if (pattern_processing_status != 0){
            RETURN_FALSE;
        } else {
            RETURN_TRUE;
        }
}
