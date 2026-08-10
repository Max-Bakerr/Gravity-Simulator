#include <iostream>
#include <cmath>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/gl3.h>

struct vec2{
    float x;
    float y;
    float z = 0.0f;
};

/*=============Error Message Definition=============*/
std::string Error_Message = "An error has occured";

const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out float lightIntensity;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vec3 worldPos = (model * vec4(aPos, 1.0)).xyz;
    vec3 normal = normalize(aPos);
    vec3 dirToCenter = normalize(-worldPos);
    lightIntensity = max(dot(normal, dirToCenter), 0.15);})glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in float lightIntensity;
out vec4 FragColor;
uniform vec4 objectColor;
uniform bool isGrid; // Add this uniform
uniform bool GLOW;
void main() {
    if (isGrid) {
        // If it's the grid, use the original color without lighting
        FragColor = objectColor;
    } else if(GLOW){
        FragColor = vec4(objectColor.rgb * 100000, objectColor.a);
    }else {
        // If it's an object, apply the lighting effect
        float fade = smoothstep(0.0, 10.0, lightIntensity*10);
        FragColor = vec4(objectColor.rgb * fade, objectColor.a);
    }})glsl";

bool running = true;
bool pause = true;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float lastX = 400.0, lastY = 300.0;
float yaw = -90;
float pitch =0.0;
float deltaTime = 0.0;
float lastFrame = 0.0;

const double G = 6.6743e-11; // m^3 kg^-1 s^-2
const float c = 299792458.0;
float initMass = float(pow(10, 22));
float sizeRatio = 30000.0f;

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




GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource);
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount);
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount);

std::vector<Planet> planets;
std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Planet>& planets);
//std::vector<float> UpdateGridVertices(std::vector<float> vertices, const std::vector<Planet>& planets);

GLuint gridVAO, gridVBO;


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
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUseProgram(shaderProgram);



    //projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 750000.0f);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    cameraPos = glm::vec3(0.0f, 0.0f, 50000.0f);

    
    planets = {
        //Planet(glm::vec3(3844, 0, 0), glm::vec3(0, 0, 228), 7.34767309*pow(10, 22), 3344),
        //Planet(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), 1.989 * pow(10, 30), 5515, glm::vec4(1.0f, 0.929f, 0.176f, 1.0f)),
       //Planet(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), 5.97219*pow(10, 24), 5515, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f)),
       //Planet(glm::vec3(-5000, 650, -350), glm::vec3(0, 0, 1500), 5.97219*pow(10, 22), 5515, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f)),
       //Planet(glm::vec3(5000, 650, -350), glm::vec3(0, 0, -1500), 5.97219*pow(10, 22), 5515, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f)),
       //Planet(glm::vec3(0, 0, -350), glm::vec3(0, 0, 0), 1.989 * pow(10, 25), 5515, glm::vec4(1.0f, 0.929f, 0.176f, 1.0f), true),

    };
    std::vector<float> gridVertices = CreateGridVertices(20000.0f, 25, planets);
    CreateVBOVAO(gridVAO, gridVBO, gridVertices.data(), gridVertices.size());
    for(int i = 0; i < gridVertices.size(); i+=3){
        std::cout << "Vertex " << i/3 << ": (" << gridVertices[i] << ", " << gridVertices[i+1] << ", " << gridVertices[i+2] << ")\n";
    }

    while (!glfwWindowShouldClose(window) && running == true) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        /*
        if (!planets.empty() && planets.back().Initalizing) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                // increase mass by 1% per second
                planets.back().mass *= 1.0 + 1.0 * deltaTime;
                
                // update radius based on new mass
                objs.back().radius = pow(
                    (3 * objs.back().mass / objs.back().density) / 
                    (4 * 3.14159265359f), 
                    1.0f/3.0f
                ) / sizeRatio;
                
                // update vertex data
                objs.back().UpdateVertices();
            }
        }
        §*/

        // Draw the grid
        glUseProgram(shaderProgram);
        glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 0.25f);
        glUniform1i(glGetUniformLocation(shaderProgram, "isGrid"), 1);
        glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 0);
        //gridVertices = UpdateGridVertices(gridVertices, planets);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_DYNAMIC_DRAW);

        glUseProgram(shaderProgram);
        glm::mat4 model = glm::mat4(1.0f); // Identity matrix for the grid
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


        // Set up view matrix
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


        glBindVertexArray(gridVAO);
        glLineWidth(10.0f); // Make lines more visible
        glDrawArrays(GL_LINES, 0, gridVertices.size() / 3); // vertexCount is number of floats, divide by 3 for vertices
        glBindVertexArray(0);


        // Draw the triangles / sphere
        /*
        for(auto& obj : objs) {
            glUniform4f(objectColorLoc, obj.color.r, obj.color.g, obj.color.b, obj.color.a);

            for(auto& obj2 : objs){
                if(&obj2 != &obj && !obj.Initalizing && !obj2.Initalizing){
                    float dx = obj2.GetPos()[0] - obj.GetPos()[0];
                    float dy = obj2.GetPos()[1] - obj.GetPos()[1];
                    float dz = obj2.GetPos()[2] - obj.GetPos()[2];
                    float distance = sqrt(dx * dx + dy * dy + dz * dz);

                    if (distance > 0) {
                        std::vector<float> direction = {dx / distance, dy / distance, dz / distance};
                        distance *= 1000;
                        double Gforce = (G * obj.mass * obj2.mass) / (distance * distance);
                        

                        float acc1 = Gforce / obj.mass;
                        std::vector<float> acc = {direction[0] * acc1, direction[1]*acc1, direction[2]*acc1};
                        if(!pause){
                            obj.accelerate(acc[0], acc[1], acc[2]);
                        }

                        //collision
                        obj.velocity *= obj.CheckCollision(obj2);
                        std::cout<<"radius: "<<obj.radius<<std::endl;
                    }
                }
            }
            if(obj.Initalizing){
                obj.radius = pow(((3 * obj.mass/obj.density)/(4 * 3.14159265359)), (1.0f/3.0f)) / 1000000;
                obj.UpdateVertices();
            }

            //update positions
            if(!pause){
                obj.UpdatePos();
            }
            
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position); // apply position
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(glGetUniformLocation(shaderProgram, "isGrid"), 0);
            if(obj.glow){
                glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 1);
            } else {
                glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 0);
            }
            
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount / 3);
        }
        */
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    /*
    for (auto& obj : objs) {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteBuffers(1, &obj.VBO);
    }
    */

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);

    glDeleteProgram(shaderProgram);
    glfwTerminate();

    glfwTerminate();
    return 0;
}








/*
GLFWwindow* StartGLU() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW, panic" << std::endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(800, 600, "3D_TEST", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW." << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}
*/



GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}


void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount) {
   glUseProgram(shaderProgram);
   glm::mat4 model = glm::mat4(1.0f); // Identity matrix for the grid
   GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
   glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


   // Set up view matrix
   glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
   GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
   glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


   glBindVertexArray(gridVAO);
   glLineWidth(10.0f); // Make lines more visible
   glDrawArrays(GL_LINES, 0, vertexCount / 3); // vertexCount is number of floats, divide by 3 for vertices
   glBindVertexArray(0);
}

std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Planet>& planets) {
    std::vector<float> vertices;
    float step = size / divisions;
    float halfSize = size / 2.0f;

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
    

    //y axis
    /*
    for (int xStep = 0; xStep <= divisions; ++xStep) {
        float x = -halfSize + xStep * step;
        for (int zStep = 0; zStep <= divisions; ++zStep) {
            float z = -halfSize + zStep * step;
            for (int yStep = 3; yStep < 3; ++yStep) {
                float yStart = -halfSize*0.3f + yStep * step;
                float yEnd = yStart + step;
                vertices.push_back(x); vertices.push_back(yStart); vertices.push_back(z);
                vertices.push_back(x); vertices.push_back(yEnd);   vertices.push_back(z);
            }
        }
    }
    */

    

    return vertices;
}


/*
std::vector<float> UpdateGridVertices(std::vector<float> vertices, const std::vector<Planet>& planets){
    
    // centre of mass calc
    
    float totalMass = 0.0f;
    float comY = 0.0f;
    for (const auto& obj : objs) {
        if (obj.Initalizing) continue;
        comY += obj.mass * obj.position.y;
        totalMass += obj.mass;
    }
    if (totalMass > 0) comY /= totalMass;
    
    float originalMaxY = -std::numeric_limits<float>::infinity();
    for (int i = 0; i < vertices.size(); i += 3) {
        originalMaxY = std::max(originalMaxY, vertices[i+1]);
    }
    

    float verticalShift = comY - originalMaxY;
    std::cout<<"vertical shift: "<<verticalShift<<" |         comY: "<<comY<<"|            originalmaxy: "<<originalMaxY<<std::endl;

    
    for (int i = 0; i < vertices.size(); i += 3) {

        // mass bending space
        glm::vec3 vertexPos(vertices[i], vertices[i+1], vertices[i+2]);
        glm::vec3 totalDisplacement(0.0f);
        for (const auto& obj : planets) {
            //f (obj.Initalizing) continue;

            glm::vec3 toObject = obj.GetPos() - vertexPos;
            float distance = glm::length(toObject);
            float distance_m = distance * 1000.0f;
            float rs = (2*G*obj.mass)/(c*c);

            float dz = 2 * sqrt(rs * (distance_m - rs));
            totalDisplacement.y += dz * 2.0f;
        }
        vertices[i+1] = totalDisplacement.y + -abs(verticalShift);
    }
    
    return vertices;
}

*/
