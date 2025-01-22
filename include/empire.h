#ifndef EMPIRE_H
#define EMPIRE_H
#include "map.h"

using namespace std;

class Empire {
    int id;
    string name;
    habitableType preferredType;
    vector<System*> systems;

    public:
    Empire(int id, string name, habitableType preferredType, System *HomeSystem);
    Empire(const Empire& other);
    Empire(int id);
    int getId() const;
    string getName() const;
    habitableType getPreferredType() const;
    vector<System*> getSystems() const;
    void fill(Galaxy &galaxy);
    void addSystem(System* system);
    void removeSystem(System *system);
    Empire& operator=(const Empire& other);
    ~Empire();
};

#endif // EMPIRE_H