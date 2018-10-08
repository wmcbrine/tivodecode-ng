/*
 * tivodecode-ng
 * Copyright 2006-2018, Jeremy Drake et al.
 * See COPYING file for license terms
 */

#ifndef CLI_COMMON_HXX_
#define CLI_COMMON_HXX_

#include <string>

/* mak better be a 12-byte buffer */
std::string get_mak_from_conf_file();

void do_version(int exitval);
void print_qualcomm_msg();

#endif /* CLI_COMMON_HXX_ */
