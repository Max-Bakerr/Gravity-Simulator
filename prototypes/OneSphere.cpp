#include <iostream>
#include <cmath>
#include <variant>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/gl3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

//vertex shader:
const char* vertexShader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    out vec3 FragPos;
    out vec3 Normal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = normalize(aPos);
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";


//fragment shader:

const char* fragmentShader = R"(
    #version 330 core
    out vec4 FragColor;
    in vec3 Normal;
    in vec3 FragPos;

    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;

    void main() {
        // Ambient shading
        float ambientStrength = 0.2;
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse shading
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Specular shading
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;

        //Combine results
        vec3 result = (ambient + diffuse + specular) * objectColor;
        FragColor = vec4(result, 1.0);
    }
)";



static glm::vec3 sphericalToCartesian(float r, float theta, float phi)
{
    float x = r * sinf(theta) * cosf(phi);
    float y = r * cosf(theta);
    float z = r * sinf(theta) * sinf(phi);
    return glm::vec3(x, y, z);
}

static std::vector<float> GenerateSphere(float radius, int stacks, int sectors)
{
    std::vector<float> vertices;
    vertices.reserve(stacks * sectors * 6 * 3); // 2 triangles per quad, 3 verts each, 3 comps

    for (int i = 0; i < stacks; ++i) {
        float theta1 = (float)i / (float)stacks * glm::pi<float>();
        float theta2 = (float)(i + 1) / (float)stacks * glm::pi<float>();

        for (int j = 0; j < sectors; ++j) {
            float phi1 = (float)j / (float)sectors * glm::two_pi<float>();
            float phi2 = (float)(j + 1) / (float)sectors * glm::two_pi<float>();

            glm::vec3 v1 = sphericalToCartesian(radius, theta1, phi1);
            glm::vec3 v2 = sphericalToCartesian(radius, theta1, phi2);
            glm::vec3 v3 = sphericalToCartesian(radius, theta2, phi1);
            glm::vec3 v4 = sphericalToCartesian(radius, theta2, phi2);

            // Triangle 1: v1, v2, v3
            vertices.insert(vertices.end(), { v1.x, v1.y, v1.z,
                                              v2.x, v2.y, v2.z,
                                              v3.x, v3.y, v3.z });

            // Triangle 2: v2, v4, v3
            vertices.insert(vertices.end(), { v2.x, v2.y, v2.z,
                                              v4.x, v4.y, v4.z,
                                              v3.x, v3.y, v3.z });
        }
    }
    return vertices;
}


int main() {

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Request OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int width = 2000;
    const int height = 1000;
    GLFWwindow* window = glfwCreateWindow(width, height, "Planet Simulation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);


    unsigned int vertexShaderObj = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderObj, 1, &vertexShader, NULL);
    glCompileShader(vertexShaderObj);

    unsigned int fragmentShaderObj = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderObj, 1, &fragmentShader, NULL);
    glCompileShader(fragmentShaderObj);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShaderObj);
    glAttachShader(shaderProgram, fragmentShaderObj);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShaderObj);
    glDeleteShader(fragmentShaderObj);

    

    float sphereRadius = 100.0f;
    int stacks = 64;
    int sectors = 64;
    std::vector<float> sphere = GenerateSphere(sphereRadius, stacks, sectors);
    GLsizei sphereVertexCount = static_cast<GLsizei>(sphere.size() / 3);

    // Create VAO/VBO
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sphere.size() * sizeof(float), sphere.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    // GL state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);


    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    
   
    float aspect = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 500.0f), // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),   // Look at point
        glm::vec3(0.0f, 1.0f, 0.0f)    // Up vector
    );

    while (!glfwWindowShouldClose(window)) {
        // Basic input
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform3f(lightPosLoc, 200.0f, 200.0f, 200.0f);

        glm::vec3 camPos(0.0f, 0.0f, 500.0f);
        glUniform3f(viewPosLoc, camPos.x, camPos.y, camPos.z);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);


        // Set view and projection matrices
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        //Draw Sphere
        // Create model matrix for sphere
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // Position at origin
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); // Uniform scale, might be radius in those brackets 
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glUniform3f(objectColorLoc, 0.1f, 0.6f, 1.0f);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

    }



    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;


}
