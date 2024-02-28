#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>
#include <sys/stat.h>

#define EARTH_RADIUS 6372.8
#define X_MAX 180
#define Y_MAX 90

#define MAX_IDENT 64

#define min(a, b) (a) <= (b) ? (a) : (b)
#define max(a, b) (a) >= (b) ? (a) : (b)

typedef int8_t u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef double f64;

typedef struct {
    size_t count;
    u8 *data;
} buffer;

typedef enum {
    TOKEN_NONE,

    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_IDENTIFIER,
    TOKEN_FLOAT,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,

    TOKEN_COUNT
} TokenType;

typedef struct {
    TokenType token_type;
    buffer value;
} Token;

typedef struct {
    f64 x0, y0;
    f64 x1, y1;
} Pair;