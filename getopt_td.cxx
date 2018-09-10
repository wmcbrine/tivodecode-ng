/*
 * getopt_td() -- long options parser
 *
 * Portions Copyright (c) 1987, 1993, 1994
 * The Regents of the University of California.  All rights reserved.
 *
 * Portions Copyright (c) 2003
 * PostgreSQL Global Development Group
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.    IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "getopt_td.hxx"

#include <cstring>

int td_optind = 1;
const char *td_optarg = NULL;

#define BADCH  '?'
#define BADARG ':'
#define EMSG   ""

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
        {
            return -1;
        }

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
            for (i = 0; longopts[i].name != NULL; i++)
            {
                if (std::strlen(longopts[i].name) == namelen
                    && std::strncmp(place, longopts[i].name, namelen) == 0)
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
                        td_optarg = NULL;
                        if (place[namelen] != 0)
                        {
                            /* XXX error? */
                        }
                    }

                    td_optind++;

                    if (longindex)
                        *longindex = i;

                    place = EMSG;

                    if (longopts[i].flag == NULL)
                        return longopts[i].val;
                    else
                    {
                        *longopts[i].flag = longopts[i].val;
                        return 0;
                    }
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
        td_optarg = NULL;
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
