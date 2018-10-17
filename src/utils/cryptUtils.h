#ifndef CRYPT_UTILS_H
#define CRYPT_UTILS_H

#include "../crypt.h"

struct hashed {
  unsigned char* hash;
  char           salt_hex[2 * SALT_LEN + 1];
};

char*          encryptText(const char* text, const char* password);
unsigned char* decryptText(const char* cypher, const char* password);
int            crypt_compare(const unsigned char* s1, const unsigned char* s2);
struct hashed* hash(const char* str);
int            compareToHash(const char* str, struct hashed* h);
void           secFreeHashed(struct hashed* h);

#endif  // CRYPT_UTILS_H