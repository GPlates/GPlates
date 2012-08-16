//
// Geometry shader source code to render surface fill mask.
//

// "#extension" needs to be specified in the shader source *string* where it is used (this is not
// documented in the GLSL spec but is mentioned at http://www.opengl.org/wiki/GLSL_Core_Language).
#extension GL_EXT_geometry_shader4 : enable

// The view projection matrices for rendering into each cube face.
uniform mat4 cube_face_view_projection_matrices[6];

void main (void)
{
    // Iterate over the six cube faces.
    for (int cube_face = 0; cube_face < 6; cube_face++)
    {
        // The three vertices of the current triangle primitive.
        for (int vertex = 0; vertex < 3; vertex++)
        {
            // Transform each vertex using the current cube face view-projection matrix.
            gl_Position = cube_face_view_projection_matrices[cube_face] * gl_PositionIn[vertex];

            // Each cube face is represented by a separate layer in a texture array.
            gl_Layer = cube_face;

            EmitVertex();
        }
        EndPrimitive();
    }
}
