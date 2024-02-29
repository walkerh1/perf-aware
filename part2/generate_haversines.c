#include "haversine.h"
#include "haversine_formula.c"

u64 seed = 0;
u64 pair_count = 0;

f64 random_f64(f64 min, f64 max) {
    assert(max - min <= RAND_MAX);
    f64 normalised_rand = rand() / (f64)RAND_MAX;
    f64 diff = abs((i32)max - (i32)min);
    f64 res = min + (normalised_rand * diff);
    return res;
}

f64 random_degree(f64 centre, f64 radius) {
    f64 upper = centre + radius;
    f64 lower = -upper;

    upper = min(upper, X_MAX);
    lower = max(lower, -X_MAX);

    f64 res = random_f64(lower, upper);
    return res;
}

int main(int argc, char *argv[]) {
    f64 x_centre = 0;
    f64 y_centre = 0;
    f64 x_radius = X_MAX;
    f64 y_radius = Y_MAX;

    // for counting how many more coordinates to generate in the current cluster
    u64 cluster_count = UINT64_MAX;

    if (argc != 4) {
        fprintf(stderr, "USAGE: %s [uniform/cluster] [seed] [number of coordinates to generate]\n", argv[0]);
        exit(1);
    }

    // parse mode of coordinate generation
    if (strcmp(argv[1], "cluster") == 0) {
        cluster_count = 0;
    } else if (strcmp(argv[1], "uniform") != 0) {
        fprintf(stderr, "ERROR: unrecognised mode: %s\n", argv[1]);
        exit(1);
    }

    // parse seed and pair_count args
    seed = atoll(argv[2]);
    pair_count = atoll(argv[3]);

    // for breaking up into 64 clusters
    u64 cluster_count_max = 1 + pair_count / 64;

    // set PRNG seed
    srand(seed);

    // open files to be written to
    FILE *coordinates = fopen("haversine_coordinates.json", "wb");
    FILE *haversines = fopen("haversines.f64", "wb");

    f64 sum = 0;

    fprintf(coordinates, "{ \"pairs\": [\n");

    // generate n_coords and compute their haversine
    for (int i = 0; i < pair_count; i++) {
        f64 x0, y0, x1, y1;

        if (cluster_count-- == 0) {
            cluster_count = cluster_count_max;
            x_centre = random_f64(-X_MAX, X_MAX);
            y_centre = random_f64(-Y_MAX, Y_MAX);
            x_radius = random_f64(0, X_MAX);
            y_radius = random_f64(0, Y_MAX);
        }
        x0 = random_degree(x_centre, x_radius);
        y0 = random_degree(y_centre, y_radius);
        x1 = random_degree(x_centre, x_radius);
        y1 = random_degree(y_centre, y_radius);


        // write coordinates to coordinates file
        fprintf(coordinates, "\t{ \"x0\": %.16f, \"y0\": %.16f, \"x1\": %.16f, \"y1\": %.16f }", x0, y0, x1, y1);
        fprintf(coordinates, "%s", i + 1 == pair_count ? "\n" : ",\n");

        // compute haversine
        f64 hav = haversine(x0, y0, x1, y1, EARTH_RADIUS);

        sum += hav;

        // write haversine to haversine file
        fwrite(&hav, sizeof(hav), 1, haversines);
    }

    fprintf(coordinates, "]}");

    fprintf(stdout, "Method: %s\n", argv[1]);
    fprintf(stdout, "Random seed: %llu\n", seed);
    fprintf(stdout, "Pair count: %llu\n", pair_count);
    fprintf(stdout, "Expected sum: %.16f\n", sum / (f64)pair_count);

    fclose(coordinates);
    fclose(haversines);

    return 0;
}
