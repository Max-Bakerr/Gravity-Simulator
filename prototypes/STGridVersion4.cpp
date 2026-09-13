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
    uniform int objectType = 1; // 0 for grid, 1 for planet

    void main() {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Default magenta for debugging;

        /*
        if (objectType == 1){
            FragColor = objectColor;    
        }else if (objectType == 0){
        // Ambient shading
        float ambientStrength = 1.0;
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
        vec3 result = (ambient + diffuse + specular) * objectColor.rgb;
        FragColor = vec4(result, objectColor.a);
        }
        */
    }
)";

struct vec2{
    float x;
    float y;
    float z = 0.0f;
};

std::string Error_Message = "An error has occured";


class Planet{


    public:
        std::string name;
        float mass;
        vec2 position;
        vec2 velocity;
        vec2 acceleration;
        bool trailPath;
        float radius;
        int sphereVertexCount;
        std::vector<float> sphereVertices;
        unsigned int VAO, VBO;
        
        Planet(std::string name, double mass, vec2 position, vec2 velocity, vec2 acceleration, bool trailPath){
        this->name = name;
        this->mass = mass;
        this->position = position;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->trailPath = trailPath;
        this->radius = mass/100;
        this->sphereVertices = GenerateSphere(64, 64);
        this->sphereVertexCount = static_cast<int>(sphereVertices.size() / 3);
        MakeVBOVAO();

        }        

            
        float distanceToPlanet(vec2 otherPlanet){
            float distance;
            distance = sqrt(pow((position.x - otherPlanet.x),2) + pow((position.y - otherPlanet.y),2));
            return distance; 
            }


        std::variant<float, std::string> applyGravitationalForce(float otherMass, vec2 otherPosition){
            if (mass > 0){
                float Force;
                Force = (1000) * ((mass*otherMass) / pow((distanceToPlanet(otherPosition)),2));
                return Force;    
            }
            else{
                return Error_Message;
            }
        };
            
        float calculateAngle(vec2 otherPlanet){
            float dx = otherPlanet.x - position.x;
            float dy = otherPlanet.y - position.y;
            return atan2(dy, dx);
        }

        vec2 calculateResultantForce(float Force, float angle){
            vec2 resultantForce;
            resultantForce.x = Force * cos(angle);
            resultantForce.y = Force * sin(angle);
            return resultantForce;
        }

        void updateAcceleration(vec2 otherPosition, float otherMass){
            vec2 resultantForce;
            resultantForce = calculateResultantForce(std::get<float>(applyGravitationalForce(otherMass, otherPosition)), calculateAngle(otherPosition) );
            acceleration.x = acceleration.x + resultantForce.x / mass;
            acceleration.y = acceleration.y + resultantForce.y / mass;
        }

        vec2 updateVelocity(vec2 acceleration){
            velocity.x = velocity.x + acceleration.x;
            velocity.y = velocity.y + acceleration.y;
            return velocity;
        }

        vec2 updatePosition(vec2 velocity){
            position.x = position.x + velocity.x;
            position.y = position.y + velocity.y;
            return position;
        }

        glm::vec3 sphericalToCartesian(float theta, float phi)
        {
            float x = radius * sinf(theta) * cosf(phi);
            float y = radius * cosf(theta);
            float z = radius * sinf(theta) * sinf(phi);
            return glm::vec3(x, y, z);
        }
       

        std::vector<float> GenerateSphere(int stacks, int sectors)
        {
            std::vector<float> vertices;
            vertices.reserve(stacks * sectors * 6 * 3); // 2 triangles per quad, 3 verts each, 3 comps

            for (int i = 0; i < stacks; ++i) {
                float theta1 = (float)i / (float)stacks * glm::pi<float>();
                float theta2 = (float)(i + 1) / (float)stacks * glm::pi<float>();

                for (int j = 0; j < sectors; ++j) {
                    float phi1 = (float)j / (float)sectors * glm::two_pi<float>();
                    float phi2 = (float)(j + 1) / (float)sectors * glm::two_pi<float>();

                    glm::vec3 v1 = sphericalToCartesian(theta1, phi1);
                    glm::vec3 v2 = sphericalToCartesian(theta1, phi2);
                    glm::vec3 v3 = sphericalToCartesian(theta2, phi1);
                    glm::vec3 v4 = sphericalToCartesian(theta2, phi2);

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
        
        void MakeVBOVAO(){
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
        }

        void DrawPlanet(GLuint shaderProgram, GLuint objectColorLoc) {
            glUseProgram(shaderProgram);
        
            glm::mat4 model = glm::mat4(1.0f);             
            
            model = glm::translate(model, glm::vec3(position.x, position.y, position.z));  
            model = glm::scale(model, glm::vec3(radius));    
            GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

 	        glUniform4f(objectColorLoc, 0.1f, 0.6f, 1.0f, 1.0f);
            glUniform1i(glGetUniformLocation(shaderProgram, "objectType"), 0); // Set objectType to 1 for planet

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
            glBindVertexArray(0);

        }

        ~Planet() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }




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
            glUniform1i(glGetUniformLocation(shaderProgram, "objectType"), 1); // Set objectType to 0 for grid

            glBindVertexArray(VAO);
            glLineWidth(10.0f); 
            glDrawArrays(GL_LINES, 0, vertexCount); 
            glBindVertexArray(0);
        }


};




int main(){
    //==================Initialize GLFW==================
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

    //==================Initialize ImGui==================
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    
    //==================Create Shader Program==================
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

    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUseProgram(shaderProgram);

    //==================Setup Projection Matrix==================
    float aspect = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100000.0f);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));


    //==================Create Space-Time Grid==================
    SpaceTimeGrid grid;
    grid.CreateGrid();
    grid.MakeVBOVAO();

    //==================Create Planets==================
    std::vector<Planet> planets;

    //==================Main Loop Setup==================
    double lastTime = glfwGetTime();

    //==================ImGui Defaults==================
    char name[128] = "Placeholder"; 
    float newPositionX[1] = {0.0f};
    float newPositionY[1] = {0.0f};
    float newVelocityX[1] = {0.0f};
    float newVelocityY[1] = {0.0f};
    float newMass = 1.0f;


    while (!glfwWindowShouldClose(window)) {


        //==================ImGui Frame Start==================
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        //==================Set up Window==================
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform3f(lightPosLoc, 0.0f, 3000.0f, 0.0f);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        glUniform3f(viewPosLoc, 0.0f, 0.0f, 5000.0f);

        glm::vec3 cameraPos(0.0f, 0.0f, 5000.0f);
        glm::vec3 cameraFront(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraFront, cameraUp);
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        //==================Set up ImGui in Loop==================
        ImGui::SetNextWindowPos(ImVec2(width - 300, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, height), ImGuiCond_Always);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;
        ImGui::Begin("Planet Creator", nullptr, window_flags);
        ImGui::Text("Create a new planet:");

        ImGui::InputText("Planet Name", name, 128);
        ImGui::Text("Position:");
        ImGui::SliderFloat("Position X: ", newPositionX, -500.0f, 500.0f);
        ImGui::SliderFloat("Position Y: ", newPositionY, -350.0f, 350.0f);

        ImGui::Text("Velocity:");
        ImGui::SliderFloat("Velocity X: ", newVelocityX, -100.0f, 100.0f);
        ImGui::SliderFloat("Velocity Y: ", newVelocityY, -100.0f, 100.0f);

        ImGui::SliderFloat("Mass", &newMass, 50.0f, 10000.0f);

        //==================Add Planet Button==================
        if (ImGui::Button("Add Planet")) {
        
            vec2 newPosition = {newPositionX[0], newPositionY[0]};
            vec2 newVelocity = {newVelocityX[0], newVelocityY[0]};
            vec2 newAcceleration = {0.0f, 0.0f};
            Planet newPlanet(name, newMass, newPosition, newVelocity, newAcceleration, true);
            planets.push_back(newPlanet);

        }

        ImGui::End();

        //==================Time Calculation==================
        float currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            if (dt <= 0.0) dt = 0.0001;
            lastTime = currentTime;

        //==================Calculate Planet Movements==================
        for(int i = 0; i <planets.size(); i++){
            planets[i].acceleration = vec2{0,0};
            for(int j = 0; j <planets.size(); j++){
                if (i != j){
                    planets[i].updateAcceleration(planets[j].position, planets[j].mass);
                }
            }
            planets[i].updateVelocity(vec2{planets[i].acceleration.x * dt, planets[i].acceleration.y * dt});
            planets[i].updatePosition(vec2{planets[i].velocity.x * dt, planets[i].velocity.y * dt});
        }

        //==================Draw Planets==================
        for(int i = 0; i <planets.size();i++){

            planets[i].DrawPlanet(shaderProgram, objectColorLoc);
        }
        //==================Draw Space-Time Grid==================
        
        grid.DrawGrid(shaderProgram, objectColorLoc);

        //==================ImGui Frame End==================
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


        //==================Swap Buffers and Poll Events==================
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


        //==================Cleanup==================
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glDeleteVertexArrays(1, &grid.VAO);
        glDeleteBuffers(1, &grid.VBO);
        glDeleteProgram(shaderProgram);
        glfwTerminate();

        glfwTerminate();
        return 0;

}

