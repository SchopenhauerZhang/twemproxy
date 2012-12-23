# Define the package version numbers and the bug reporting address
m4_define([NC_MAJOR], 0)
m4_define([NC_MINOR], 2)
m4_define([NC_PATCH], 2)
m4_define([NC_BUGS], [manj@twitter.com])

# Initialize autoconf
AC_PREREQ([2.64])
AC_INIT([nutcracker], [NC_MAJOR.NC_MINOR.NC_PATCH], [NC_BUGS])
AC_CONFIG_SRCDIR([src/nc.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_MACRO_DIR([m4])

# Initialize automake
AM_INIT_AUTOMAKE([1.9 foreign])

# Define macro variables for the package version numbers
AC_DEFINE(NC_VERSION_MAJOR, NC_MAJOR, [Define the major version number])
AC_DEFINE(NC_VERSION_MINOR, NC_MINOR, [Define the minor version number])
AC_DEFINE(NC_VERSION_PATCH, NC_PATCH, [Define the patch version number])
AC_DEFINE(NC_VERSION_STRING, "NC_MAJOR.NC_MINOR.NC_PATCH", [Define the version string])

# Checks for language
AC_LANG([C])

# Checks for programs
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for typedefs, structures, and compiler characteristics
AC_C_INLINE
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INTMAX_T
AC_TYPE_INTPTR_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINTMAX_T
AC_TYPE_UINTPTR_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

AC_C_BIGENDIAN(
  [],
  [AC_DEFINE(HAVE_LITTLE_ENDIAN, 1, [Define to 1 if machine is little endian])],
  [AC_MSG_ERROR([endianess of this machine is unknown])],
  [AC_MSG_ERROR([universial endianess not supported])]
)

# Checks for header files
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([fcntl.h float.h limits.h stddef.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([inttypes.h stdint.h])
AC_CHECK_HEADERS([sys/ioctl.h sys/time.h sys/uio.h])
AC_CHECK_HEADERS([sys/socket.h sys/un.h netinet/in.h arpa/inet.h netdb.h])
AC_CHECK_HEADER([sys/epoll.h], AC_DEFINE([HAVE_EPOLL], , [for epoll support])) 
AC_CHECK_HEADER([sys/event.h], AC_DEFINE([HAVE_KQUEUE], , [for kqueue support])) 

haveepollsyscall=no
haveepoll=no
AC_CHECK_FUNCS(epoll_ctl, [haveepoll=yes], )
if test "x$haveepoll" = "xyes" ; then
	AC_DEFINE(HAVE_EPOLL, 1,
		[Define if your system supports the epoll system calls])
	needsignal=yes
fi
if test "x$ac_cv_header_sys_epoll_h" = "xyes"; then
	if test "x$haveepoll" = "xno" ; then
		AC_MSG_CHECKING(for epoll system call)
		AC_TRY_RUN(
#include <stdint.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/epoll.h>
#include <unistd.h>

int
epoll_create(int size)
{
	return (syscall(__NR_epoll_create, size));
}

int
main(int argc, char **argv)
{
	int epfd;

	epfd = epoll_create(256);
	exit (epfd == -1 ? 1 : 0);
}, [AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_EPOLL, 1,
	[Define if your system supports the epoll system calls])
    needsignal=yes
    have_epoll=yes
    AC_LIBOBJ(epoll_sub)
    ], AC_MSG_RESULT(no), AC_MSG_RESULT(no))
	fi
fi
AM_CONDITIONAL(EPOLL_BACKEND, [test "x$haveepoll" = "xyes"])

havekqueue=no
if test "x$ac_cv_header_sys_event_h" = "xyes"; then
	AC_CHECK_FUNCS(kqueue, [havekqueue=yes], )
	if test "x$havekqueue" = "xyes" ; then
		AC_MSG_CHECKING(for working kqueue)
		AC_TRY_RUN(
#include <sys/types.h>
#include <sys/time.h>
#include <sys/event.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int
main(int argc, char **argv)
{
	int kq;
	int n;
	int fd[[2]];
	struct kevent ev;
	struct timespec ts;
	char buf[[8000]];

	if (pipe(fd) == -1)
		exit(1);
	if (fcntl(fd[[1]], F_SETFL, O_NONBLOCK) == -1)
		exit(1);

	while ((n = write(fd[[1]], buf, sizeof(buf))) == sizeof(buf))
		;

        if ((kq = kqueue()) == -1)
		exit(1);

	memset(&ev, 0, sizeof(ev));
	ev.ident = fd[[1]];
	ev.filter = EVFILT_WRITE;
	ev.flags = EV_ADD | EV_ENABLE;
	n = kevent(kq, &ev, 1, NULL, 0, NULL);
	if (n == -1)
		exit(1);

	read(fd[[0]], buf, sizeof(buf));

	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	n = kevent(kq, NULL, 0, &ev, 1, &ts);
	if (n == -1 || n == 0)
		exit(1);

	exit(0);
}, [AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_WORKING_KQUEUE, 1,
		[Define if kqueue works correctly with pipes])
    havekqueue=yes
    ], AC_MSG_RESULT(no), AC_MSG_RESULT(no))
	fi
fi
AM_CONDITIONAL(KQUEUE_BACKEND, [test "x$havekqueue" = "xyes"])

# Checks for libraries
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for library functions
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 gethostname gettimeofday strerror])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([memchr memmove memset])
AC_CHECK_FUNCS([strchr strndup strtoul])

# Package options
AC_MSG_CHECKING([whether to enable debug logs and asserts])
AC_ARG_ENABLE([debug],
  [AS_HELP_STRING(
    [--enable-debug=@<:@full|yes|log|no@:>@],
    [enable debug logs and asserts @<:@default=no@:>@])
  ],
  [],
  [enable_debug=no])
AS_CASE([x$enable_debug],
  [xfull], [AC_DEFINE([HAVE_ASSERT_PANIC], [1],
                      [Define to 1 if panic on an assert is enabled])
            AC_DEFINE([HAVE_DEBUG_LOG], [1], [Define to 1 if debug log is enabled])
           ],
  [xyes], [AC_DEFINE([HAVE_ASSERT_LOG], [1],
                     [Define to 1 if log on an assert is enabled])
           AC_DEFINE([HAVE_DEBUG_LOG], [1], [Define to 1 if debug log is enabled])
          ],
  [xlog], [AC_DEFINE([HAVE_DEBUG_LOG], [1], [Define to 1 if debug log is enabled])],
  [xno], [],
  [AC_MSG_FAILURE([invalid value ${enable_debug} for --enable-debug])])
AC_MSG_RESULT($enable_debug)

AC_MSG_CHECKING([whether to disable stats])
AC_ARG_ENABLE([stats],
  [AS_HELP_STRING(
    [--disable-stats],
    [disable stats])
  ],
  [disable_stats=yes],
  [disable_stats=no])
AS_IF([test "x$disable_stats" = xyes],
  [],
  [AC_DEFINE([HAVE_STATS], [1], [Define to 1 if stats is not disabled])])
AC_MSG_RESULT($disable_stats)

# Untar the yaml-0.1.4 in contrib/ before config.status is rerun
AC_CONFIG_COMMANDS_PRE([tar xvfz contrib/yaml-0.1.4.tar.gz -C contrib])

# Call yaml-0.1.4 ./configure recursively
AC_CONFIG_SUBDIRS([contrib/yaml-0.1.4])

# Define Makefiles
AC_CONFIG_FILES([Makefile
                 contrib/Makefile
                 src/Makefile
                 src/hashkit/Makefile
                 src/proto/Makefile])

# Generate the "configure" script
AC_OUTPUT
