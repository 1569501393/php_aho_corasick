PHP_ARG_ENABLE(hello_world, whether to enable the hello_world extension,
[ --enable-hello_world   Enable hello_world support])

if test "$PHP_HELLO_WORLD" != "no"; then
  PHP_NEW_EXTENSION(hello_world, php_hello_world.c, $ext_shared)
fi
