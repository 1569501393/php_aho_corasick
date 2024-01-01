PHP_ARG_ENABLE(simple_add, whether to enable the simple_add extension,
[  --enable-simple_add        Enable simple_add extension support])

PHP_NEW_EXTENSION(simple_add, simple_add.c, $ext_shared)

if test $PHP_SIMPLE_ADD != "no"; then
  # PHP_REQUIRE_CXX()
  # PHP_SUBST(SIMPLE_ADD_SHARED_LIBADD)
  PHP_NEW_EXTENSION(simple_add, simple_add.c, $ext_shared)
fi
