#ifndef __TYPES__H
#define __TYPES__H

typedef unsigned char byte;
typedef unsigned char *bytes;
typedef unsigned long long size;
typedef unsigned long long label;

void f(
  bytes *out,
  size *outlen,
  const label pid,
  const bytes in,
  const size inlen
);

void fserver(
  bytes *out,
  size *outlen,
  const bytes in,
  const size inlen
);

#endif 
