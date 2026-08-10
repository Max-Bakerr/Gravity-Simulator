#include <iostream>
#include <cmath>
#include <variant>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include <OpenGL/gl3.h>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

float G = 750000.0f; // Gravitational constant


struct vec2{
    float x;
    float y;
    float z = 0.0f;
};


/*=============Error Message Definition=============*/
std::string Error_Message = "An error has occured";

class Label{
    public:
        std::string text;
        float x;
        float y;

        Label(std::string text = "", const glm::mat4& projection = glm::mat4(1.0f), 
              const glm::mat4& view = glm::mat4(1.0f), float planetX = 0.0f, 
              float planetY = 0.0f, int width = 0, int height = 0){
            this->text = text;
            this->x = 0.0f;
            this->y = 0.0f;
            UpdateLabelPosition(projection, view, planetX, planetY, width, height);
            
        }

        void UpdateLabelPosition(const glm::mat4& projection, const glm::mat4& view, float planetX, float planetY, int width, int height) {
            glm::vec4 clipSpacePos = projection * view * glm::vec4(planetX, planetY, 0.0f, 1.0f);
            glm::vec3 ndcPos = glm::vec3(clipSpacePos) / clipSpacePos.w;
            
            x = (ndcPos.x * 0.5f + 0.5f) * width;
            y = (1.0f - (ndcPos.y * 0.5f + 0.5f)) * height;
        }

        void DrawLabel(int width, int height, ImGuiWindowFlags labelFlags) {
            ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
            ImGui::Begin(text.c_str(), nullptr, labelFlags);
            ImGui::Text("%s", text.c_str());
            ImGui::End();
        }
};


class TrailPath{
    public:
        std::vector<float> TrailVertices;
        int maxLength;
        int currentLength;
        unsigned int vao, vbo;


    TrailPath(){
        this->maxLength = 1000;
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

    void DrawTrail(unsigned int shaderProgram, int objectColorLoc) {
        glUniform4f(objectColorLoc, 1.0f, 1.0f, 0.0f, 1.0f); // Yellow color for trail
        glUniform1i(glGetUniformLocation(shaderProgram, "objectType"), 2);
        glBindVertexArray(vao);
        glDrawArrays(GL_LINE_STRIP, 0, currentLength);
        glBindVertexArray(0);
    }

    void Init(){
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
        TrailPath trail;
        Label label;
        
        Planet(std::string name, double mass, vec2 position, vec2 velocity, vec2 acceleration, bool trailPath){
        this->name = name;
        this->mass = mass;
        this->position = position;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->trailPath = trailPath;
        this->radius = mass/50;
        this->sphereVertices = GenerateSphere(64, 64);
        this->sphereVertexCount = static_cast<int>(sphereVertices.size() / 3);
        // MakeVBOVAO() will be called after OpenGL context is ready
        this->label.text = name;
        }        

            
        float distanceToPlanet(vec2 otherPlanet){
            float distance;
            distance = sqrt(pow((position.x - otherPlanet.x),2) + pow((position.y - otherPlanet.y),2));
            return distance; 
            }


        std::variant<float, std::string> applyGravitationalForce(float otherMass, vec2 otherPosition){
            if (mass > 0){
                float Force;
                Force = G * ((mass*otherMass) / pow((distanceToPlanet(otherPosition)),2));
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

            glUniform3f(objectColorLoc, 0.392f, 0.706f, 1.0f);
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
        std::vector<float> baseVertices;

        void CreateGrid() {
            float size = 20000.0f;
            float divisions = 100;
            float step = size / divisions;
            float halfSize = 20000.0f/2.0f;


            // x axis

            for (int zStep = 0; zStep <= 0; ++zStep) {
                float z =  zStep * step;
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
                    float z = zStep * step;
                    for (int yStep = 0; yStep < divisions; ++yStep) {
                        float yStart = -halfSize + yStep * step;
                        float yEnd = yStart + step;
                        vertices.push_back(x); vertices.push_back(yStart); vertices.push_back(z);
                        vertices.push_back(x); vertices.push_back(yEnd);   vertices.push_back(z);
                    }
                }
            }
            vertexCount = vertices.size() / 3;

            baseVertices = vertices;
        
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

        void UpdateGrid(std::vector<Planet>& planets){
            vertices = baseVertices;

            float k = 240000.0f; // Gravitational constant for grid distortion
            float c = 1000.0f;    // Damping factor to control distortion intensity

            for (int i = 0; i < vertices.size(); i += 3){
                float x = vertices[i];
                float y = vertices[i+1];
                float z = vertices[i+2];

                float totalDisplacement = 0.0f;

                for(int j = 0; j < planets.size(); j++){
                    float dx = x - planets[j].position.x;
                    float dy = y - planets[j].position.y;
                    float distance = sqrt(dx*dx + dy*dy); // Avoid division by zero
                    

                    float displacement = 0.01*(-k * planets[j].mass) / (pow(distance, 1) + c); // Gravitational distortion formula
                    totalDisplacement += displacement;
                    /*if (totalDisplacement < -1500.0f){
                        totalDisplacement = -1500.0f;
                    }
                    */
                }

                vertices[i+2] = z + totalDisplacement; // Update z-coordinate based on total displacement

            }

        }

        ~SpaceTimeGrid() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }


};

//=============Global Varibles=============
std::string EducationalMessage;
std::vector<Planet> planets;



//==============Define Functions===============
void DecideEducationalMessage(const std::vector<Planet>& planets);

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
    uniform int objectType = 1; // true for planet

    void main() {
        FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Default magenta for debugging;

        if (objectType == 1){
            FragColor = vec4(1.0, 1.0, 1.0, 0.25); // Green for grid
        }else if (objectType == 0){
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
            FragColor = vec4(result, 1.0); // Yellow for planet
        }else if (objectType == 2){
            FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Yellow for trail
        }


    }
)";
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// Global variables for the main loop
struct AppContext {
    GLFWwindow* window;
    unsigned int shaderProgram;
    GLuint objectColorLoc;
    GLuint lightPosLoc;
    GLuint lightColorLoc;
    GLuint viewPosLoc;
    GLuint projLoc;
    GLuint viewLoc;
    glm::mat4 projection;
    SpaceTimeGrid* grid;
    double lastTime;
    int width;
    int height;
    
    // UI state
    char name[128];
    float newPositionX[1];
    float newPositionY[1];
    float newVelocityX[1];
    float newVelocityY[1];
    float newMass;
    bool WithTrailPath;
    bool IsRunning;
    int selectedPlanetIndex;
    float cameraX, cameraY, cameraZ;
    ImGuiWindowFlags labelFlags;


};

void main_loop(void* arg);

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);

    const int width = 2000;
    const int height = 1000;
    GLFWwindow* window = glfwCreateWindow(width, height, "Planet Simulation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.10f, 0.8f);
    colors[ImGuiCol_Header]   = ImVec4(0.2f, 0.4f, 0.8f, 0.55f);
    colors[ImGuiCol_Button]   = ImVec4(0.2f, 0.4f, 0.8f, 0.6f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.5f, 1.0f, 0.8f);
    colors[ImGuiCol_Text]     = ImVec4(0.9f, 0.9f, 1.0f, 1.0f);
    style.FrameRounding = 6.0f;
    style.WindowRounding = 8.0f;

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
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    GLuint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
    GLuint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    GLuint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
    GLuint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor"); 
    glUseProgram(shaderProgram);

    float aspect = (float)width / (float)height;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 60000.0f);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    SpaceTimeGrid* grid = new SpaceTimeGrid();
    grid->CreateGrid();
    grid->MakeVBOVAO();

    AppContext* ctx = new AppContext();
    ctx->window = window;
    ctx->shaderProgram = shaderProgram;
    ctx->objectColorLoc = objectColorLoc;
    ctx->lightPosLoc = lightPosLoc;
    ctx->lightColorLoc = lightColorLoc;
    ctx->viewPosLoc = viewPosLoc;
    ctx->projLoc = projLoc;
    ctx->viewLoc = viewLoc;
    ctx->projection = projection;
    ctx->grid = grid;
    ctx->lastTime = glfwGetTime();
    ctx->width = width;
    ctx->height = height;
    
    strcpy(ctx->name, "Placeholder");
    ctx->newPositionX[0] = 0.0f;
    ctx->newPositionY[0] = 0.0f;
    ctx->newVelocityX[0] = 0.0f;
    ctx->newVelocityY[0] = 0.0f;
    ctx->newMass = 1.0f;
    ctx->WithTrailPath = false;
    ctx->IsRunning = true;
    ctx->selectedPlanetIndex = -1;
    ctx->cameraX = 15000.0f;
    ctx->cameraY = 0.0f;
    ctx->cameraZ = 15000.0f;
    ctx->labelFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                      ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar;

#ifdef __EMSCRIPTEN__
    // Cleanup function for Emscripten
    auto cleanup = []() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        // shaderProgram, grid, and ctx are not accessible here, so consider making them global or static if needed
        glDeleteProgram(shaderProgram);
        delete grid;
        delete ctx;
        glfwTerminate();
    };
    emscripten_atexit(cleanup);
    emscripten_set_main_loop_arg(main_loop, ctx, 0, 1);
#else
    while (!glfwWindowShouldClose(window)) {
        main_loop(ctx);
    }
    
    delete grid;
    delete ctx;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteProgram(shaderProgram);
    glfwTerminate();
#endif

    return 0;
}

void main_loop(void* arg) {
    AppContext* ctx = (AppContext*)arg;
    
    glfwPollEvents(); // Move poll events to start
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(ctx->shaderProgram);
    
    // Set projection and view BEFORE drawing
    glm::vec3 cameraPos(ctx->cameraX, ctx->cameraY, ctx->cameraZ);
    glm::vec3 cameraLookAt(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp(0.0f, 0.0f, 1.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraLookAt, cameraUp);
    
    glUniformMatrix4fv(ctx->projLoc, 1, GL_FALSE, glm::value_ptr(ctx->projection));
    glUniformMatrix4fv(ctx->viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    
    glUniform3f(ctx->lightPosLoc, 3000.0f, 10000.0f, 0.0f);
    glUniform3f(ctx->lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(ctx->viewPosLoc, ctx->cameraX, ctx->cameraY, ctx->cameraZ);

    ImGui::SetNextWindowPos(ImVec2(ctx->width - 300, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, ctx->height), ImGuiCond_Always);
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove;
    ImGui::Begin("Planet Creator", nullptr, window_flags);
    ImGui::Text("Create a new planet:");

    ImGui::InputText("Planet Name", ctx->name, 128);
    ImGui::Text("Position:");
    ImGui::SliderFloat("Position X: ", ctx->newPositionX, -10000.0f, 10000.0f);
    ImGui::SliderFloat("Position Y: ", ctx->newPositionY, -10000.0f, 10000.0f);
    ImGui::Text("Velocity:");
    ImGui::SliderFloat("Velocity X: ", ctx->newVelocityX, -500.0f, 500.0f);
    ImGui::SliderFloat("Velocity Y: ", ctx->newVelocityY, -500.0f, 500.0f);
    ImGui::Text("Mass:");
    ImGui::SliderFloat("Mass", &ctx->newMass, 50.0f, 1200.0f);
    ImGui::Checkbox("Trail Path", &ctx->WithTrailPath);
    ImGui::Text("Click to add planet with above parameters");

    if (ImGui::Button("Add Planet")) {
        vec2 newPosition = {ctx->newPositionX[0], ctx->newPositionY[0]};
        vec2 newVelocity = {ctx->newVelocityX[0], ctx->newVelocityY[0]};
        vec2 newAcceleration = {0.0f, 0.0f};
        Planet newPlanet(ctx->name, ctx->newMass, newPosition, newVelocity, newAcceleration, true);
        planets.push_back(newPlanet);
        planets.back().sphereVertices = planets.back().GenerateSphere(64, 64);
        planets.back().MakeVBOVAO();
        if (ctx->WithTrailPath){
            planets.back().trail.Init();
            planets.back().trail.AddPoint(ctx->newPositionX[0], ctx->newPositionY[0]);
        }
        planets.back().label = Label(ctx->name, ctx->projection, view, newPosition.x, newPosition.y, ctx->width, ctx->height); 
    }

    ImGui::SeparatorText("Simulation Control:");
    if (ImGui::Button(ctx->IsRunning ? "Pause Simulation" : "Resume Simulation")) {
        ctx->IsRunning = !ctx->IsRunning;
    }
    if (ImGui::Button("Reset Simulation")) {
        planets.clear();
    }

    ImGui::Text("Select Planet to Delete:");
    if (ImGui::BeginCombo("     ", (ctx->selectedPlanetIndex >= 0 && ctx->selectedPlanetIndex < planets.size()) ? planets[ctx->selectedPlanetIndex].name.c_str() : "None")){
        for (int i = 0; i < planets.size(); i++){
            bool SelectedPlanet = (ctx->selectedPlanetIndex == i);
            if (ImGui::Selectable(planets[i].name.c_str(), SelectedPlanet)){
                ctx->selectedPlanetIndex = i;
            }
        }
        ImGui::EndCombo();
    }

    if (ctx->selectedPlanetIndex >= 0 && ctx->selectedPlanetIndex < planets.size()){
        if (ImGui::Button("Delete Selected Planet")){
            planets.erase(planets.begin() + ctx->selectedPlanetIndex);
            ctx->selectedPlanetIndex = -1;
        }
    }

    DecideEducationalMessage(planets);
    ImGui::SeparatorText("Educational Information");
    ImGui::TextWrapped("%s", EducationalMessage.c_str());

    ImGui::SeparatorText("Camera Control");
    ImGui::TextWrapped("Forwards/Backwards: ");
    ImGui::SliderFloat(" ", &ctx->cameraX, -30000.0f, 30000.0f);
    ImGui::TextWrapped("Left/Right: ");
    ImGui::SliderFloat("   ", &ctx->cameraY, -30000.0f, 30000.0f);
    ImGui::TextWrapped("Up/Down: ");
    ImGui::SliderFloat("  ", &ctx->cameraZ, -30000.0f, 30000.0f);

    ImGui::End();

    float currentTime = glfwGetTime();
    float dt = currentTime - ctx->lastTime;
    if (dt <= 0.0) dt = 0.0001;
    ctx->lastTime = currentTime;

    for(int i = 0; i < planets.size(); i++){
        planets[i].acceleration = vec2{0,0};
        if (ctx->IsRunning){
            for(int j = 0; j < planets.size(); j++){
                if (i != j){
                    planets[i].updateAcceleration(planets[j].position, planets[j].mass);
                }
            }
            planets[i].updateVelocity(vec2{planets[i].acceleration.x * dt, planets[i].acceleration.y * dt});
            planets[i].updatePosition(vec2{planets[i].velocity.x * dt, planets[i].velocity.y * dt});
        }
        planets[i].DrawPlanet(ctx->shaderProgram, ctx->objectColorLoc);
    }

    ctx->grid->UpdateGrid(planets);
    ctx->grid->MakeVBOVAO();
    ctx->grid->DrawGrid(ctx->shaderProgram, ctx->objectColorLoc);

    for(int i = 0; i < planets.size(); i++){
        if (ctx->IsRunning){
            planets[i].trail.AddPoint(planets[i].position.x, planets[i].position.y);
        }
        planets[i].trail.UpdateBuffer();
        planets[i].trail.DrawTrail(ctx->shaderProgram, ctx->objectColorLoc);
    }

    for (int i = 0; i < planets.size(); i++){
        planets[i].label.UpdateLabelPosition(ctx->projection, view, planets[i].position.x, planets[i].position.y, ctx->width, ctx->height);
        planets[i].label.DrawLabel(ctx->width, ctx->height, ctx->labelFlags);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(ctx->window);
}


void DecideEducationalMessage(const std::vector<Planet>& planets){
    if(planets.size() == 0){
        EducationalMessage = "Welcome to my planet simulation. To get started, use the panel on the right to create a new planet by specifying its name, position, velocity, and mass. You can also choose to enable a trail path to visualize the planet's trajectory.";
    }
    else if (planets.size() == 1){
        EducationalMessage = "You've added your first planet! Observe how it the space-time grid distorts around it, simulating gravitational effects. This shows how mass doesn't just sit in space-time - it tells space-time how to curve, and curved space-time tells mass how to move. Add another planet to see gravitational interactions!";
    }
    else if (planets.size() >= 2){
        EducationalMessage = "With multiple planets in the simulaiton, you can see how they influence each others trajectories through gravitational forces. Each planet's mass distorts the space-time grid, affecting the paths of nearby planets. Experiment with different masses and initial velocities to explore various orbital dynamics!";
    }
}