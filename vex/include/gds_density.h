#ifndef GDS_DENSITY_H
#define GDS_DENSITY_H

/*
    wraps a fit function inside, the parameters are named so the gds has better meaning
*/
float gds_density_falloff(
    vector origin, point_position;
    float min_distance, max_distance, min_value, max_value, decay_rate
)
{
    float _distance = distance(origin, point_position);
    return pow(fit(_distance, min_distance, max_distance, min_value, max_value), decay_rate);
}

float gds_density_along_axis(
    vector axis_origin, axis_direction, point_position;
    float min_distance, max_distance, min_value, max_value 
)
{
    vector axis =  normalize(axis_direction);
    float _distance = dot(axis, point_position - axis_origin);
    return fit(_distance, min_distance, max_distance, min_value, max_value);
}

#endif