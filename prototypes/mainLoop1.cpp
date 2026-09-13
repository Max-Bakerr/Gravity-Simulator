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
        }        


        std::variant<int, std::string> getRadius(int mass){
            if (mass > 0){
                int radius = mass * 2;   
                return radius;    
            }
            else{
                return Error_Message;
            }
        };
            
        double distanceToPlanet(vec2 thisPlanet, vec2 otherPlanet){
            double distance;
            distance = sqrt(pow((thisPlanet.x - otherPlanet.x),2) + pow((thisPlanet.y - otherPlanet.y),2));
            return distance; 
            }


        std::variant<double, std::string> applyGravitationalForce(double mass, double mass2, double distance){
            if (mass > 0){
                double Force;
                Force = (6.67 * pow(10,-11)) * ((mass*mass2) / pow(distance,2));
                return Force;    
            }
            else{
                return Error_Message;
            }
        };
            
        double calculateAngle(vec2 thisPlanet, vec2 otherPlanet){
            double Angle;
            Angle = atan(abs(thisPlanet.x - otherPlanet.x)/abs(thisPlanet.y - otherPlanet.y));
            return Angle;
        }

        vec2 calculateResultantForce(double Force, double angle){
            vec2 resultantForce;
            resultantForce.x = Force * sin(angle);
            resultantForce.y = Force * cos(angle);
            return resultantForce;
        }

        vec2 calculateAcceleration(vec2 resultantForce, double mass){
            vec2 acceleration;
            acceleration.x = resultantForce.x / mass;
            acceleration.y = resultantForce.y / mass;
            return acceleration;
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

std::vector<Planet> planets;
int numPlanets = planets.size();

void calculateAngles(){
    for (int i = 0; i < numPlanets; i++){
        for (int j = 0; j < numPlanets; j++){
            if (i != j){
                double angle = planets[i].calculateAngle(planets[i].position, planets[j].position);
            }
        }
    }
}

void calculateDistances(){
    for (int i = 0; i < numPlanets; i++){
        for (int j = 0; j < numPlanets; j++){
            if (i != j){
                double distance = planets[i].distanceToPlanet(planets[i].position, planets[j].position);
            }
        }
    }
}

void updateVelocities(){
    for (int i = 0; i < numPlanets; i++){
        planets[i].updateVelocity(planets[i].acceleration);
    }
}

void calculateForces(){
    for (int i = 0; i < numPlanets; i++){
        for (int j = 0; j < numPlanets; j++){
            if (i != j){
                double distance = planets[i].distanceToPlanet(planets[i].position, planets[j].position);
                auto forceVariant = planets[i].applyGravitationalForce(planets[i].mass, planets[j].mass, distance);
                if (std::holds_alternative<double>(forceVariant)){
                    double force = std::get<double>(forceVariant);
                    double angle = planets[i].calculateAngle(planets[i].position, planets[j].position);
                    vec2 resultantForce = planets[i].calculateResultantForce(force, angle);
                }
            }
        }
    }
}

void calculateAccelerations(){
    for (int i = 0; i < numPlanets; i++){
        // Placeholder for resultant force calculation
        vec2 resultantForce = {0, 0}; 
        planets[i].calculateAcceleration(resultantForce, planets[i].mass);
    }
}

void updatePositions(){
    for (int i = 0; i < numPlanets; i++){
        planets[i].updatePosition(planets[i].velocity);
    }
}


int main(){

    Planet earth("Earth", 5.972e24, {1100,1000}, {0,0}, {0,0}, true);
    Planet moon("Moon", 7.342e22, {1000,1000}, {0,10022}, {0,0}, true);

    std::cout << "Planet simulation initialized." << std::endl;

    planets.push_back(earth);
    planets.push_back(moon);
    numPlanets = planets.size();
    
    for (int step = 0; step < 10000; step++){
        calculateForces();
        calculateAccelerations();
        updateVelocities();
        updatePositions();
    
        /* Optional: Print positions for debugging
        std::cout << "Step " << step << ":\n";
        for (const auto& planet : planets){
              std::cout << planet.name << " Position: (" << planet.position.x << ", " << planet.position.y << ")\n";
        }*/
    };


 
// OpenGL/GLFW rendering loop and helpers
// Place this inside main() where indicated.

#include <GLFW/glfw3.h>

// Simple filled circle renderer (immediate mode)
auto drawCircle = [](double cx, double cy, double r, int segments){
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(cx, cy);
    for (int i = 0; i <= segments; ++i){
        double theta = 2.0 * M_PI * double(i) / double(segments);
        double x = r * cos(theta);
        double y = r * sin(theta);
        glVertex2d(cx + x, cy + y);
    }
    glEnd();
};

// Initialize GLFW and create window
if (!glfwInit()){
    std::cerr << "Failed to initialize GLFW\n";
} else {
    const int width = 1000;
    const int height = 700;
    GLFWwindow* window = glfwCreateWindow(width, height, "Planet Simulation", NULL, NULL);
    if (!window){
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
    } else {
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // vsync

        // World -> screen mapping
        double scale = 10; // meters to pixels (adjustable)
        // Center the simulation at the window center
        double cx = width * 0.5;
        double cy = height * 0.5;

        // Main render loop (runs until window closed)
        while (!glfwWindowShouldClose(window)){
            // advance physics one step per frame
            calculateForces();
            calculateAccelerations();
            updateVelocities();
            updatePositions();

            // Clear
            glViewport(0, 0, width, height);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, width, 0, height, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Draw each planet
            for (const auto& p : planets){
                // Map world position (meters) to screen coordinates (pixels)
                double sx = cx + p.position.x * scale;
                double sy = cy + p.position.y * scale;

                // Choose color by name (simple)
                if (p.name == "Earth") glColor3f(0.2f, 0.5f, 1.0f);
                else if (p.name == "Moon") glColor3f(0.8f, 0.8f, 0.8f);
                else glColor3f(1.0f, 0.6f, 0.2f);

                // Radius on screen: scale mass-derived radius or fallback
                double screenRadius = std::max(4.0, p.radius * 0.05); // if radius was set; otherwise small circle
                // If radius not set in objects, derive from mass for visualization
                if (screenRadius <= 4.0){
                    screenRadius = std::max(4.0, std::log10(p.mass) * 0.5);
                }

                drawCircle(sx, sy, screenRadius, 48);

                // Optionally draw simple trail point
                if (p.trailPath){
                    glPointSize(2.0f);
                    glBegin(GL_POINTS);
                    glColor3f(1.0f, 1.0f, 1.0f);
                    glVertex2d(sx, sy);
                    glEnd();
                }
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

    return 0;
}