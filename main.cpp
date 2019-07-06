#include <iostream>
using namespace std;

#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <GL/glew.h>
#include <GL/gl.h>

// Vertex shader, bare bones no color.
const GLchar* vertexSource = R"glsl(
    #version 150 core
    
    in vec2 position;
    
    void main()
    {
        gl_Position = vec4(position, 0.0, 1.0);
    }
)glsl";
    
// Fragment shader
const GLchar* fragmentSource = R"glsl(
    #version 150 core
    
    out vec4 outColor;
    
    void main()
    {
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
)glsl";

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
    window.setVerticalSyncEnabled(true);
    
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
    
    //Triangle
    float vertices[] = {
        0.0f, 0.5f, // Vertex 1 (X, Y)
        0.5f, -0.5f, // Vertex 2 (X, Y)
        -0.5f, -0.5f // Vertex 3 (X, Y)
    };
    
    // Upload vertices data into GPU for processing by the shaders n' stuff
    GLuint vbo;
    glGenBuffers(1, &vbo); //Generate 1 buffer
    
    // upload data through GLuint vbo and call glBindBuffer to
    // create active array buffer and put vertices data into it
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // compile shaders
    // create shader object and load data
    // compile vertices
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    
    // Troubleshooting
    // Check shader compilation debug output?
    GLint status;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    
    cout << status;
    
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
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
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
    if (!music.openFromFile("Retribution.ogg"))
        return EXIT_FAILURE;
    
    // Play the music
    music.play();
    
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
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // end the current frame (internally swaps the front and back buffers)
        window.display();
    }

    // release resources...

    return 0;
}
