#include <iostream>
#include <cmath>
#include <variant>
#include <vector>
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>


/*=============Error Message Definition=============*/

std::string Error_Message = "An error has occured";

/*=============Struct for 2D vectors=============*/

struct vec2{
    double x;
    double y;
};

/*=============Planet Class Definition=============*/
class Planet{


    public:
        std::string name;
        double mass;
        vec2 position;
        vec2 velocity;
        vec2 acceleration;
        bool trailPath;
        double radius;


        
        Planet(std::string name, double mass, vec2 position, vec2 velocity, vec2 acceleration, bool trailPath){
        this->name = name;
        this->mass = mass;
        this->position = position;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->trailPath = trailPath;
        this->radius = mass/100;
        }        

            
        double distanceToPlanet(vec2 otherPlanet){
            double distance;
            distance = sqrt(pow((position.x - otherPlanet.x),2) + pow((position.y - otherPlanet.y),2));
            return distance; 
            }


        std::variant<double, std::string> applyGravitationalForce(double otherMass, vec2 otherPosition){
            if (mass > 0){
                double Force;
                Force = (100) * ((mass*otherMass) / pow((distanceToPlanet(otherPosition)),2));
                return Force;    
            }
            else{
                return Error_Message;
            }
        };
            
        double calculateAngle(vec2 otherPlanet){
            double dx = otherPlanet.x - position.x;
            double dy = otherPlanet.y - position.y;
            return atan2(dy, dx);
        }

        vec2 calculateResultantForce(double Force, double angle){
            vec2 resultantForce;
            resultantForce.x = Force * cos(angle);
            resultantForce.y = Force * sin(angle);
            return resultantForce;
        }

        void updateAcceleration(vec2 otherPosition, double otherMass){
            vec2 resultantForce;
            resultantForce = calculateResultantForce(std::get<double>(applyGravitationalForce(otherMass, otherPosition)), calculateAngle(otherPosition) );
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
  


int main(){

    Planet circle1("CircleOne", 1500, vec2{0,0}, vec2{0,0}, vec2{0,0}, true);
    Planet circle2("CircleTwo", 900, vec2{-300,0}, vec2{0,-40}, vec2{0,0}, true);
    std::vector<Planet> planets;
    planets.push_back(circle1);
    planets.push_back(circle2);
    Planet circle3("CircleThree", 2000, vec2{100,0}, vec2{0,40}, vec2{0,0}, true);
    planets.push_back(circle3);

    
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW\n";
            return -1;
        }

        // Request an older/compatibility context so immediate mode (glBegin/glEnd) works on platforms
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);


        const int width = 1000;
        const int height = 700;
        GLFWwindow* window = glfwCreateWindow(width, height, "Planet Simulation", NULL, NULL);
        if (!window){
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return -1;
        }
        glfwMakeContextCurrent(window);

        // Set up viewport and coordinate system
        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // Use a centered coordinate system so wrap-around math using width/2 and height/2 works
        glOrtho(-width/2, width/2, -height/2, height/2, -1, 1);
        glMatrixMode(GL_MODELVIEW);

        // Set a clear color for the background
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        // Use a time step so movement is frame-rate independent
        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(window)) {

            double currentTime = glfwGetTime();
            double dt = currentTime - lastTime;
            if (dt <= 0.0) dt = 0.0001;
            lastTime = currentTime;



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


            // Update physics using the time step:
            // apply acceleration scaled by dt to velocity, and move position by velocity * dt
            /*
            for(int i = 0; i < planets.size(); i++){
                planets[i].updateVelocity(vec2{planets[i].acceleration.x * dt, planets[i].acceleration.y * dt});
                planets[i].updatePosition(vec2{planets[i].velocity.x * dt, planets[i].velocity.y * dt});
            };
            */

            for (int i = 0; i <planets.size();i++){
                if (planets[i].position.x > width/2 + planets[i].radius) planets[i].position.x = -width/2 - planets[i].radius;
                if (planets[i].position.x < -width/2 - planets[i].radius) planets[i].position.x = width/2 + planets[i].radius;
                if (planets[i].position.y > height/2 + planets[i].radius) planets[i].position.y = -height/2 - planets[i].radius;
                if (planets[i].position.y < -height/2 - planets[i].radius) planets[i].position.y = height/2 + planets[i].radius;
            }
            

            glClear(GL_COLOR_BUFFER_BIT);

            // Reset modelview so object coordinates are drawn in world space
            glLoadIdentity();


            for (int i = 0; i <planets.size();i++){
                glBegin(GL_TRIANGLE_FAN);
                glColor3f(1.0f, 0.5f, 0.5f);
                glVertex2f(static_cast<float>(planets[i].position.x), static_cast<float>(planets[i].position.y));
                for (int j = 0; j <= 100; j++) {
                    float angle = j * 2.0f * static_cast<float>(M_PI) / 100.0f;
                    glVertex2f(static_cast<float>(planets[i].position.x) + cosf(angle) * planets[i].radius, static_cast<float>(planets[i].position.y) + sinf(angle) * planets[i].radius);
                }
                glEnd();
            };




            glfwSwapBuffers(window);
            glfwPollEvents();
        }

    glfwTerminate();
    return 0;    
}

