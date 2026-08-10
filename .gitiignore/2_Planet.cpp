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
  



int main(){

    Planet circle1("CircleOne", 5000, vec2{100,200}, vec2{60,0}, vec2{9.8,0}, true);
    Planet circle2("CircleTwo", 3000, vec2{100,-200}, vec2{-30,0}, vec2{-9.8,0}, true);

    
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

            // Update physics using the time step:
            // apply acceleration scaled by dt to velocity, and move position by velocity * dt
            circle1.updateVelocity(vec2{circle1.acceleration.x * dt, circle1.acceleration.y * dt});
            circle1.updatePosition(vec2{circle1.velocity.x * dt, circle1.velocity.y * dt});
            circle2.updateVelocity(vec2{circle2.acceleration.x * dt, circle2.acceleration.y * dt});
            circle2.updatePosition(vec2{circle2.velocity.x * dt, circle2.velocity.y * dt});
            // Wrap around screen edges
            // To prevent the circle from disappearing when it moves across the screen, wrap to the opposite side.
            // Include the circle draw radius (40.0) so it wraps when the whole circle leaves the view.
            if (circle1.position.x > width/2 + 40.0) circle1.position.x = -width/2 - 40.0;
            if (circle1.position.x < -width/2 - 40.0) circle1.position.x = width/2 + 40.0;
            if (circle1.position.y > height/2 + 40.0) circle1.position.y = -height/2 - 40.0;
            if (circle1.position.y < -height/2 - 40.0) circle1.position.y = height/2 + 40.0;
            if (circle2.position.x > width/2 + 80.0) circle2.position.x = -width/2 - 80.0;
            if (circle2.position.x < -width/2 - 80.0) circle2.position.x = width/2 + 80.0;
            if (circle2.position.y > height/2 + 80.0) circle2.position.y = -height/2 - 80.0;
            if (circle2.position.y < -height/2 - 80.0) circle2.position.y = height/2 + 80.0;


            glClear(GL_COLOR_BUFFER_BIT);

            // Reset modelview so object coordinates are drawn in world space
            glLoadIdentity();

            // Draw planet (simple circle)
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(0.0f, 0.5f, 1.0f); // Planet color
            glVertex2f(static_cast<float>(circle1.position.x), static_cast<float>(circle1.position.y)); // Center
            for (int i = 0; i <= 100; i++) {
                float angle = i * 2.0f * static_cast<float>(M_PI) / 100.0f;
                glVertex2f(static_cast<float>(circle1.position.x) + cosf(angle) * 40.0f, static_cast<float>(circle1.position.y) + sinf(angle) * 40.0f);
            }
            glEnd();

            glBegin(GL_TRIANGLE_FAN);
            glColor3f(0.0f, 1.0f, 0.0f); // Planet color
            glVertex2f(static_cast<float>(circle2.position.x), static_cast<float>(circle2.position.y)); // Center
            for (int i = 0; i <= 100; i++) {
                float angle = i * 2.0f * static_cast<float>(M_PI) / 100.0f;
                glVertex2f(static_cast<float>(circle2.position.x) + cosf(angle) * 80.0f, static_cast<float>(circle2.position.y) + sinf(angle) * 80.0f);
            }
            glEnd();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

    glfwTerminate();
    return 0;    
}

