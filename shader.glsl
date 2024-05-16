#shader vertex
#version 330 core
layout(location = 0) in vec2 position;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}


#shader fragment
#version 330 core
out vec4 outColor;
void main()
{
    outColor = vec4(0.3, 0.8, 0.0, 1.0);
}

