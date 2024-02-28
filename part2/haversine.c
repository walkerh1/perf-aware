#include "haversine.h"

#define MAX_IDENT 64            // max allowed length for an identifier in JSON
#define MIN_JSON_PAIR_SIZE 24   // min 6 bytes (e.g. '"x0":0') * 4 coordinates == 24 bytes min

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
    union {
        f64 number;
        char identifier[MAX_IDENT];
    };
} Token;

typedef struct {
    f64 x0, y0;
    f64 x1, y1;
} Pair;

buffer load_json(char *);
Token *tokenize(buffer);

// =================================== Haversine Formula ===================================//

f64 square(f64 a) {
    f64 res = (a*a);
    return res;
}

f64 radians_from_degrees(f64 degrees) {
    f64 res = 0.01745329251994329577 * degrees;
    return res;
}

f64 haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius) {
    f64 lat1 = y0;
    f64 lat2 = y1;
    f64 lon1 = x0;
    f64 lon2 = x1;

    f64 dLat = radians_from_degrees(lat2 - lat1);
    f64 dLon = radians_from_degrees(lon2 - lon1);
    lat1 = radians_from_degrees(lat1);
    lat2 = radians_from_degrees(lat2);

    f64 a = square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));

    f64 res = earth_radius * c;

    return res;
}

// ======================================= Tokenizer =======================================//

char *curr_byte;    // pointer to current byte in json input
Token token = {};   // current token

void grab_identifier() {

}

void grab_number() {

}

void next_token() {
    while (*curr_byte == ' ') {
        curr_byte++;
    }

    char c = *curr_byte;

    switch (c) {
        case '{':
            token.token_type = TOKEN_LBRACE;
            break;
        case '}':
            token.token_type = TOKEN_RBRACE;
            break;
        case '\"':
            grab_identifier();
            break;
        default:
            if (c == '-' || isdigit(c)) {
                grab_number();
            }
            break;
    }
}

// ======================================== Parser =========================================//

void parse_haversine_pairs(buffer input_json, Pair *pairs) {
    curr_byte = (char *)input_json.data;
    next_token();
    if (token.token_type != TOKEN_LBRACE) {
        fprintf(stderr, "PARSING ERROR: JSON input must begin with '{'\n");
    }

}

// ===================================== Main Routine ======================================//

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [coordinate_pairs.json]\n", argv[0]);
        exit(1);
    }

    // open json file > determine size of file > allocate memory for data > and read file into memory
    char *filename = argv[1];
    buffer input_json = {};
    FILE *file;
    if ((file = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "ERROR: unable to open \"%s\"\n", filename);
        exit(1);
    }
    struct stat file_stat;
    stat(filename, &file_stat);
    size_t count = file_stat.st_size;
    if ((input_json.data = (u8 *) malloc(count)) == NULL) {
        fprintf(stderr, "ERROR: unable to allocate %lu bytes\n", count);
        exit(1);
    }
    input_json.count = count;
    if ((fread(input_json.data, input_json.count, 1, file)) != 1) {
        fprintf(stderr, "ERROR: unable to read \"%s\"\n", filename);
        free(input_json.data);
        exit(1);
    }
    fclose(file);

    // allocate memory for haversine pairs (just estimate size based on input_json size)
    buffer haversine_pairs = {};
    u64 max_pair_count = input_json.count / MIN_JSON_PAIR_SIZE;
    if (max_pair_count == 0) {
        fprintf(stderr, "ERROR: malformed JSON input\n");
        exit(1);
    }
    if ((haversine_pairs.data = (u8 *) malloc(max_pair_count * sizeof(Pair))) == NULL) {
        fprintf(stderr, "ERROR: unable to allocate %lu bytes\n", haversine_pairs.count);
        exit(1);
    }
    haversine_pairs.count = max_pair_count * sizeof(Pair);

    // cast u8 array to Pair array
    Pair *pairs = (Pair *)haversine_pairs.data;

    // parse input JSON into haversine pairs
    parse_haversine_pairs(input_json, pairs);

    // TODO: sum haversine distances

    // free allocated memory
    free(haversine_pairs.data);
    free(input_json.data);

    return 0;
}