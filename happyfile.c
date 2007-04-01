/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
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
#include "fseeko.h"
#include "happyfile.h"

#if defined HAVE_FSEEKO
#	define hftell(a) ftello(a)
#	define hfseek(a, b, c) fseeko(a, b, c)
#else
#	define hftell(a) ftell(a)
#	define hfseek(a, b, c) fseek(a, b, c)
#	warning Large file support is questionable on this platform
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
		memcpy (ptr, fh->buffer + (fh->pos - fh->buffer_start), (size_t)(fh->buffer_fill - (fh->pos - fh->buffer_start)));
		nbytes += (size_t)(fh->buffer_fill - (fh->pos - fh->buffer_start));
	}

	do
	{
		fh->buffer_start += fh->buffer_fill;
		fh->buffer_fill = (hoff_t)fread (fh->buffer, 1, BUFFERSIZE, fh->fh);

		if (fh->buffer_fill == 0)
			break;

		memcpy((char *)ptr + nbytes, fh->buffer, ((hoff_t)(size - nbytes) < fh->buffer_fill ? (size - nbytes) : (size_t)fh->buffer_fill));
		nbytes += ((hoff_t)(size - nbytes) < fh->buffer_fill ? (size - nbytes) : (size_t)fh->buffer_fill);
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
				int t = (int)((offset - fh->pos) & 0xfff);
				hoff_t u = (offset - fh->pos) >> 12;
				hoff_t s;
				for (s=0; s < u; s++)
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
				int t = (int)(offset & 0xfff);
				hoff_t u = offset >> 12;
				hoff_t s;
				for (s=0; s < u; s++)
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
