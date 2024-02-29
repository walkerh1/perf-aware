#include "haversine.h"
#include "haversine_formula.c"

#define MAX_IDENT 64            // max allowed length for an identifier in JSON
#define MAX_JSON_DIGITS 32      // max allowed digits in a given JSON number
#define MIN_JSON_PAIR_SIZE 24   // min 6 bytes (e.g. '"x0":0') * 4 coordinates == 24 bytes min

// ====================================== Token Types ===================================== //

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
    TokenType type;
    union {
        f64 number;
        char identifier[MAX_IDENT];
    };
} Token;

// ===================================== Parser Types ===================================== //

typedef enum {
    ELEM_NONE,

    ELEM_IDENTIFIER,
    ELEM_FLOAT,
    ELEM_DICT,
    ELEM_ARRAY,

    ELEM_COUNT,
} JsonElementType;

typedef struct JsonElement JsonElement;

typedef struct {
    char *key;
    JsonElement *value;
    JsonElement *next;
} DictPair;

typedef struct {
    DictPair *entries;
} JsonDict;

typedef struct {
    JsonElement *value;
    JsonElement *next;
} ArrayElement;

typedef struct {
    ArrayElement *entries;
} JsonArray;

struct JsonElement {
    JsonElementType type;
    union {
        JsonDict *dict;
        JsonArray *array;
        char *identifier;
        f64 number;
    };
};

typedef struct {
    f64 x0, y0;
    f64 x1, y1;
} Pair;

// ======================================= Tokenizer ====================================== //

char *curr_byte;    // pointer to current byte in json input

void set_identifier(Token *token) {
    // PRE: assume curr_byte is '"' when this function is called; if
    // it isn't, that is a bug in the calling code not this function.
    curr_byte++;
    u32 i = 0;
    while (*curr_byte != '"') {
        token->identifier[i] = *curr_byte;
        i++;
        curr_byte++;
    }

    token->identifier[i] = '\0'; // close off identifier with null byte

    if (i == 0) {
        fprintf(stderr, "PARSING ERROR: cannot have empty identifier in JSON\n");
        exit(1);
    }

    curr_byte++;

    // POST: curr_byte is at first char after the closing '"' of identifier
}

void set_number(Token *token) {
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

    token->number = atof(num);

    // POST: curr_byte is at first char after the last digit in the number
}

Token next_token() {
    while (*curr_byte == ' ' || *curr_byte == '\n' || *curr_byte == '\t') {
        curr_byte++;
    }

    Token token = {};

    char c = *curr_byte;
    switch (c) {
        case '{':
            token.type = TOKEN_LBRACE;
            break;
        case '}':
            token.type = TOKEN_RBRACE;
            break;
        case ':':
            token.type = TOKEN_COLON;
            break;
        case ',':
            token.type = TOKEN_COMMA;
            break;
        case '[':
            token.type = TOKEN_LBRACKET;
            break;
        case ']':
            token.type = TOKEN_RBRACKET;
            break;
        case '"':
            token.type = TOKEN_IDENTIFIER;
            set_identifier(&token);
            break;
        default:
            if (c == '-' || isdigit(c)) {
                token.type = TOKEN_FLOAT;
                set_number(&token);
            } else if (c == '\0') {
                token.type = TOKEN_NONE;
            } else {
                fprintf(stderr, "PARSING ERROR: unknown token '%d'\n", c);
                exit(1);
            }
            break;
    }

    curr_byte++; // advance to next byte for next time next_token is called

    return token;
}

// ======================================== Parser ======================================== //

JsonElement *parse_json_element() {
    JsonElement *res = (JsonElement *) malloc(sizeof(JsonElement));

    Token token = next_token();
    switch (token.type) {
        case TOKEN_LBRACE:
            res->type = ELEM_DICT;
            break;
        case TOKEN_LBRACKET:
            res->type = ELEM_ARRAY;
            break;
        case TOKEN_IDENTIFIER:
            res->type = ELEM_IDENTIFIER;
            res->identifier = token.identifier;
            break;
        case TOKEN_FLOAT:
            res->type = ELEM_FLOAT;
            break;
        case TOKEN_NONE:
            res->type = ELEM_NONE;
            break;
        default:
            fprintf(stderr, "ERROR: malformed JSON\n");
            exit(1);
    }

    return res;
}

JsonElement *parse_json(buffer input_json) {
    curr_byte = (char *)input_json.data;

    JsonElement *top_element = parse_json_element();

    return top_element;
}

void parse_haversine_pairs(buffer input_json, Pair *pairs) {
//    JsonElement *json = parse_json(input_json);
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