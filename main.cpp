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

// Structure to hold shader source code
struct ShaderProgramSource
{
    std::string VertexSource; // Vertex shader source code
    std::string FragmentSource; // Fragment shader source code
};

// Function to parse the shader file and separate vertex and fragment shaders
static ShaderProgramSource ParseShader(const std::string& filepath)
{
    std::ifstream stream(filepath);
    std::string line;
    std::stringstream ss[2];
    enum class ShaderType { NONE = -1, VERTEX = 0, FRAGMENT = 1 };
    ShaderType type = ShaderType::NONE;

    // Read the shader file line by line
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
            ss[(int)type] << line << '\n'; // Append line to the appropriate shader source
        }
    }

    return { ss[0].str(), ss[1].str() }; // Return the parsed shader sources
}

// Function to compile a shader from source code
static GLuint CompileShader(GLuint type, const std::string& source)
{
    GLuint id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

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

    return id; // Return the shader ID if compilation is successful
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

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program; // Return the created shader program ID
}

// Function to generate a sphere's vertices and indices
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, unsigned int rings, unsigned int sectors)
{
    const float PI = 3.14159265359f;
    const float R = 1.0f / (float)(rings - 1);
    const float S = 1.0f / (float)(sectors - 1);

    // Generate sphere vertices
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

    // Generate sphere indices
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

// Class to analyze the amplitude of audio samples
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

    // Override the onGetData function to fetch audio samples
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

    // Create a window with OpenGL context
    sf::RenderWindow window(sf::VideoMode(1024, 768), "OpenGL + SFML Test", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(false);
    window.setFramerateLimit(144);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    if (glGetError() != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error occurred" << std::endl;
        return -1;
    }

    // Parse the shader file
    ShaderProgramSource source = ParseShader("shader.glsl");
    // Create and use the shader program
    GLuint shader = CreateShader(source.VertexSource, source.FragmentSource);
    glUseProgram(shader);

    // Generate sphere vertices and indices
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(vertices, indices, 1.0f, 32, 32);

    // Create and bind VAO, VBO, and EBO
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Specify vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Set up projection, view, and model matrices
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    GLuint projLoc = glGetUniformLocation(shader, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    GLuint viewLoc = glGetUniformLocation(shader, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glm::mat4 model = glm::mat4(1.0f);
    GLuint modelLoc = glGetUniformLocation(shader, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLuint lightPosLoc = glGetUniformLocation(shader, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shader, "viewPos");

    window.setActive(true);
    sf::Clock clock;

    glClearDepth(1.f);
    glClearColor(0.0f, 0.0f, 0.0f, 0.f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // Create an instance of the amplitude analyzer
    AmplitudeAnalyzer analyzer;

    // Load the OGG sound file into the sound buffer
    sf::SoundBuffer buffer;
    if (!buffer.loadFromFile("sample.ogg")) {
        std::cerr << "Failed to load sound file." << std::endl;
        return EXIT_FAILURE;
    }

    // Set the sound buffer for the analyzer
    analyzer.setSoundBuffer(buffer);
    analyzer.setLoop(true); // Set the sound to loop

    // Start playing the audio
    analyzer.play();

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

        // Calculate and display the amplitude of the current sample
        float amplitude = analyzer.getAmplitude();
        //std::cout << "Amplitude: " << amplitude << std::endl;

        // Calculate scale based on amplitude and apply to the model matrix
        float scale = 1.0f + amplitude; // Adjust this scale factor as needed
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 2.0f);
        glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, 3.0f);

        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        window.display();
    }

    // Clean up resources
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader);

    return 0;
}
