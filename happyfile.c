/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#include <errno.h>
#include <stdio.h>
#include <memory.h>
#if !defined(WIN32)
#	include <unistd.h>
#else
#	include <io.h>
#endif
#include "happyfile.h"

#if defined (_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#	define hftell ftello
#	define hfseek fseeko
#elif defined (WIN32)
static __int64 hftell (FILE *fp)
{
	fpos_t pos;
	if (fgetpos(fp, &pos))
		return (__int64)-1LL;
	else
		return (__int64)pos;
}

static int hfseek (FILE *fp, __int64 offset, int whence)
{
	fpos_t pos;
	if (whence == SEEK_CUR)
	{
		int rt=fgetpos(fp, &pos);
		if(rt)
			return rt;
		pos += (fpos_t) offset;
	}
	else if (whence == SEEK_END)
		pos = (fpos_t) (_filelengthi64(fileno(fp)) + offset);
	else if (whence == SEEK_SET)
		pos = (fpos_t) offset;

	return fsetpos(fp, &pos);
}
#else
#	define hftell ftell
#	define hfseek fseek
#endif

happy_file * hopen (char * filename, char * mode)
{
	happy_file * fh = malloc (sizeof (happy_file));
	fh->fh = fopen (filename, mode);
	if (!fh->fh)
	{
		free (fh);
		return NULL;
	}
	fh->pos = 0;
	fh->buffer_start = 0;
	fh->buffer_fill = 0;
	return fh;
}

happy_file * hattach (FILE * fh)
{
	happy_file * hfh = malloc (sizeof (happy_file));
	hfh->fh = fh;
	hfh->pos = 0;
	hfh->buffer_start = 0;
	hfh->buffer_fill = 0;
	return hfh;
}

int hclose(happy_file * fh)
{
	int x = fclose (fh->fh);
	free (fh);
	return x;
}

int hdetach(happy_file * fh)
{
	free(fh);
	return 0;
}

size_t hread (void * ptr, size_t size, happy_file * fh)
{
	size_t nbytes = 0;

	if (size == 0)
		return 0;

	if ((fh->pos + (hoff_t)size) - fh->buffer_start <= fh->buffer_fill)
	{
		memcpy (ptr, fh->buffer + (fh->pos - fh->buffer_start), size);
		fh->pos += (hoff_t)size;
		return size;
	}
	else if (fh->pos < fh->buffer_start + fh->buffer_fill)
	{
		memcpy (ptr, fh->buffer + (fh->pos - fh->buffer_start), fh->buffer_fill - (fh->pos - fh->buffer_start));
		nbytes += fh->buffer_fill - (fh->pos - fh->buffer_start);
	}

	do
	{
		fh->buffer_start += fh->buffer_fill;
		fh->buffer_fill = (hoff_t)fread (fh->buffer, 1, BUFFERSIZE, fh->fh);

		if (fh->buffer_fill == 0)
			break;

		memcpy((char *)ptr + nbytes, fh->buffer, ((hoff_t)(size - nbytes) < fh->buffer_fill ? (size - nbytes) : fh->buffer_fill));
		nbytes += ((hoff_t)(size - nbytes) < fh->buffer_fill ? (size - nbytes) : fh->buffer_fill);
	} while (nbytes < size);

	fh->pos += (hoff_t)nbytes;
	return nbytes;
}

hoff_t htell (happy_file * fh)
{
	return fh->pos;
}

int hseek (happy_file * fh, hoff_t offset, int whence)
{
	static char junk_buf[4096];

	register int r;

	switch (whence)
	{
		case SEEK_SET:
			if (offset < fh->pos)
			{
				r = hfseek (fh->fh, offset, SEEK_SET);
				if (r < 0)
					return r;
				fh->pos = offset;
				fh->buffer_start = fh->pos;
				fh->buffer_fill = 0;
				return r;
			}
			else
			{
				register int t = (offset - fh->pos) & 0xfff;
				register int u = (offset - fh->pos) >> 12;
				for (r=0; r < u; r++)
				{
					if (hread(junk_buf, 4096, fh) != 4096)
						return -1;
				}

				if (hread(junk_buf, t, fh) != t)
				{
					return -1;
				}

				return 0;
			}
			break;
		case SEEK_CUR:
			if (offset < 0)
			{
				r = hfseek (fh->fh, offset, SEEK_CUR);
				if (r < 0)
					return r;
				fh->pos += offset;
				fh->buffer_start = fh->pos;
				fh->buffer_fill = 0;
				return r;
			}
			else
			{
				register int t = offset & 0xfff;
				register int u = offset >> 12;
				for (r=0; r < u; r++)
				{
					if (hread(junk_buf, 4096, fh) != 4096)
						return -1;
				}

				if (hread(junk_buf, t, fh) != t)
				{
					return -1;
				}

				return 0;
			}

			break;
		case SEEK_END:
			r = hfseek (fh->fh, offset,whence);
			fh->pos = hftell(fh->fh);
			fh->buffer_start = fh->pos;
			fh->buffer_fill = 0;
			return r;
		default:
			errno=EINVAL;
			return -1;
	}
}
