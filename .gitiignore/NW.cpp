#include <iostream>
#include <cmath>
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
    uniform vec4 objectColor;

    void main() {
        FragColor = objectColor;    
    
    }
)";


GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource);

std::string Error_Message = "An error has occured";

struct vec2{
    float x;
    float y;
    float z = 0.0f;
};



class SpaceTimeGrid{
    public:
        std::vector<float> vertices;
        GLuint VAO, VBO;
        size_t vertexCount = 0;

        void CreateGrid() {
            float size = 20000.0f;
            float divisions = 25;
            float step = size / divisions;
            float halfSize = 20000.0f/2.0f;


            // x axis

            for (int zStep = 0; zStep <= 0; ++zStep) {
                float z = -halfSize*0.3f + zStep * step;
                for (int yStep = 0; yStep <= divisions; ++yStep) {
                    float y = -halfSize + yStep * step;
                    for (int xStep = 0; xStep < divisions; ++xStep) {
                        float xStart = -halfSize + xStep * step;
                        float xEnd = xStart + step;
                        vertices.push_back(xStart); vertices.push_back(y); vertices.push_back(z);
                        vertices.push_back(xEnd);   vertices.push_back(y); vertices.push_back(z);
                    }
                }
            }
            
            for (int xStep = 0; xStep <= divisions; ++xStep) {
                float x = -halfSize + xStep * step;
                for (int zStep = 0; zStep <= 0; ++zStep) {
                    float z = -halfSize*0.3f + zStep * step;
                    for (int yStep = 0; yStep < divisions; ++yStep) {
                        float yStart = -halfSize + yStep * step;
                        float yEnd = yStart + step;
                        vertices.push_back(x); vertices.push_back(yStart); vertices.push_back(z);
                        vertices.push_back(x); vertices.push_back(yEnd);   vertices.push_back(z);
                    }
                }
            }
            vertexCount = vertices.size() / 3;
        
        }   
        
        void MakeVBOVAO(){
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            //glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
        }


        void DrawGrid(GLuint shaderProgram, GLuint objectColorLoc) {
            glUseProgram(shaderProgram);

            glm::mat4 model = glm::mat4(1.0f);             
            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

            glBindVertexArray(VAO);
            glLineWidth(10.0f); 
            glDrawArrays(GL_LINES, 0, vertexCount); 
            glBindVertexArray(0);
        }


};




int main(){
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Create a windowed mode window and its OpenGL context

    const int width = 2000;
    const int height = 1000;
    GLFWwindow* window = glfwCreateWindow(width, height, "Space-Time Grid", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    
    //Were using because used in Kavan's one, not any more 
    // GLuint shaderProgram = CreateShaderProgram(vertexShader, fragmentShader);
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
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUseProgram(shaderProgram);

    //projection matrix
    float aspect = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 750000.0f);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    SpaceTimeGrid grid;
    grid.CreateGrid();
    grid.MakeVBOVAO();


    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 cameraPos(0.0f, 0.0f, 30000.0f);
        glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
        glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


        grid.DrawGrid(shaderProgram, objectColorLoc);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


        glDeleteVertexArrays(1, &grid.VAO);
        glDeleteBuffers(1, &grid.VBO);

        glDeleteProgram(shaderProgram);
        glfwTerminate();

        glfwTerminate();
        return 0;

}



