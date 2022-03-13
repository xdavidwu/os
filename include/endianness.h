#ifndef ENDIANNESS_H
#define ENDIANNESS_H

// TODO: get rid of GCC-ism?

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define FROM_LE16(x)	(x)
#define FROM_LE32(x)	(x)
#define FROM_LE64(x)	(x)
#define FROM_BE16(x)	__builtin_bswap16(x)
#define FROM_BE32(x)	__builtin_bswap32(x)
#define FROM_BE64(x)	__builtin_bswap64(x)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FROM_LE16(x)	__builtin_bswap16(x)
#define FROM_LE32(x)	__builtin_bswap32(x)
#define FROM_LE64(x)	__builtin_bswap64(x)
#define FROM_BE16(x)	(x)
#define FROM_BE32(x)	(x)
#define FROM_BE64(x)	(x)
#else
#error Unsupported endianness
#endif

#endif
