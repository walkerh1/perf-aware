#include "haversine.h"

buffer load_json(char *);
Token *tokenize(buffer);

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

i32 main(i32 argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s [coordinate_pairs.json]\n", argv[0]);
        exit(1);
    }

    // open json file > determine size of file > allocate memory for data > and read file into memory
    buffer input_json = {};
    FILE *file;
    if ((file = fopen(argv[0], "rb")) == NULL) {
        fprintf(stderr, "ERROR: unable to open \"%s\"\n", argv[0]);
        exit(1);
    }
    struct stat file_stat;
    stat(argv[0], &file_stat);
    size_t count = file_stat.st_size;
    if ((input_json.data = (u8 *) malloc(count)) == NULL) {
        fprintf(stderr, "ERROR: unable to allocate %lu bytes\n", count);
        exit(1);
    }
    input_json.count = count;
    if ((fread(input_json.data, input_json.count, 1, file)) != 1) {
        fprintf(stderr, "ERROR: unable to read \"%s\"\n", argv[0]);
        free(input_json.data);
        exit(1);
    }
    fclose(file);

}