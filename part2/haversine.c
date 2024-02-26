#include "haversine.h"

static f64 square(f64 a) {
    f64 res = (a*a);
    return res;
}

static f64 radians_from_degrees(f64 degrees) {
    f64 res = 0.01745329251994329577 * degrees;
    return res;
}

static f64 haversine(f64 x0, f64 y0, f64 x1, f64 y1, f64 earth_radius) {
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