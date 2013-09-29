#ifndef TD_CLI_COMMON_H__
#define TD_CLI_COMMON_H__
#ifdef WIN32
#   include <fcntl.h>
#endif

#define PRINT_QUALCOMM_MSG() fprintf (stderr, "Encryption by QUALCOMM ;)\n\n")

/* TODO: make these not need to be globals */
extern int o_verbose;
extern int o_no_verify;

/* mak better be a 12-byte buffer */
int get_mak_from_conf_file(char * mak);

void do_version(int exitval);

#endif // TD_CLI_COMMON_H__
