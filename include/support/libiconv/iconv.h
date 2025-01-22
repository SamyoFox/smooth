#ifndef _ICONV_WRAP_
#define _ICONV_WRAP_

#if !defined __WIN32__ || defined BUILDING_LIBICONV
#	include <libiconv/iconv.h>
#else
#	define _LIBICONV_VERSION 0x0112

#	undef iconv_t

	typedef void* iconv_t;

#	if defined __cplusplus
	extern "C"
	{
#	endif

		extern int	 use_iconv;

		extern size_t	 (*iconv)(iconv_t, char **, size_t *, char **, size_t *);
		extern iconv_t	 (*iconv_open)(const char *, const char *);
		extern int	 (*iconv_close)(iconv_t);
		extern int	 (*iconvctl)(iconv_t, int, void *);

#	if defined __cplusplus
	}
#	endif

#	define ICONV_TRIVIALP            0 
#	define ICONV_GET_TRANSLITERATE   1
#	define ICONV_SET_TRANSLITERATE   2
#	define ICONV_GET_DISCARD_ILSEQ   3
#	define ICONV_SET_DISCARD_ILSEQ   4
#	define ICONV_SET_HOOKS           5
#	define ICONV_SET_FALLBACKS       6

#	endif
#endif
