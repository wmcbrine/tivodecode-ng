/*
 * Portions Copyright (c) 1987, 1993, 1994
 * The Regents of the University of California.  All rights reserved.
 *
 * Portions Copyright (c) 2003-2006, PostgreSQL Global Development Group
 *
 */

extern int td_optind;
extern const char *td_optarg;

struct td_option
{
    const char *name;
    bool has_arg;
    int shortopt;
};

extern int getopt_td(int argc, const char **argv, const char *optstring,
                     const struct td_option *longopts, int *longindex);
