#include "tdconfig.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "cli_common.hxx"

#ifdef WIN32
#   define HOME_ENV_NAME "USERPROFILE"
#   define DEFAULT_EMPTY_HOME "C:"
#else
#   define HOME_ENV_NAME "HOME"
#   define DEFAULT_EMPTY_HOME ""
#endif

static const char MAK_DOTFILE_NAME[] = "/.tivodecode_mak";

bool get_mak_from_conf_file(char *mak)
{
    const char *home_dir = std::getenv(HOME_ENV_NAME);

    if (!home_dir)
        home_dir = DEFAULT_EMPTY_HOME;

    std::string mak_fname = home_dir;
    mak_fname += MAK_DOTFILE_NAME;

    std::ifstream mak_file(mak_fname);
    if (mak_file.good())
    {
        mak_file.get(mak, 11);
        if (mak_file.gcount() >= 10)
            return true;
    }

    return false;
}

void print_qualcomm_msg()
{
    std::cerr << "Encryption by QUALCOMM ;)\n\n";
}

void do_version(int exitval)
{
    std::cerr << PACKAGE_STRING "\n"
        "Copyright 2006-2018, Jeremy Drake et al.\n"
        "See COPYING file in distribution for details\n\n";
    print_qualcomm_msg();
    std::exit(exitval);
}
