#include <iostream>
#include <cmath>
#include <variant>
#include <vector>


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

    Planet earth("Earth", 5.972e24, {0,0}, {0,0}, {0,0}, true);
    Planet moon("Moon", 7.342e22, {384400000,0}, {0,1022}, {0,0}, true);

    std::cout << "Planet simulation initialized." << std::endl;

    planets.push_back(earth);
    planets.push_back(moon);
    numPlanets = planets.size();

    for (int step = 0; step < 10; step++){
        calculateForces();
        calculateAccelerations();
        updateVelocities();
        updatePositions();
        std::cout << "Step " << step << ":\n";
        for (const auto& planet : planets){
            std::cout << planet.name << " Position: (" << planet.position.x << ", " << planet.position.y << ")\n";
        }
    };


 


    return 0;
}