#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;

layout(location = 0) out vec4 trianglecolor;

layout(binding = 0) uniform mvpMatrix {
    mat4 mvpMatrix;
} uMVP;

void main (void) {
    for(int vertex = 0; vertex < 3; vertex++) {
        if(vertex == 0) {
            trianglecolor = vec4(1.0, 0.0, 0.0, 1.0);
        } else if(vertex == 1) {
            trianglecolor = vec4(0.0, 1.0, 0.0, 1.0);
        } else if(vertex == 2) {
            trianglecolor = vec4(0.0, 0.0, 1.0, 1.0);
        }

        gl_Position = uMVP.mvpMatrix * (gl_in[vertex].gl_Position + vec4(0.0, 1.0, 0.0, 0.0));
        EmitVertex();

        gl_Position = uMVP.mvpMatrix * (gl_in[vertex].gl_Position + vec4(-1.0, -1.0, 0.0, 0.0));
        EmitVertex();

        gl_Position = uMVP.mvpMatrix * (gl_in[vertex].gl_Position + vec4(1.0, -1.0, 0.0, 0.0));
        EmitVertex();
        EndPrimitive();
    }
}