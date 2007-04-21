/* useful hex manipulation routine header */
/* Copyright C Qualcomm Inc 1997 */
/* @(#)hexlib.h	1.1 (AHAG) 02/08/23 */

#ifdef __cplusplus
extern "C" {
#endif
extern int	nerrors;

int hexprint(const char *, unsigned char *, int n);
int hexread(unsigned char *, char *, int n);
int hexcheck(unsigned char *, char *, int n);
int hexbulk(unsigned char *, int n);
#ifdef __cplusplus
}
#endif
