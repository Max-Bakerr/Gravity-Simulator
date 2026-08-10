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
        this->vao = 0;
        this->vbo = 0;
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
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (vao != 0) glDeleteVertexArrays(1, &vao);
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

            
        float distanceToPlanet(vec2 otherPlanetPosition){
            float distance;
            distance = sqrt(pow((position.x - otherPlanetPosition.x),2) + pow((position.y - otherPlanetPosition.y),2));
            return distance; 
        }


        std::variant<float, std::string> applyGravitationalForce(float otherMass, vec2 otherPosition){
            if (mass > 0){
                float Force;
                Force = (10000) * ((mass*otherMass) / pow((distanceToPlanet(otherPosition)),2));
                return Force;    
            }
            else{
                return Error_Message;
            }
        }
            
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

            glUniform3f(objectColorLoc, 0.1f, 0.6f, 1.0f);
            glUniform1i(glGetUniformLocation(shaderProgram, "objectType"), 0); // Set objectType to 1 for planet

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
            glBindVertexArray(0);

        }

        ~Planet() {
            if (VAO != 0) glDeleteVertexArrays(1, &VAO);
            if (VBO != 0) glDeleteBuffers(1, &VBO);
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
            for (int i = 0; i < vertices.size(); i++){
                std::cout << vertices[i] << " ";
                std::cout << "\n";
                if ((i+1) % 3 == 0){
                    std::cout << "\n";
                }
            }

        }

        ~SpaceTimeGrid() {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }


};


int main(){

    Planet test1("Test 1", 1000.0f, {100,100}, {45,54}, {0,0}, true);
    std::cout << "Planet Name: " << test1.name << std::endl;
    std::cout << "Planet Mass: " << test1.mass << std::endl;
    std::cout << "Radius: " << test1.radius << std::endl;
    std::cout << "Position X: " << test1.position.x << std::endl;
    std::cout << "Position Y: " << test1.position.y << std::endl;
    std::cout << "Velocity X: " << test1.velocity.x << std::endl;
    std::cout << "Velocity Y: " << test1.velocity.y << std::endl;
    std::cout << "Trail Path: " << test1.trailPath << std::endl;

    return 0;
}
    