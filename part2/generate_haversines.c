#include "haversine.c"

bool cluster = false;
u64 seed = 0;
u64 pair_count = 0;

f64 random_f64(i32 min, i32 max) {
    assert(max - min <= RAND_MAX);
    f64 normalised_rand = rand() / (f64)RAND_MAX;
    u32 diff = abs(max - min);
    f64 res = min + (normalised_rand * diff);
    return res;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "USAGE: %s [uniform/cluster] [seed] [number of coordinates to generate]\n", argv[0]);
        exit(1);
    }

    // parse mode of coordinate generation
    if (strcmp(argv[1], "cluster") == 0) {
        cluster = true;
    } else if (strcmp(argv[1], "uniform") == 0) {
        cluster = false;
    } else {
        fprintf(stderr, "ERROR: unrecognised mode: %s\n", argv[1]);
        exit(1);
    }

    // parse seed and pair_count args
    seed = atoi(argv[2]);
    pair_count = atoi(argv[3]);

    // set seed to PRNG
    srand(seed);

    // open files to be written to
    FILE *coordinates = fopen("haversine_coordinates.json", "wb");
    FILE *haversines = fopen("haversines.f64", "wb");

    fprintf(coordinates, "{ \"pairs\": [");

    // generate n_coords and compute their haversine
    for (int i = 0; i < pair_count; i++) {
        f64 x0, y0, x1, y1;

        // generate coordinate pair
        if (cluster) {
            return 0;
        } else {
            x0 = random_f64(X_MIN, X_MAX);
            y0 = random_f64(Y_MIN, Y_MAX);
            x1 = random_f64(X_MIN, X_MAX);
            y1 = random_f64(Y_MIN, Y_MAX);
        }

        // write coordinates to coordinates file
        fprintf(coordinates, "{ \"x0\":%.16f, \"y0\":%.16f, \"x1\":%.16f, \"y1\":%.16f }", x0, y0, x1, y1);
        if (i + 1 < pair_count) fprintf(coordinates, ",");

        // compute haversine
        f64 hav = haversine(x0, y0, x1, y1, EARTH_RADIUS);

        // write haversine to haversine file
        fwrite(&hav, sizeof(hav), 1, haversines);

    }

    fprintf(coordinates, "]}");

    fclose(coordinates);
    fclose(haversines);

    return 0;
}
