/*-------------------------------------------------------------------------
 *
 * fseeko.c
 *	  64-bit versions of fseeko/ftello()
 *
 * Portions Copyright (c) 1996-2007, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/port/fseeko.c,v 1.20 2006/03/05 15:59:10 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

/*
 * We have to use the native defines here because configure hasn't
 * completed yet.
 */
#if defined(__bsdi__) || defined(__NetBSD__) || defined(WIN32)

#ifdef HAVE_CONFIG_H
# include "tdconfig.h"
#endif

#include <errno.h>
#include <stdio.h>
#ifdef HAVE_MEMORY_H
# include <memory.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef WIN32
# include <io.h>
#endif

#ifdef bsdi
# include <pthread.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef WIN32
# define OFF_T_TYPE __int64
#else
# define OFF_T_TYPE off_t
#endif


/*
 *	On BSD/OS and NetBSD, off_t and fpos_t are the same.  Standards
 *	say off_t is an arithmetic type, but not necessarily integral,
 *	while fpos_t might be neither.
 *
 *	This is thread-safe on BSD/OS using flockfile/funlockfile.
 */

int
fseeko(FILE *stream, OFF_T_TYPE offset, int whence)
{
	fpos_t		floc;
#ifndef WIN32
	struct stat filestat;
#endif

	switch (whence)
	{
		case SEEK_CUR:
#ifdef bsdi
			flockfile(stream);
#endif
			if (fgetpos(stream, &floc) != 0)
				goto failure;
			floc += offset;
			break;
		case SEEK_SET:
			floc = offset;
			break;
		case SEEK_END:
#ifdef bsdi
			flockfile(stream);
#endif
			fflush(stream);		/* force writes to fd for stat() */
#ifdef WIN32
			floc = _filelengthi64(_fileno(stream));
#else
			if (fstat(fileno(stream), &filestat) != 0)
				goto failure;
			floc = filestat.st_size;
#endif
			floc += offset;
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	if (fsetpos(stream, &floc) != 0)
		goto failure;
#ifdef bsdi
	funlockfile(stream);
#endif
	return 0;

failure:
#ifdef bsdi
	funlockfile(stream);
#endif
	return -1;
}


OFF_T_TYPE
ftello(FILE *stream)
{
	fpos_t		floc;

	if (fgetpos(stream, &floc) != 0)
		return -1;
	return floc;
}

#endif
