好的，以下是一个最小化的 PHP ".so" 扩展示例：

首先，创建一个包含以下内容的 `hello.c` 文件：

```c
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
```

此代码定义了一个名为 "hello" 的扩展，其中包含一个函数 "hello_world"，该函数在调用时向终端输出 "Hello, world!"。

接下来，使用下面的命令来编译该扩展：

```
phpize
./configure --enable-hello
make
```

这将生成一个名为 `hello.so` 的共享库文件。要启用该扩展，请将其复制到 PHP 的扩展目录中，并在 `php.ini` 文件中添加以下行：

```
extension=hello.so
```

现在，在 PHP 程序中调用 "hello_world" 函数时，应该能够看到 "Hello, world!" 的输出。

请注意，本示例仅供演示目的，并不包含任何错误检查或安全措施。在实际生产环境中，您应该谨慎处理和管理扩展的加载、配置和执行。



[//]: # (==========)
jieqiang@PC-202306191309: ~/lfdev/wwwroot/test/php_aho_corasick2/hello develop ⚡ $ make
test                                                                                                                       (
0s)[17:42:42]

Build complete.
Don't forget to run 'make test'.

=====================================================================
PHP         : /usr/bin/php8.1
PHP_SAPI    : cli
PHP_VERSION : 8.1.2-1ubuntu2.14
ZEND_VERSION: 4.1.2
PHP_OS      : Linux - Linux PC-202306191309 5.10.16.3-microsoft-standard-WSL2 #1 SMP Fri Apr 2 22:23:49 UTC 2021 x86_64
INI actual  : /home/jieqiang/lfdev/wwwroot/test/php_aho_corasick2/hello/tmp-php.ini
More .INIs  :
CWD         : /home/jieqiang/lfdev/wwwroot/test/php_aho_corasick2/hello
Extra dirs  :
VALGRIND    : Not used
=====================================================================
TIME START 2023-12-29 09:42:48
=====================================================================
No tests were run.


jieqiang@PC-202306191309: ~/lfdev/wwwroot/test/php_aho_corasick2/hello develop ⚡ $ phpize ./cionfig
--enable-hello                                                                                                 (
1s)[17:40:41]   
Configuring for:
PHP Api Version:         20210902
Zend Module Api No:      20210902
Zend Extension Api No:   420210902
configure.ac:22: warning: $as_echo is obsolete; use AS_ECHO(["message"]) instead
build/php.m4:2111: PHP_CONFIG_NICE is expanded from...
configure.ac:22: the top level
configure.ac:165: warning: The macro `AC_PROG_LIBTOOL' is obsolete.
configure.ac:165: You should run autoupdate.
build/libtool.m4:99: AC_PROG_LIBTOOL is expanded from...
configure.ac:165: the top level


jieqiang@PC-202306191309: ~/lfdev/wwwroot/test/php_aho_corasick2/hello develop ⚡ $ phpize                                                                                                                          (0s)[17:38:31]
Configuring for:
PHP Api Version:         20210902
configure.ac:22: the top level
configure.ac:165: warning: The macro `AC_PROG_LIBTOOL' is obsolete.
configure.ac:165: You should run autoupdate.
build/libtool.m4:99: AC_PROG_LIBTOOL is expanded from...
configure.ac:165: the top level
