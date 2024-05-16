#include <iostream>
using namespace std;

#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <GL/glew.h>
#include <GL/gl.h>

// time to define a rendering pipeline
// when we issue a draw call
// the vertex shader will be called (for each vertex that is rendered)
// the vertex shader will forward all the attributes (like position and everything else) to the fragment shader
// the vertex shader is primarily responsible for vertices position
// then the fragment shader will be called
// the fragment shader will fill in the pixels between the vertices
// it will process each pixel between the vertices and decide what color each pixel will be shaded in
// the fragment shader will be called each time per vertex times the area (the amount of pixels)
// doing computations and passing data from the vertex shader to the fragment shader should be prioritized due to performance
// the primary function of the fragment shader is to decide what color a pixel should be
// then the result will be drawn on the screen (a lot of other things may happen in between, this is a simplified version)

// Vertex shader, bare bones no color.
const GLchar* vertexSource = R"glsl(
    #version 150 core
    
    in vec2 position;
    
    void main()
    {
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";

// Fragment/pixel shader
const GLchar* fragmentSource = R"glsl(
    #version 150 core
    
    out vec4 outColor;
    
    void main()
    {
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
)glsl";

// placeholder for tesselation/geometry/compute shaders?

int main()
{
    // Declare some variables for settings and then call for a window
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 2; // Optional
    settings.majorVersion = 4; //Request OpenGL version 3.2, nope 4.6
    settings.minorVersion = 6;
    settings.attributeFlags = sf::ContextSettings::Core;
    
    // create the window
    sf::RenderWindow window(sf::VideoMode(1024, 768), "OpenGL + SFML Test");
    window.setVerticalSyncEnabled(false);

    // limit framerate
    window.setFramerateLimit(60);
    
    // GLEW will pull data on what your GPU is capable of
    // set it to experimental to avoid problems when using glew in a core context
    // note that this is before glewinit
    glewExperimental = GL_TRUE;

    // initialize glew
    glewInit();
    
    // Troubleshooting
    if (glGetError() != 0)
    {
        printf("glGetError()");
    }
    
    // GLEW was initiated and loaded a function called glGenBuffers
    // lets use it
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    
    // Triangle array (data for buffer)
    // we dont specify amount of positions because screw that :D
    // each line is a vertex, a vertex is a point that is on your geometry (has nothing to do with a position)
    // a vertex can be much more than just a position (texture coords, colors, normals, binormals, tangets etc.)
    float vertices[] = {
        0.0f, 0.5f, // Vertex 1 (X, Y)
        0.5f, -0.5f, // Vertex 2 (X, Y)
        -0.5f, -0.5f // Vertex 3 (X, Y)
    };
    
    // Upload vertices data into GPU for processing by the shaders n' stuff
    // this is the vertex buffer (an array of bytes in memory in the gpu [vram])
    GLuint vbo;
    glGenBuffers(1, &vbo); //Generate 1 buffer, 1 is the ID
    
    // upload data through GLuint vbo and call glBindBuffer to
    // create active array buffer and put vertices data into it
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // create a definition of how large this buffer is
    // giving data to this buffer before or after declaration is optional
    // we dont specify size of our array, just use sizeof()
    // the static draw will probably be changed later on, but for this example lets use static
    // the "use" of this refers to how many times it will be drawn
    // docs.gl is a good website to check manpages for glBufferData
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // compile shaders (a program that runs on the gpu using the vertex array buffers to draw it on screen)
    // the shader needs to know the layout of the buffer or what it contains, position of vertices, textures etc.
    // create shader object and load data
    // compile vertices
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    
    // Troubleshooting
    // Check shader compilation debug output?
    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    
    //
    // if status = GL_TRUE, compilation successfull
    //
    // get compile log
    char buffer[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
    
    // compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    
    // connect vertices shader with
    // fragment shader into one program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    
    // the fragment shader should write into this buffer for the moment?
    // it can write into multiple buffers
    // glDrawBuffers
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    
    // update shader program by linking it
    glLinkProgram(shaderProgram);
    
    // use this shader program
    glUseProgram(shaderProgram);
    
    // init. vao to be able to switch shader
    // without resetting attribute, stor attr. here
    // vbo has raw vertex data
    GLuint vao;
    glGenVertexArrays(1, &vao);
    
    // bind it to start using
    // each time glVertexAttribPointer is called that info will be stored in vao
    glBindVertexArray(vao);
    
    // put position data in the vertex shader
    // need to tell openGL what is in memory and how to handle it
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    // index (what number in the array states the attribute of the defined type) ... ->
    // of which attribute we are refering to (an attribute is a position or color or texture etc.)
    // in this case its a position with 2 floats as values for the position (2 component vector)
    // if we had 3d (vec3) we would be setting this value to 3 and have 3 floats in the buffer
    // GL_FALSE is normalised since we are lazy and let openGL set our color values and ... ->
    // we dont want to do it manually in our cpu
    // the first 0 is "stride" which is an offset of bytes between each vertex in the vertex buffer
    // the stride depends on how many bytes your vertex is, aka the size of each vertex
    // the second 0 is the "pointer", a pointer into the actual attribute
    // the pointer refers to how many bytes in each attribute in the vertex is (aka where each value in the ... ->
    // vertex starts) so openGL can start reading from the buffer where the wanted value is
    // basically its a simplified version of where to start to read in the memory instead of manually writing ... ->
    // what byte to start to read at in the memory
    // glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    // lets try to make this stride a bit more flexible
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
    // if we had vec3 the pointer would be 8 bytes, but we cant just pass 8, it needs to be a const void pointer
    // if we were real programmers we would have a struct to define our vertex and we could use offset of macro ... ->
    // to figure out what this value should be. we are not though so screw it lol :D
    // glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (const void*)8);
    
    // enable vertex attribute array
    glEnableVertexAttribArray(posAttrib);
    
    // activate the window
    window.setActive(true);

    // load resources, initialize the OpenGL states, ...
    
    //counter var.
    int countscale = 0;
    
    // Create a clock for measuring time elapsed
    sf::Clock clock;
    
    // prepare OpenGL surface for HSR
    glClearDepth(1.f);
    glClearColor(0.3f, 0.3f, 0.3f, 0.f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // Setup a perspective projection & Camera position
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //b0rk3d (tm)?
    gluPerspective(90.f, 1.f, 1.f, 300.0f);//fov, aspect, zNear, zFar

    // Load a music to play
    sf::Music music;
    if (!music.openFromFile("sample.ogg"))
        return EXIT_FAILURE;
    
    // Play the music
    music.play();

    // Loop it
    music.setLoop(true);

    // run the main loop
    bool running = true;
    while (running)
    {
        // handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                // end the program
                running = false;
            }
            else if (event.type == sf::Event::Resized)
            {
                // adjust the viewport when the window is resized
                glViewport(0, 0, event.size.width, event.size.height);
            }
            else if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))
                
                // close program if ESC key is pressed
                running = false;
        }


        // clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // OpenGL drawing
        // this is used when you do not have an index buffer
        // the primitive we are trying to draw is a triangle
        // draw index is 0
        // and count is 3 (number of indices/vertices to be rendered)
        // note that this will draw the latest selected/binded/bound buffer
        glDrawArrays(GL_TRIANGLES, 0, 3);
        // if and index buffer is present we would use
        // count is 3
        // type of index data (unsigned int/short)
        // pointer to the indices, almost never used and prob null
        // glDrawElements(GL_TRIANGELS, 3, $index, $indicepointer)

        // end the current frame (internally swaps the front and back buffers)
        window.display();
    }

    // release resources...

    return 0;
}
