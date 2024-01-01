#include "php.h"

PHP_FUNCTION(simple_add)
{
    long a, b, result;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &a, &b) == FAILURE) {
        return;
    }

    result = a + b;

    RETURN_LONG(result);
}

ZEND_BEGIN_ARG_INFO(arginfo_simple_add, 0)
    ZEND_ARG_INFO(0, a)
    ZEND_ARG_INFO(0, b)
ZEND_END_ARG_INFO()

const zend_function_entry simple_add_functions[] = {
    PHP_FE(simple_add, arginfo_simple_add)
    PHP_FE_END
};

zend_module_entry simple_add_module_entry = {
    STANDARD_MODULE_HEADER,
    "simple_add",
    simple_add_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_SIMPLE_ADD
ZEND_GET_MODULE(simple_add)
#endif
