当然，下面是一个最简单的 PHP C 扩展示例，它将实现一个名为 `hello_world` 的函数，用于在 PHP 中输出 "Hello, World!"。

1. 创建一个新的文件夹，并在其中创建以下文件：

- `config.m4`：配置文件
- `php_hello_world.h`：头文件
- `php_hello_world.c`：源代码文件

2. 在 `config.m4` 中添加以下内容：

```
PHP_ARG_ENABLE(hello_world, whether to enable the hello_world extension,
[ --enable-hello_world   Enable hello_world support])

if test "$PHP_HELLO_WORLD" != "no"; then
  PHP_NEW_EXTENSION(hello_world, php_hello_world.c, $ext_shared)
fi
```

3. 在 `php_hello_world.h` 中添加以下内容：

```c
#ifndef PHP_HELLO_WORLD_H
#define PHP_HELLO_WORLD_H

extern zend_module_entry hello_world_module_entry;
#define phpext_hello_world_ptr &hello_world_module_entry

PHP_FUNCTION(hello_world);

#endif /* PHP_HELLO_WORLD_H */
```

4. 在 `php_hello_world.c` 中添加以下内容：

```c
#include "php_hello_world.h"

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
```

5. 打开终端，进入扩展文件夹，并执行以下命令：

```
phpize
./configure --enable-hello_world
make
```

6. 编译完成后，会生成一个名为 `hello_world.so` 的共享库文件。

7. 打开您的 PHP 配置文件 `php.ini`，添加以下内容：

```
extension=hello_world.so
```

8. 重启您的 PHP 服务器。

9. 创建一个简单的 PHP 脚本来测试扩展功能：

```php
<?php
hello_world();
?>
```

10. 运行该脚本，您将在输出中看到 "Hello, World!"。

这是一个非常基本的示例，用于演示如何创建一个简单的 PHP C 扩展。您可以根据自己的需求扩展和修改代码。

希望这个示例对您有所帮助！如果您有更多问题，请随时提问。
