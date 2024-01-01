#ifndef PHP_HELLO_WORLD_H
#define PHP_HELLO_WORLD_H

//extern zend_module_entry hello_world_module_entry;
#define phpext_hello_world_ptr &hello_world_module_entry

PHP_FUNCTION(hello_world);

#endif /* PHP_HELLO_WORLD_H */
