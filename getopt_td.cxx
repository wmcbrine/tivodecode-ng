/*
 * getopt_td() -- long options parser
 *
 * Portions Copyright 1987, 1993, 1994
 * The Regents of the University of California.  All rights reserved.
 *
 * Portions Copyright 2003-2006, PostgreSQL Global Development Group
 * Adapted for tivodecode-ng
 * See COPYING file for license terms
 */

#include "getopt_td.hxx"

#include <cstring>

int td_optind = 1;
std::string td_optarg;

static const int BADCH = '?';
static const int BADARG = ':';
static const char EMSG[] = "";

int getopt_td(int argc, const char **argv, const char *optstring,
              const struct td_option *longopts, int *longindex)
{
    static const char *place = EMSG;   /* option letter processing */
    const char *oli;                   /* option letter list index */
    int optopt;

    if (!*place)
    {                            /* update scanning pointer */
        if (td_optind >= argc)
        {
            place = EMSG;
            return -1;
        }

        place = argv[td_optind];

        if (place[0] != '-')
        {
            place = EMSG;
            return -1;
        }

        if (place[1] == '\0')
            return -1;

        place++;

        if (place[0] == '-' && place[1] == '\0')
        {                        /* found "--" */
            ++td_optind;
            place = EMSG;
            return -1;
        }

        if (place[0] == '-' && place[1])
        {
            /* long option */
            size_t namelen;
            int i;

            place++;

            namelen = std::strcspn(place, "=");
            std::string name(place, namelen);

            for (i = 0; longopts[i].name != ""; i++)
            {
                if (name == longopts[i].name)
                {
                    if (longopts[i].has_arg)
                    {
                        if (place[namelen] == '=')
                            td_optarg = place + namelen + 1;
                        else if (td_optind < argc - 1)
                        {
                            td_optind++;
                            td_optarg = argv[td_optind];
                        }
                        else
                        {
                            if (optstring[0] == ':')
                                return BADARG;
                            place = EMSG;
                            td_optind++;
                            return BADCH;
                        }
                    }
                    else
                    {
                        td_optarg = "";
                        if (place[namelen] != 0)
                        {
                            /* XXX error? */
                        }
                    }

                    td_optind++;

                    if (longindex)
                        *longindex = i;

                    place = EMSG;

                    return longopts[i].shortopt;
                }
            }
            place = EMSG;
            td_optind++;
            return BADCH;
        }
    }

    /* short option */
    optopt = (int) *place++;

    oli = std::strchr(optstring, optopt);
    if (!oli)
    {
        if (!*place)
            ++td_optind;
        return BADCH;
    }

    if (oli[1] != ':')
    {                            /* don't need argument */
        td_optarg = "";
        if (!*place)
            ++td_optind;
    }
    else
    {                            /* need an argument */
        if (*place)                /* no white space */
            td_optarg = place;
        else if (argc <= ++td_optind)
        {                        /* no arg */
            place = EMSG;
            if (*optstring == ':')
                return BADARG;
            return BADCH;
        }
        else
            /* white space */
            td_optarg = argv[td_optind];
        place = EMSG;
        ++td_optind;
    }
    return optopt;
}
