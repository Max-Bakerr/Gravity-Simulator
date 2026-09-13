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


/*=============Error Message Definition=============*/
std::string Error_Message = "An error has occured";


struct vec2{
    float x;
    float y;
};


class Planet{


    public:
        std::string name;
        float mass;
        vec2 position;
        vec2 velocity;
        vec2 acceleration;
        bool trailPath;
        float radius;


        
        Planet(std::string name, double mass, vec2 position, vec2 velocity, vec2 acceleration, bool trailPath){
        this->name = name;
        this->mass = mass;
        this->position = position;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->trailPath = trailPath;
        this->radius = mass/100;
        }        

            
        float distanceToPlanet(vec2 otherPlanet){
            float distance;
            distance = sqrt(pow((position.x - otherPlanet.x),2) + pow((position.y - otherPlanet.y),2));
            return distance; 
            }


        std::variant<float, std::string> applyGravitationalForce(float otherMass, vec2 otherPosition){
            if (mass > 0){
                float Force;
                Force = (100) * ((mass*otherMass) / pow((distanceToPlanet(otherPosition)),2));
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


};

/*=============Shader Source Code=============*/
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    uniform mat4 projection;
    uniform vec2 offset;
    uniform float radius;
    void main() {
        vec2 pos = aPos * radius + offset;
        gl_Position = projection * vec4(pos, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() {
        FragColor = vec4(color, 1.0);
    }
)";

int main() {

    /*
    Planet circle1("CircleOne", 5000, vec2{0,0}, vec2{0,40}, vec2{0,0}, true);
    Planet circle2("CircleTwo", 3000, vec2{-300,0}, vec2{0,-40}, vec2{0,0}, true);
    Planet circle3("CircleThree", 2000, vec2{100,0}, vec2{10,0}, vec2{0,0}, true);
    */
   
    std::vector<Planet> planets;
    
    /*
    planets.push_back(circle1);
    planets.push_back(circle2);
    planets.push_back(circle3);
    */

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // Request OpenGL 3.3 core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int width = 1000;
    const int height = 700;
    GLFWwindow* window = glfwCreateWindow(width, height, "Planet Simulation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 

    // Setup ImGui bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Initialize circle vertices
    std::vector<float> vertices;
    const int segments = 32;
    for(int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * M_PI / segments;
        vertices.push_back(cos(angle));
        vertices.push_back(sin(angle));
    }

    // Create and compile shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create VAO and VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Create projection matrix
    glm::mat4 projection = glm::ortho(-width*0.5f, width*0.5f, -height*0.5f, height*0.5f, -1.0f, 1.0f);

    double lastTime = glfwGetTime();

    


    while (!glfwWindowShouldClose(window)) {
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        char name[128] = "Placeholder"; 
        float newPositionX[1] = {0.0f};
        float newPositionY[1] = {0.0f};
        float newVelocityX[1] = {0.0f};
        float newVelocityY[1] = {0.0f};
        float newMass = 1.0f;

        // Force the window position/size each frame and disable moving
        /*ImGui::SetNextWindowPos(ImVec2(width - 300, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, height), ImGuiCond_Always);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;
        ImGui::Begin("Planet Creator", nullptr, window_flags);
        */

        ImGui::Begin("Planet Creator");
        ImGui::Text("Create a new planet:");

        ImGui::InputText("Planet Name", name, 128);
        ImGui::Text("Position:");
        ImGui::SliderFloat("X: ", newPositionX, -10.0f, 10.0f);
        ImGui::SliderFloat("Y: ", newPositionY, -10.0f, 10.0f);

        ImGui::Text("Velocity:");
        ImGui::SliderFloat("Velocity", newVelocityX, -5.0f, 5.0f);
        ImGui::SliderFloat("Velocity", newVelocityY, -5.0f, 5.0f);
        ImGui::SliderFloat("Mass", &newMass, 0.1f, 100.0f);
        
        

        if (ImGui::Button("Add Planet")) {
        
            vec2 newPosition = {newPositionX[0], newPositionY[0]};
            vec2 newVelocity = {newVelocityX[0], newVelocityY[0]};
            vec2 newAcceleration = {0.0f, 0.0f};
            Planet newPlanet(name, newMass, newPosition, newVelocity, newAcceleration, true);
            
            planets.push_back(newPlanet);
        }

            
        ImGui::End();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // Set projection matrix
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.5f, 0.5f);

        float currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            if (dt <= 0.0) dt = 0.0001;
            lastTime = currentTime;


        for (int i = 0; i <planets.size();i++){
            if (planets[i].position.x > width/2 + planets[i].radius) planets[i].position.x = -width/2 - planets[i].radius;
            if (planets[i].position.x < -width/2 - planets[i].radius) planets[i].position.x = width/2 + planets[i].radius;
            if (planets[i].position.y > height/2 + planets[i].radius) planets[i].position.y = -height/2 - planets[i].radius;
            if (planets[i].position.y < -height/2 - planets[i].radius) planets[i].position.y = height/2 + planets[i].radius;
        }


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


        // Draw planets
        for(int i = 0; i <planets.size();i++){
            glUniform2f(glGetUniformLocation(shaderProgram, "offset"), 
                static_cast<float>(planets[i].position.x), 
                static_cast<float>(planets[i].position.y));
            glUniform1f(glGetUniformLocation(shaderProgram, "radius"), 
                static_cast<float>(planets[i].radius));
            
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
