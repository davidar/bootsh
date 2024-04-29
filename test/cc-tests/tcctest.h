#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline const char *get_basefile_from_header(void)
{
  return __BASE_FILE__;
}

static inline const char *get_file_from_header(void)
{
  return __FILE__;
}
