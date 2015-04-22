#include "tdconfig.h"
#include <stdio.h>

#include <stddef.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif

#include "tivo_types.hxx"
#include "cli_common.hxx"

#ifdef WIN32
#   define HOME_ENV_NAME "USERPROFILE"
#   define DEFAULT_EMPTY_HOME "C:"
#else
#   define HOME_ENV_NAME "HOME"
#   define DEFAULT_EMPTY_HOME ""
#endif

static const char MAK_DOTFILE_NAME[] = "/.tivodecode_mak";

int get_mak_from_conf_file(char *mak)
{
    char * mak_fname = NULL;
    FILE * mak_file = NULL;
    const char * home_dir = getenv(HOME_ENV_NAME);
    size_t home_dir_len;

    if (!home_dir)
        home_dir = DEFAULT_EMPTY_HOME;

    home_dir_len = strlen(home_dir);

    mak_fname = new char[home_dir_len + sizeof(MAK_DOTFILE_NAME)];
    if (!mak_fname)
    {
        fprintf(stderr, "error allocing string for mak config file name\n");
        goto fail;
    }

    memcpy(mak_fname, home_dir, home_dir_len);
    memcpy(mak_fname + home_dir_len, MAK_DOTFILE_NAME,
           sizeof(MAK_DOTFILE_NAME));

    if ((mak_file = fopen(mak_fname, "r")))
    {
        if (fread(mak, 1, 11, mak_file) >= 10)
        {
            int i;
            for (i = 11; i >= 10 && (mak[i] == '\0' || isspace((int)mak[i]));
                 --i)
            {
                mak[i] = '\0';
            }
        }
        else if (ferror(mak_file))
        {
            perror ("reading mak config file");
            goto fail;
        }
        else
        {
            fprintf(stderr, "mak too short in mak config file\n");
            goto fail;
        }

        fclose (mak_file);
        mak_file = NULL;
    }
    else
        goto fail;

    delete[] mak_fname;
    mak_fname = NULL;
    return 1;

fail:
    if (mak_file)
        fclose(mak_file);
    if (mak_fname)
        delete[] mak_fname;
    return 0;
}

void do_version(int exitval)
{
    fprintf(stderr, "%s\n", PACKAGE_STRING);
    fprintf(stderr, "Copyright 2006-2015, Jeremy Drake et al.\n");
    fprintf(stderr, "See COPYING file in distribution for details\n\n");
    PRINT_QUALCOMM_MSG();

    exit(exitval);
}

/* vi:set ai ts=4 sw=4 expandtab: */
