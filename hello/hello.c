#include "php.h"

static function_entry hello_functions[] = {
    PHP_FE(hello_world, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry hello_module_entry = {
    STANDARD_MODULE_HEADER,
    "hello",
    hello_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "1.0",
    STANDARD_MODULE_PROPERTIES
};

ZEND_FUNCTION(hello_world)
{
    php_printf("Hello, world!\n");
}

ZEND_GET_MODULE(hello)
