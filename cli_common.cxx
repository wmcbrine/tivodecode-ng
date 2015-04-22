#include "tdconfig.h"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

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
    FILE *mak_file = NULL;
    const char *home_dir = std::getenv(HOME_ENV_NAME);

    if (!home_dir)
        home_dir = DEFAULT_EMPTY_HOME;

    std::string mak_fname = home_dir;
    mak_fname += MAK_DOTFILE_NAME;

    if ((mak_file = std::fopen(mak_fname.c_str(), "r")))
    {
        if (std::fread(mak, 1, 11, mak_file) >= 10)
        {
            int i;
            for (i = 11; i >= 10 && (mak[i] == '\0' || std::isspace(mak[i]));
                 --i)
            {
                mak[i] = '\0';
            }
        }
        else if (std::ferror(mak_file))
        {
            perror("reading mak config file");
            goto fail;
        }
        else
        {
            std::fprintf(stderr, "mak too short in mak config file\n");
            goto fail;
        }

        std::fclose(mak_file);
    }
    else
        goto fail;

    return 1;

fail:
    if (mak_file)
        std::fclose(mak_file);
    return 0;
}

void do_version(int exitval)
{
    std::cerr << PACKAGE_STRING "\n";
    std::cerr << "Copyright 2006-2015, Jeremy Drake et al.\n";
    std::cerr << "See COPYING file in distribution for details\n\n";
    PRINT_QUALCOMM_MSG();

    std::exit(exitval);
}

/* vi:set ai ts=4 sw=4 expandtab: */
