/*
 * tivodecode, (c) 2006, Jeremy Drake
 * See COPYING file for license terms
 */
#ifndef SHA1_H_
#define SHA1_H_

typedef struct {
    unsigned int state[5];
    unsigned int count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void sha1_init (SHA1_CTX * context);
void sha1_update (SHA1_CTX * context, unsigned char * data, size_t len);
void sha1_final (unsigned char digest[20], SHA1_CTX * context);

#endif
