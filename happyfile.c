#include <errno.h>
#include <stdio.h>
#include "happyfile.h"

happy_file * hopen (char * filename, char * mode)
{
	happy_file * fh = malloc (sizeof (happy_file));
	fh->fh = fopen (filename, mode);
	fh->pos = 0;
	return fh;
}

happy_file * hattach (FILE * fh)
{
	happy_file * hfh = malloc (sizeof (happy_file));
	hfh->fh = fh;
	hfh->pos = 0;
	return hfh;
}

int hclose(happy_file * fh)
{
	int x = fclose (fh->fh);
	free (fh);
	return x;
}

size_t hread (void * ptr, size_t size, happy_file * fh)
{
	size_t r = fread (ptr, 1, size, fh->fh);
	fh->pos += r;
	return r;
}

off_t htell (happy_file * fh)
{
	return fh->pos;
}

int hseek (happy_file * fh, off_t offset, int whence)
{
	static char junk_buf[4096];

	register int r;

	switch (whence)
	{
		case SEEK_SET:
			if (offset < fh->pos)
			{
				r = fseek (fh->fh, offset, SEEK_SET);
				if (r < 0)
					return r;
				fh->pos = offset;
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
				r = fseek (fh->fh, offset, SEEK_CUR);
				if (r < 0)
					return r;
				fh->pos += offset;
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
			r = fseek (fh->fh, offset,whence);
			fh->pos = ftell(fh->fh);
			return r;
		default:
			errno=EINVAL;
			return -1;
	}
}
