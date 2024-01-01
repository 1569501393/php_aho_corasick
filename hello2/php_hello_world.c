#include "php_hello_world.h"
#include <php.h>


PHP_FUNCTION(hello_world)
{
    php_printf("Hello, World!\n");
}

zend_function_entry hello_world_functions[] = {
    PHP_FE(hello_world, NULL)
    {NULL, NULL, NULL}
};

zend_module_entry hello_world_module_entry = {
    STANDARD_MODULE_HEADER,
    "hello_world",
    hello_world_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NO_VERSION_YET,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_HELLO_WORLD
ZEND_GET_MODULE(hello_world)
#endif
