//#define MYRIAD

#ifndef MYRIAD

#include "../archive/pkey.h"
#include "../archive/skey.h"

#define MOD_VERSION 0x01

#else

#include "../myriad/pkey.h"
#include "../myriad/skey.h"

#define MOD_VERSION 0x02

#endif
