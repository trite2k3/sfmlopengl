#include <iostream>

#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <GL/glew.h>
#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static GLuint CompileShader(GLuint type, const std::string& source)
{
    // create shader object and load data
    GLuint id = glCreateShader(type);

    // load source and compile
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // Check shader compilation debug output
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    // if result = GL_TRUE, compilation successfull
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));

        // get compile log
        glGetShaderInfoLog(id, length, &length, message);

        // print
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")  << " shader!" << std::endl;
        std::cout << message << std::endl;

        // delete shader since it didnt work out
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static GLuint CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    // make the program and compile
    GLuint program = glCreateProgram();
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    // attach vertex and fragment to shader program
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    // update shader program by linking it and do validation
    glLinkProgram(program);
    glValidateProgram(program);

    // cleanup (not needed anymore it has been linked to shader program)
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

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
    
    // create the window and set limit framerate
    sf::RenderWindow window(sf::VideoMode(1024, 768), "OpenGL + SFML Test");
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(60);
    
    // set GLEW to experimental to avoid problems when using glew in a core context, then initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Troubleshooting
    if (glGetError() != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error occurred" << std::endl;
        return -1;
    }
    
    // Triangle array (data for buffer)
    float vertices[] = {
        0.0f, 0.5f, // Vertex 1 (X, Y)
        0.5f, -0.5f, // Vertex 2 (X, Y)
        -0.5f, -0.5f // Vertex 3 (X, Y)
    };
    
    // Upload vertices data into GPU
    GLuint buffer;
    glGenBuffers(1, &buffer); //Generate 1 buffer, 1 is the ID
    
    // create active array buffer and put vertices data into it
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // put position data in the shader and enable vertex attribute array
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

    // Vertex shader, bare bones no color.
    const std::string vertexSource = R"glsl(
        #version 330 core
        layout(location = 0) in vec2 position;
        void main()
        {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )glsl";

    const std::string fragmentSource = R"glsl(
        #version 330 core
        out vec4 outColor;
        void main()
        {
            outColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )glsl";

    // pass source and create + compile, then use that program
    GLuint shader = CreateShader(vertexSource, fragmentSource);
    glUseProgram(shader);

    // activate the window
    window.setActive(true);
    
    // Create a clock for measuring time elapsed
    sf::Clock clock;
    
    // prepare OpenGL surface for HSR
    glClearDepth(1.f);
    glClearColor(0.3f, 0.3f, 0.3f, 0.f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // modern opengl glm perspective and camera
    glm::mat4 projection = glm::perspective(glm::radians(90.0f), 4.0f / 3.0f, 1.0f, 300.0f);
    GLuint projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // load a music to play and loop
    sf::Music music;
    if (!music.openFromFile("sample.ogg"))
        return EXIT_FAILURE;
    music.play();
    music.setLoop(true);

    //
    // main loop
    //
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
                glViewport(0, 0, event.size.width, event.size.height);
                projection = glm::perspective(glm::radians(90.0f), static_cast<float>(event.size.width) / event.size.height, 1.0f, 300.0f);
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            }
            else if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))
            {
                running = false;
            }
        }

        // clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // OpenGL drawing
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // end the current frame (internally swaps the front and back buffers)
        window.display();
    }

    // release resources...
    glDeleteProgram(shader);
    glDeleteBuffers(1, &buffer);

    return 0;
}
