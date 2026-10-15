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
    // always compute dacay int a 0 to 1 range
    float _decay = pow(fit(_distance, min_distance, max_distance, 1, 0), decay_rate);
    
    // set the final range from 0 to 1 to min and max values. 
    return fit10(_decay, min_value, max_value);
}

float gds_density_along_axis(
    vector axis_origin, axis_direction, point_position;
    float min_distance, max_distance, min_value, max_value, decay_rate 
)
{
    vector axis =  normalize(axis_direction);
    float _distance = dot(axis, point_position - axis_origin);
    
    // always compute dacay int a 0 to 1 range
    float _decay = pow(fit(_distance, min_distance, max_distance, 1, 0), decay_rate);
    
    // set the final range from 0 to 1 to min and max values. 
    return fit10(_decay, min_value, max_value);
}

#endif