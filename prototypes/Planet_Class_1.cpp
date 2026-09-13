#include <iostream>
#include <cmath>

struct Position{
    double x;
    double y;
};

struct Velocity{
    double x;
    double y;
};

struct Acceleration{
    double x;
    double y;
};

struct ResultantForce{
    double x;
    double y;
};


class Planet{
    public:
        std::string name;
        double mass;
        Position position;
        Velocity velocity;
        Acceleration acceleration;
        bool trailPath;
        double radius;

        Planet(std::string name, double mass, Position position, Velocity velocity, Acceleration acceleration, bool trailPath){
        this->name = name;
        this->mass = mass;
        this->position = position;
        this->velocity = velocity;
        this->acceleration = acceleration;
        this->trailPath = trailPath;
        }

        int getRadius(int mass){
            radius = mass * 1;   // as mentioned in design, a constant, k, will decide the value for the radius. For now I am using 1 for simplicity.  
            //feel like this could be in the constructor, for version 2 maybe 
            return 0;
        }

        double distanceToPlanet(Position planet2){
            double distance;
            distance = sqrt(pow((planet2.x-position.x),2) + pow((planet2.y-position.y),2));
            return distance; 
            // this function calculates the distance to a given planet. To be used in the force calculations 
        }

        double applyGravitationalForce(double mass, double mass2, double distance){
            double Force;
            Force = (6.67 * pow(10,-11)) * ((mass*mass2) / pow(distance,2));
            return Force;
        }

        double calculateAngle(Position position, Position planet2){
            double Angle;
            Angle = atan(abs(position.x - planet2.x)/abs(position.y - planet2.y));
            return Angle;
        }

        ResultantForce calculateResultantForce(double Force, double angle){
            ResultantForce resultantForce;
            resultantForce.x = Force * sin(angle);
            resultantForce.y = Force * cos(angle);
            return resultantForce;
            
        }

        Velocity updateVelocity(Acceleration acceleration){
            velocity.x = velocity.x + acceleration.x;
            velocity.y = velocity.y + acceleration.y;
            // this comes from using the SUVAT equation v = u + at. However, as this is being done each time step, t = 1.
            return velocity;
        }

        Position updatePosition(Velocity velocity){
            position.x = position.x + velocity.x;
            position.y = position.y + velocity.y;
            //Similarly comes from motion physics, which is simplified due to this process happening every time-step
            return position;
        }

};

int main(){
    std::string name;
    double mass;
    Position position;
    Velocity velocity;
    Acceleration acceleration;
    bool trailPath;


    std::cout << "This is the planet creator, you will create a new planet: \n";
    std::cout << "Firstly input the name of your planet: ";
    std::cin >> name;
    std::cout << "\n";
    
    std::cout << "Next input the mass of your planet: ";
    std::cin >> mass;
    std::cout << "\n";
    std::cout << "Next input the x coordinate of your planet: ";
    std::cin >> position.x;
    std::cout << "\n";
    std::cout << "Next input the y coordinate of your planet: ";
    std::cin >> position.y;
    std::cout << "\n";
    std::cout << "Next input the x coordinate of the velocity of your planet: ";
    std::cin >> velocity.x;
    std::cout << "\n";
    std::cout << "Next input the y coordinate of the velocity of your planet: ";
    std::cin >> velocity.y;
    std::cout << "\n";
    std::cout << "Next input the x coordinate of the acceleration of your planet: ";
    std::cin >> acceleration.x;
    std::cout << "\n";
    std::cout << "Next input the y coordinate of the acceleration of your planet: ";
    std::cin >> acceleration.y;
    std::cout << "\n";
    std::cout << "Do you want a trail path for your planet? (true or false) ";
    std::cin >> trailPath;
    std::cout << "\n";
    

    Planet Planet1(name,mass,  position,  velocity,  acceleration,  trailPath);

    std::cout << Planet1.name;
    std::cout << Planet1.mass;

}