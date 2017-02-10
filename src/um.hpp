#ifndef UM_HPP
#define UM_HPP

#include <cstdint>
#include <cstdlib>
#include <cstdio>

// --------------------
//    Type aliases
// --------------------
typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef DEBUG
#define DEBUG(...) printf("[DEBUG] ");printf(__VA_ARGS__);
#endif

#ifndef ASSERT
# define Assert1(cond, file, line)                                  \
    if(!(cond)) {                                                   \
        printf("\nAssert failed in %s (Line %d)\n", file, line);    \
        abort();                                                    \
    }
# define ASSERT(cond) Assert1(cond, __FILE__, __LINE__)
#endif

#ifndef COUNT_OF
#define COUNT_OF(x) sizeof((x))/sizeof((x)[0])
#endif

#ifndef SIGN
#define SIGN(x) ((x) > 0) - ((x) < 0)
#endif

namespace um
{

struct Pairi
{
    i32 a, b;

    Pairi() {}
    Pairi(i32 a, i32 b): a(a), b(b) {}
};

inline bool
is_little_endian()
{
    i32 num = 1;
    if (*(char *)&num == 1)
        return true;
    else
        return false;
}

inline i32
min3(i32 a, i32 b, i32 c)
{
    i32 r = a;
    if (b < r) r = b;
    if (c < r) r = c;
    return r;
}

inline i32
max3(i32 a, i32 b, i32 c)
{
    i32 r = a;
    if (b > r) r = b;
    if (c > r) r = c;
    return r;
}

}

#endif // UM_HPP
