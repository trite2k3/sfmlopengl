#include <iostream>
#include <vector>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>

// Struct to hold the source code for vertex and fragment shaders
struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

// Function to parse shader files and separate vertex and fragment shader code
static ShaderProgramSource ParseShader(const std::string& filepath)
{
    std::ifstream stream(filepath);
    std::string line;
    std::stringstream ss[2];
    enum class ShaderType { NONE = -1, VERTEX = 0, FRAGMENT = 1 };
    ShaderType type = ShaderType::NONE;

    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << '\n';
        }
    }

    return { ss[0].str(), ss[1].str() };
}

// Function to compile individual shaders (vertex or fragment)
static GLuint CompileShader(GLuint type, const std::string& source)
{
    GLuint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // Error handling for shader compilation
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cerr << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
        std::cerr << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

// Function to create a shader program by linking vertex and fragment shaders
static GLuint CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    GLuint program = glCreateProgram();
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    // Clean up shaders as they are no longer needed after linking
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

// Function to generate vertices and indices for a sphere
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, unsigned int rings, unsigned int sectors)
{
    const float PI = 3.14159265359f;
    const float R = 1.0f / (float)(rings - 1);
    const float S = 1.0f / (float)(sectors - 1);

    for (unsigned int r = 0; r < rings; ++r)
    {
        for (unsigned int s = 0; s < sectors; ++s)
        {
            float y = sin(-PI / 2.0f + PI * r * R);
            float x = cos(2.0f * PI * s * S) * sin(PI * r * R);
            float z = sin(2.0f * PI * s * S) * sin(PI * r * R);

            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    for (unsigned int r = 0; r < rings - 1; ++r)
    {
        for (unsigned int s = 0; s < sectors - 1; ++s)
        {
            indices.push_back(r * sectors + s);
            indices.push_back(r * sectors + (s + 1));
            indices.push_back((r + 1) * sectors + (s + 1));
            indices.push_back(r * sectors + s);
            indices.push_back((r + 1) * sectors + (s + 1));
            indices.push_back((r + 1) * sectors + s);
        }
    }
}

// Class for analyzing amplitude from an SFML sound buffer
class AmplitudeAnalyzer : public sf::SoundStream {
public:
    AmplitudeAnalyzer() {}

    // Set the sound buffer to be analyzed
    void setSoundBuffer(const sf::SoundBuffer& buffer) {
        m_buffer = buffer;
        m_sampleRate = buffer.getSampleRate();
        m_channelCount = buffer.getChannelCount();
        initialize(m_channelCount, m_sampleRate); // Reinitialize with the correct sample rate and channels
        m_currentSampleIndex = 0; // Reset current sample index
    }

    // Provide audio data to the stream
    bool onGetData(sf::SoundStream::Chunk& data) override {
        const std::size_t sampleCount = 3072; // Adjust the buffer size as needed
        data.sampleCount = sampleCount;
        data.samples = new sf::Int16[sampleCount];

        // Read audio samples from the sound buffer
        if (m_currentSampleIndex < m_buffer.getSampleCount()) {
            const sf::Int16* bufferSamples = m_buffer.getSamples();
            std::size_t samplesToCopy = std::min(sampleCount, static_cast<std::size_t>(m_buffer.getSampleCount() - m_currentSampleIndex));
            std::memcpy(const_cast<sf::Int16*>(data.samples), &bufferSamples[m_currentSampleIndex], samplesToCopy * sizeof(sf::Int16));

            m_currentSampleIndex += samplesToCopy;
            return true;
        } else {
            return false;
        }
    }

    // Override the onSeek function to reset the current sample index
    void onSeek(sf::Time timeOffset) override {
        m_currentSampleIndex = static_cast<std::size_t>(timeOffset.asSeconds() * m_sampleRate * m_channelCount); // Convert seconds to sample index
    }

    // Calculate the amplitude of the current sample
    float getAmplitude() const {
        if (m_currentSampleIndex < m_buffer.getSampleCount()) {
            sf::Int16 sample = m_buffer.getSamples()[m_currentSampleIndex];
            return std::abs(static_cast<float>(sample)) / 32767.0f; // Normalize to range [0, 1]
        } else {
            return 0.0f;
        }
    }

private:
    sf::SoundBuffer m_buffer; // Sound buffer to hold the audio data
    std::size_t m_currentSampleIndex = 0; // Index of the current sample being analyzed
    unsigned int m_sampleRate; // Sample rate of the audio buffer
    unsigned int m_channelCount; // Channel count of the audio buffer
};

int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 2;
    settings.majorVersion = 4;
    settings.minorVersion = 6;
    settings.attributeFlags = sf::ContextSettings::Core;

    // Create the SFML window with fullscreen style
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktopMode, "OpenGL + SFML Test", sf::Style::Fullscreen, settings);
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(144);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Check for OpenGL errors
    if (glGetError() != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error occurred" << std::endl;
        return -1;
    }

    // Parse and compile shaders
    ShaderProgramSource source = ParseShader("shader.glsl");
    GLuint shader = CreateShader(source.VertexSource, source.FragmentSource);
    glUseProgram(shader);

    // Generate sphere data
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(vertices, indices, 1.0f, 32, 32);

    // Setup vertex array object (VAO), vertex buffer object (VBO), and element buffer object (EBO)
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Specify vertex attribute layout
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Set up projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(desktopMode.width) / desktopMode.height, 0.1f, 100.0f);
    glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 3.0f);

    // Get uniform locations
    GLuint modelLoc = glGetUniformLocation(shader, "model");
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    GLuint projLoc = glGetUniformLocation(shader, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shader, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shader, "viewPos");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Set up SFML audio
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("sample.ogg"))
    {
        std::cerr << "Failed to load sound file" << std::endl;
        return -1;
    }

    // Initialize and play audio analyzer
    AmplitudeAnalyzer analyzer;
    analyzer.setSoundBuffer(buffer);
    analyzer.play();

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Set up view matrix
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    bool running = true;
    while (running)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                running = false;
            }
            else if (event.type == sf::Event::Resized)
            {
                glViewport(0, 0, event.size.width, event.size.height);
                projection = glm::perspective(glm::radians(90.0f), static_cast<float>(event.size.width) / event.size.height, 0.1f, 100.0f);
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            }
            else if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))
            {
                running = false;
            }
        }

        // Get the current amplitude and scale the model matrix accordingly
        float amplitude = analyzer.getAmplitude();
        float scale = 1.0f + amplitude;
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set uniform variables for light and view positions
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

        // Draw the sphere
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Display the rendered frame
        window.display();
    }

    // Clean up OpenGL resources
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader);

    return 0;
}
