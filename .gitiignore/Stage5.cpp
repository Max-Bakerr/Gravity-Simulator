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

/*=============Vec2 Definition=============*/

struct vec2{
    float x;
    float y;
    float z = 0.0f;
};

/*=============Trail Path Class Definition=============*/

class TrailPath{
    public:
        std::vector<float> TrailVertices;
        int maxLength;
        int currentLength;
        unsigned int vao, vbo;


    TrailPath(){
        this->maxLength = 10000000;
        this->currentLength = 0;
    }

    void AddPoint(float x, float y){
        if (currentLength < maxLength){
            TrailVertices.push_back(x);
            TrailVertices.push_back(y);
            TrailVertices.push_back(0.0f); // z-coordinate
            currentLength++;
        }
        else{
            //Remove oldest point
            TrailVertices.erase(TrailVertices.begin(), TrailVertices.begin() + 3);
            //Add new point
            TrailVertices.push_back(x);
            TrailVertices.push_back(y);
            TrailVertices.push_back(0.0f); // z-coordinate
        }
    }

    void Init() {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        // Allocate empty buffer with maximum capacity
        glBufferData(GL_ARRAY_BUFFER, maxLength * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void UpdateBuffer() {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, TrailVertices.size() * sizeof(float), TrailVertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Destructor to clean up VAO and VBO
    ~TrailPath() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

};

/*=============Planet Class Definition=============*/
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



};

/*=============Shader Source Code=============*/

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

int main() {

    std::vector<Planet> planets;
    Planet circle1("CircleOne", 5000, vec2{0,0}, vec2{0,40}, vec2{0,0}, true);
    std::vector<TrailPath> trails;

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

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 

    // Setup ImGui bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");


    // Create and compile shaders
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

    // Create VAO and VBO
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, circle1.sphereVertices.size() * sizeof(float), circle1.sphereVertices.data(), GL_STATIC_DRAW);


    glEnable(GL_DEPTH_TEST);

    //code for 3D rendering setup
    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor"); 

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

   
    float aspect = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 10000.0f);


    double lastTime = glfwGetTime();

    char name[128] = "Placeholder"; 
    float newPositionX[1] = {0.0f};
    float newPositionY[1] = {0.0f};
    float newVelocityX[1] = {0.0f};
    float newVelocityY[1] = {0.0f};
    float newMass = 1.0f;




    while (!glfwWindowShouldClose(window)) {
        


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform3f(lightPosLoc, 0.0f, 3000.0f, 0.0f);

        glm::vec3 camPos(1000.0f, 0.0f, 10000.0f);
        glUniform3f(viewPosLoc, camPos.x, camPos.y, camPos.z);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

        glm::mat4 view = glm::lookAt(
            camPos, // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f),   // Look at point
            glm::vec3(0.0f, 0.0f, 1.0f)    // Up vector
        );


        // Force the window position/size each frame and disable moving
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

        ImGui::SliderFloat("Mass", &newMass, 50.0f, 3000.0f);;
        

        if (ImGui::Button("Add Planet")) {
            vec2 newPosition = {newPositionX[0], newPositionY[0]};
            vec2 newVelocity = {newVelocityX[0], newVelocityY[0]};
            vec2 newAcceleration = {0.0f, 0.0f};
            Planet newPlanet(name, newMass, newPosition, newVelocity, newAcceleration, true);
            planets.push_back(newPlanet);
            planets.back().sphereVertices = planets.back().GenerateSphere(64, 64);
            TrailPath trail;
            trails.push_back(trail);
        }

            
        ImGui::End();

        // Set view and projection matrices
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        float currentTime = glfwGetTime();
            float dt = currentTime - lastTime;
            if (dt <= 0.0) dt = 0.0001;
            lastTime = currentTime;

        


        for(int i = 0; i <planets.size(); i++){
            vec2 previousPosition = planets[i].position;
            planets[i].acceleration = vec2{0,0};
            for(int j = 0; j <planets.size(); j++){
                if (i != j){
                    planets[i].updateAcceleration(planets[j].position, planets[j].mass);
                }
            }
            planets[i].updateVelocity(vec2{planets[i].acceleration.x * dt, planets[i].acceleration.y * dt});
            planets[i].updatePosition(vec2{planets[i].velocity.x * dt, planets[i].velocity.y * dt});
            if (planets[i].trailPath){
                trails[i].AddPoint(previousPosition.x, previousPosition.y);
            }
        }
    
        //Draw planets
        for(int i = 0; i <planets.size();i++){
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(planets[i].position.x, planets[i].position.y, planets[i].position.z)); 
            model = glm::scale(model, glm::vec3(planets[i].radius));    
                  
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

 	        glUniform3f(objectColorLoc, 0.1f, 0.6f, 1.0f);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, planets[i].sphereVertexCount);
            glBindVertexArray(0);

            //rendering trail path:
            /*unsigned int vao, vbo;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, trails[i].TrailVertices.size() * sizeof(float), trails[i].TrailVertices.data(), GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glUniform3f(objectColorLoc, 1.0f, 1.0f, 0.0f); // Yellow color for trail

            glDrawArrays(GL_LINE_STRIP, 0, trails[i].currentLength);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            */
            // In your render loop (each frame):
            // ...existing code...
            trails[i].Init(); // Initialize VAO and VBO for the trail
            glBindVertexArray(trails[i].vao);
            glBindBuffer(GL_ARRAY_BUFFER, trails[i].vbo);

            // Update the buffer with new trail data
            glBufferData(GL_ARRAY_BUFFER, trails[i].TrailVertices.size() * sizeof(float), trails[i].TrailVertices.data(), GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glUniform3f(objectColorLoc, 1.0f, 1.0f, 0.0f);
            glLineWidth(500.0f); // Set line width for better visibility
            glDrawArrays(GL_LINE_STRIP, 0, trails[i].currentLength);

            glBindVertexArray(0);

            trails[i].UpdateBuffer();


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
