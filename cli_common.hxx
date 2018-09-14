#ifndef TD_CLI_COMMON_H__
#define TD_CLI_COMMON_H__

/* mak better be a 12-byte buffer */
bool get_mak_from_conf_file(char *mak);

void do_version(int exitval);
void print_qualcomm_msg();

#endif // TD_CLI_COMMON_H__
