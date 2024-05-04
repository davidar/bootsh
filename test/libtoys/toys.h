#pragma once

/* Toybox infrastructure.
 *
 * Copyright 2006 Rob Landley <rob@landley.net>
 */

// Stuff that needs to go before the standard headers

#include "config.h"
#include "portability.h"

// General posix-2008 headers
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <paths.h>
#include <pwd.h>
#include <regex.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

// Posix networking

#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

// Internationalization support (also in POSIX)

#include <langinfo.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

// Non-posix headers
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/ttydefaults.h>

#include "lib.h"
#include "lsm.h"
#include "toyflags.h"

#include "flags.h"
