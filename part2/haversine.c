#include "haversine.h"

#define MAX_IDENT 64            // max allowed length for an identifier in JSON
#define MAX_JSON_DIGITS 32      // max allowed digits in a given JSON number
#define MIN_JSON_PAIR_SIZE 24   // min 6 bytes (e.g. '"x0":0') * 4 coordinates == 24 bytes min

// ========================================= Types ======================================== //

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

// =================================== Haversine Formula ================================== //

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

// ======================================= Tokenizer ====================================== //

char *curr_byte;    // pointer to current byte in json input
Token token = {};   // current token

void set_identifier() {
    // PRE: assume curr_byte is '"' when this function is called; if
    // it isn't, that is a bug in the calling code not this function.
    curr_byte++;
    u32 i = 0;
    while (*curr_byte != '"') {
        token.identifier[i] = *curr_byte;
        i++;
        curr_byte++;
    }

    token.identifier[i] = '\0'; // close off identifier with null byte

    if (i == 0) {
        fprintf(stderr, "PARSING ERROR: cannot have empty identifier in JSON\n");
        exit(1);
    }

    curr_byte++; // advance past closing '"'

    // POST: curr_byte is at first char after closing '"' of identifier
}

void set_number() {
    // PRE: assume curr_byte is a digit or '-' when this function is called;
    // if it isn't, that is a bug in the calling code not this function.
    char num[MAX_JSON_DIGITS];
    u32 i = 0;

    if (*curr_byte == '-') {
        num[i++] = *curr_byte;
        curr_byte++;
    }

    while (isdigit(*curr_byte)) {
        num[i++] = *curr_byte;
        curr_byte++;
    }

    if (*curr_byte == '.') {
        num[i++] = *curr_byte;
        curr_byte++;
        if (!isdigit(*curr_byte)) {
            fprintf(stderr, "PARSING ERROR: malformed number in JSON input\n");
            exit(1);
        }
    }

    while (isdigit(*curr_byte)) {
        num[i++] = *curr_byte;
        curr_byte++;
    }

    num[i] = '\0';

    token.number = atof(num);

    // POST: curr_byte is at first char after the last digit in the number
}

void next_token() {
    while (*curr_byte == ' ' || *curr_byte == '\n' || *curr_byte == '\t') {
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
        case ':':
            token.token_type = TOKEN_COLON;
            break;
        case ',':
            token.token_type = TOKEN_COMMA;
            break;
        case '[':
            token.token_type = TOKEN_LBRACKET;
            break;
        case ']':
            token.token_type = TOKEN_RBRACKET;
            break;
        case '"':
            token.token_type = TOKEN_IDENTIFIER;
            set_identifier();
            break;
        default:
            if (c == '-' || isdigit(c)) {
                token.token_type = TOKEN_FLOAT;
                set_number();
            } else if (c == '\0') {
                return;
            } else {
                fprintf(stderr, "PARSING ERROR: unknown token '%d'\n", c);
                exit(1);
            }
            break;
    }

    // advance to next byte for next time next_token is called
    curr_byte++;
}

// ======================================== Parser ======================================== //

void parse_haversine_pairs(buffer input_json, Pair *pairs) {
    curr_byte = (char *)input_json.data;
    next_token();

    // expect first token to be LBrace
    if (token.token_type != TOKEN_LBRACE) {
        fprintf(stderr, "PARSING ERROR: JSON input must begin with '{'\n");
    }

    while (*curr_byte != '\0') {
        next_token();
    }
}

// ===================================== Main Routine ===================================== //

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
    size_t count = file_stat.st_size + 1; // add 1 for sentinel null byte
    if ((input_json.data = (u8 *) malloc(count)) == NULL) {
        fprintf(stderr, "ERROR: unable to allocate %lu bytes\n", count);
        exit(1);
    }
    input_json.count = count-1;
    if ((fread(input_json.data, input_json.count, 1, file)) != 1) {
        fprintf(stderr, "ERROR: unable to read \"%s\"\n", filename);
        free(input_json.data);
        exit(1);
    }
    input_json.data[input_json.count] = '\0';
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