#include "haversine.h"
#include "haversine_formula.c"
#include "haversine_clock.c"

#define MAX_IDENT 64            // max allowed length for an identifier in JSON
#define MAX_JSON_DIGITS 32      // max allowed digits in a given JSON number
#define MIN_JSON_PAIR_SIZE 24   // min 6 bytes (e.g. '"x0":0') * 4 coordinates == 24 bytes min

// toggle for profiler
#define PROFILER 1
#include "haversine_profiler.c"

// ====================================== Token Types ===================================== //

typedef struct {
    size_t count;
    u8 *data;
} Buffer;

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
        Buffer identifier;
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
typedef struct DictPair DictPair;
typedef struct ArrayElement ArrayElement;

struct DictPair {
    Buffer key;
    JsonElement *value;
    DictPair *next;
};

typedef struct {
    DictPair *entries;
} JsonDict;

struct ArrayElement {
    JsonElement *value;
    ArrayElement *next;
};

typedef struct {
    ArrayElement *entries;
} JsonArray;

struct JsonElement {
    JsonElementType type;
    union {
        JsonDict *dict;
        JsonArray *array;
        Buffer identifier;
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
    Buffer identifier;
    curr_byte++;
    u32 i = 0;
    identifier.data = (u8*)curr_byte;
    while (*curr_byte != '"') {
        i++;
        curr_byte++;
    }
    identifier.count = i;
    token->identifier = identifier;

    if (i == 0) {
        fprintf(stderr, "PARSING ERROR: cannot have empty identifier in JSON\n");
        exit(1);
    }

    // POST: curr_byte is at last char of identifier
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
    curr_byte--;
    num[i] = '\0';

    token->number = atof(num);

    // POST: curr_byte is at last char of number string
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

JsonElement *parse_json_element(Token *token);

JsonDict *parse_dictionary() {
    JsonDict *res = (JsonDict *) malloc(sizeof(JsonDict));

    Token token = next_token();
    if (token.type == TOKEN_RBRACE) {
        res->entries = NULL;
        return res;
    }

    DictPair *entry = (DictPair *) malloc(sizeof(DictPair));
    res->entries = entry;
    while (token.type != TOKEN_RBRACE) {
        if (token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "PARSING ERROR: expected identifier for dictionary key (%u)\n", token.type);
            exit(1);
        }
        entry->key = token.identifier;

        token = next_token();
        if (token.type != TOKEN_COLON) {
            fprintf(stderr, "PARSING ERROR: expected colon after identifier in dictionary entry\n");
            exit(1);
        }

        token = next_token();
        JsonElement *value = parse_json_element(&token);
        entry->value = value;

        token = next_token();
        if (token.type == TOKEN_COMMA) {
            entry->next = (DictPair *) malloc(sizeof(DictPair));
            entry = entry->next;
            token = next_token();
            if (token.type == TOKEN_RBRACE) {
                fprintf(stderr, "PARSING ERROR: unexpected '}' after ','\n");
                exit(1);
            }
        } else {
            entry->next = NULL;
        }
    }

    return res;
}

JsonArray *parse_array() {
    JsonArray *res = (JsonArray *) malloc(sizeof(JsonArray));

    Token token = next_token();
    if (token.type == TOKEN_RBRACKET) {
        res->entries = NULL;
        return res;
    }

    ArrayElement *entry = (ArrayElement *) malloc(sizeof(ArrayElement));
    res->entries = entry;
    while (token.type != TOKEN_RBRACKET) {
        entry->value = parse_json_element(&token);
        token = next_token();
        if (token.type == TOKEN_COMMA) {
            token = next_token();
            if (token.type == TOKEN_RBRACKET) {
                fprintf(stderr, "PARSING ERROR: unexpected ']' after ','\n");
                exit(1);
            }
            entry->next = (ArrayElement *) malloc(sizeof(ArrayElement));
            entry = entry->next;
        } else {
            entry->next = NULL;
        }
    }

    return res;
}

JsonElement *parse_json_element(Token *token) {
    JsonElement *res = (JsonElement *) malloc(sizeof(JsonElement));

    switch (token->type) {
        case TOKEN_LBRACE:
            res->type = ELEM_DICT;
            res->dict = parse_dictionary();
            break;
        case TOKEN_LBRACKET:
            res->type = ELEM_ARRAY;
            res->array = parse_array();
            break;
        case TOKEN_IDENTIFIER:
            res->type = ELEM_IDENTIFIER;
            res->identifier = token->identifier;
            break;
        case TOKEN_FLOAT:
            res->type = ELEM_FLOAT;
            res->number = token->number;
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

JsonElement *parse_json(Buffer input_json) {
    curr_byte = (char *)input_json.data;

    Token token = next_token();
    JsonElement *top_element = parse_json_element(&token);

    return top_element;
}

void free_json(JsonElement *json) {
    switch (json->type) {
        case ELEM_DICT: {
            DictPair *curr = json->dict->entries;
            DictPair *prev = NULL;
            while (curr != NULL) {
                prev = curr;
                curr = curr->next;
                free_json(prev->value);
                free(prev);
            }
            free(json->dict);
            break;
        }
        case ELEM_ARRAY: {
            ArrayElement *curr = json->array->entries;
            ArrayElement *prev = NULL;
            while (curr != NULL) {
                prev = curr;
                curr = curr->next;
                free_json(prev->value);
                free(prev);
            }
            free(json->array);
            break;
        }
        default: {}
    }
    free(json);
}

bool are_equal(Buffer s1, Buffer s2) {
    bool res = true;
    if (s1.count != s2.count) {
        res = false;
    } else {
        for (u32 i = 0; i < s1.count; i++) {
            if (s1.data[i] != s2.data[i]) {
                res = false;
                break;
            }
        }
    }
    return res;
}

JsonElement *lookup(JsonElement *dict, Buffer key) {
    DictPair *curr = dict->dict->entries;
    while (!are_equal(curr->key, key)) {
        curr = curr->next;
    }
    return curr->value;
}

f64 unwrap_number(JsonElement *number) {
    assert(number->type == ELEM_FLOAT);
    return number->number;
}

u64 parse_haversine_pairs(Buffer input_json, Pair *pairs, u64 max_count) {
    BEGIN_TIME_FUNCTION;
    Buffer pairs_key = { 5, (u8*)("pairs") };
    BEGIN_TIME_BLOCK("parse json");
    JsonElement *parsed_json = parse_json(input_json);
    END_TIME_BLOCK("parse json");

    JsonElement *pairs_array = lookup(parsed_json, pairs_key);

    if (pairs_array->type != ELEM_ARRAY) {
        fprintf(stderr, "LOOKUP ERROR: expected an array\n");
        exit(1);
    }

    BEGIN_TIME_BLOCK("populate pairs_array");
    Buffer x0 = { 2, (u8*)("x0") };
    Buffer y0 = { 2, (u8*)("y0") };
    Buffer x1 = { 2, (u8*)("x1") };
    Buffer y1 = { 2, (u8*)("y1") };
    u64 count = 0;
    for (ArrayElement *element = pairs_array->array->entries; element && count < max_count; element = element->next) {
        JsonElement *dict = element->value;
        assert(dict->type == ELEM_DICT);
        pairs->x0 = unwrap_number(lookup(dict, x0));
        pairs->y0 = unwrap_number(lookup(dict, y0));
        pairs->x1 = unwrap_number(lookup(dict, x1));
        pairs->y1 = unwrap_number(lookup(dict, y1));
        pairs++;
        count++;
    }
    END_TIME_BLOCK("populate pairs_array");

    BEGIN_TIME_BLOCK("free");
    free_json(parsed_json);
    END_TIME_BLOCK("free");

    END_TIME_FUNCTION;

    return count;
}

// ===================================== Main Routine ===================================== //

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [coordinate_pairs.json]\n", argv[0]);
        exit(1);
    }

     begin_profiler();

    // allocate all memory required for program
    char *filename = argv[1];
    Buffer input_json = {};

    BEGIN_TIME_BLOCK("allocating")
    struct stat file_stat;
    stat(filename, &file_stat);
    size_t count = file_stat.st_size + 1; // add 1 for sentinel null byte
    if ((input_json.data = (u8 *) malloc(count)) == NULL) {
        fprintf(stderr, "ERROR: unable to allocate %lu bytes\n", count);
        exit(1);
    }
    Buffer haversine_pairs = {};
    input_json.count = count-1;
    u64 max_pair_count = input_json.count / MIN_JSON_PAIR_SIZE; // just estimate size based on input_json size)
    if (max_pair_count == 0) {
        fprintf(stderr, "ERROR: malformed JSON input\n");
        exit(1);
    }
    if ((haversine_pairs.data = (u8 *) malloc(max_pair_count * sizeof(Pair))) == NULL) {
        fprintf(stderr, "ERROR: unable to allocate %lu bytes\n", haversine_pairs.count);
        exit(1);
    }
    haversine_pairs.count = max_pair_count * sizeof(Pair);
    END_TIME_BLOCK("allocating")

    // open and read json file into memory
    BEGIN_BANDWIDTH_BLOCK("reading", input_json.count)
    FILE *file;
    if ((file = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "ERROR: unable to open \"%s\"\n", filename);
        exit(1);
    }
    if ((fread(input_json.data, input_json.count, 1, file)) != 1) {
        fprintf(stderr, "ERROR: unable to read \"%s\"\n", filename);
        free(input_json.data);
        exit(1);
    }
    input_json.data[input_json.count] = '\0';
    fclose(file);
    END_TIME_BLOCK("reading")

    // parse input JSON into haversine pairs
    Pair *pairs = (Pair *)haversine_pairs.data; // cast u8 array to Pair array
    u64 n = parse_haversine_pairs(input_json, pairs, max_pair_count);

    // sum haversine distances
    BEGIN_BANDWIDTH_BLOCK("sum", 32 * n)
    f64 sum = 0;
    for (int i = 0; i < n; i++) {
        sum += haversine(pairs[i].x0, pairs[i].y0, pairs[i].x1, pairs[i].y1, EARTH_RADIUS);
    }
    END_TIME_BLOCK("sum")

    // report
    fprintf(stdout, "Input size: %zu bytes\n", input_json.count);
    fprintf(stdout, "Pair count: %lu\n", n);
    fprintf(stdout, "Haversine average: %.16f\n", sum / (f64)n);

    end_and_print_profiler();

    return 0;
}
