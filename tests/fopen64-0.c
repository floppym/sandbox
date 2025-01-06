#define CONFIG HAVE_FOPEN64
#define FUNC fopen64
#define SFUNC "fopen64"
#define FUNC_STR "\"%s\", \"%s\""
#define FUNC_IMP file, mode
#define ARG_CNT 2
#define ARG_USE "<file> <strmode>"

#define process_args() \
	s = argv[i++]; \
	const char *file = f_get_file(s); \
	\
	s = argv[i++]; \
	char *mode = s;

#define _LARGEFILE64_SOURCE
#include "test-skel-0.c"
