#include <cmath>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct Ideology {
    int materialism;
    int liberalism;
    int militarism;
    int xenophilia;
};

class Empire {
    string name;
    habitableType preferredType;
    vector<Resource> resources;
    vector<System> systems;
    vector<Resource> revenue;
    Ideology ideology;

   public:
    Empire(string name, habitableType preferredType, Ideology ideology, System homeSystem) {
        this->name = name;
        this->preferredType = preferredType;
        this->ideology = ideology;
        this->systems = {homeSystem};
        this->revenue = {};
        this->resources = {Resources(Энергия, 500), Resources(Минералы, 200),
                           Resources(Металлы, 100), Resources(Блага, 100)};
    }

    void fill() {
        string name = 
    }
};