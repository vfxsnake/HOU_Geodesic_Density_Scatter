#ifndef GDS_LLOYD_H
#define GDS_LLOYD_H

#define GDS_CELL_ATTRIB "gds_cell"  

int gds_assign_euclidean(int geometry_input_index; vector position)
{
    return nearpoint(geometry_input_index, position);
}


vector gds_cell_center_position(int geometry_input_index, point_number; vector seed_position)
{
    // getting and array of points that hold the ptnum as cell value.
    int sample_array_num[] = findattribval(geometry_input_index, "point", GDS_CELL_ATTRIB, point_number);
    int cell_count = len(sample_array_num);
    if (cell_count <= 0)
    {
        return seed_position;
    } 
    
    vector cell_centroid = {0.0, 0.0, 0.0};
    int success = 0;
    foreach (int pt_num; sample_array_num)
    {
        cell_centroid += pointattrib(geometry_input_index, "P",pt_num, success);
    }

    return cell_centroid / cell_count;
   
}


vector gds_project_to_surface(int geometry_input_index; vector position)
{
    // use xyz dist to get the prim and uv
    int prim_num;
    vector prim_uv;
    xyzdist(geometry_input_index, position, prim_num, prim_uv);

    return primuv(geometry_input_index, "P", prim_num, prim_uv);

} 

#endif