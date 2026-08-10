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
                Force = (6.67 * pow(10,-11)) * ((mass*otherMass) / pow((distanceToPlanet(otherPosition)),2));
                return Force;    
            }
            else{
                return Error_Message;
            }
        };
            
        double calculateAngle(vec2 otherPlanet){
            double Angle;
            Angle = atan(abs(position.x - otherPlanet.x)/abs(position.y - otherPlanet.y));
            return Angle;
        }

        vec2 calculateResultantForce(double Force, double angle){
            vec2 resultantForce;
            resultantForce.x = Force * sin(angle);
            resultantForce.y = Force * cos(angle);
            return resultantForce;
        }

        void updateAcceleration(vec2 otherPosition, double otherMass){
            vec2 resultantForce;
            resultantForce = calculateResultantForce(std::get<double>(applyGravitationalForce(otherMass, otherPosition)), calculateAngle(otherPosition) );
            acceleration.x = resultantForce.x / mass;
            acceleration.y = resultantForce.y / mass;
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
  