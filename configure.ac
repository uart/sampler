AC_INIT([sampler],[1.0.0], [andreas.sandberg@it.uu.se])
AC_PREREQ(2.59)
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CC_STDC

AC_CHECK_HEADERS([uart/usf.h], 
  [ true ],
  [ AC_MSG_ERROR([Usable USF header files not found. Please install USF.])])

# USF might need BZip2, check if it is available and assume that USF
# depends on BZip2 if it exists.
AC_CHECK_LIB([bz2], [BZ2_bzReadOpen],
  [ BZ2_LIBS="-lbz2" ],
  [ BZ2_LIBS="" ])
AC_CHECK_LIB([usf], [usf_open],
  [ true ],
  [ AC_MSG_ERROR([Usable USF library files not found. Please install USF.]) ],
  [ $BZ2_LIBS ])

AC_ARG_ENABLE([strict],
  AS_HELP_STRING([--disable-strict],
    [Disable strict compile time checks.]),
  [
    true
  ], [
    CFLAGS="-Werror ${CFLAGS}"
  ])

AC_MSG_CHECKING([for PIN])
AC_ARG_VAR([PIN_HOME], [ Path to PIN toolkit ])
if [ test "$PIN_HOME" != "" ] && \
   [ test -e ${PIN_HOME}/source/tools/Config/makefile.config ]; then

  have_pin=true
  AC_MSG_RESULT([ yes ($PIN_HOME) ])
else
  AC_MSG_RESULT([no])
fi
AM_CONDITIONAL([HAVE_PIN], [ test x$have_pin = xtrue ])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
	Makefile

	include/Makefile
	include/uart/Makefile

	lib/Makefile

	tools/Makefile

	pin/Makefile
	pin/makefile.rules
])

AC_OUTPUT
