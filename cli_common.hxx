#ifndef TD_CLI_COMMON_H__
#define TD_CLI_COMMON_H__

#include <string>

/* mak better be a 12-byte buffer */
std::string get_mak_from_conf_file();

void do_version(int exitval);
void print_qualcomm_msg();

#endif // TD_CLI_COMMON_H__
