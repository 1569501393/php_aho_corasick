PHP_ARG_ENABLE(hello, whether to enable the hello extension,
[  --enable-hello        Enable hello extension support])

if test $PHP_HELLO != "no"; then
  PHP_NEW_EXTENSION(hello, hello.c, $ext_shared)
fi
