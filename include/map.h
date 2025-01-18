#ifndef MAP_H
#define MAP_H

#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <random>
#include <set>
#include <string>
#include <vector>
#include <sstream>

// шанс того что планета будет обитаемой = 1/HAB_CHANCE * 100%
// т.е если HAB_CHANCE = 10, то шанс 10%
#define HAB_CHANCE 15
#define MINERAL_PRICE 2
#define METAL_PRICE 10
#define GOODS_PRICE 10

using namespace std;

extern const double phi;
// вектор возможных названий для систем
extern vector<string> starNames;

// enum возможных типов обитаемых планет
enum habitableType {
    Пустынный,
    Сухой,
    Саванна,
    Океанический,
    Континентальный,
    Тропический,
    Арктический,
    Горный,
    Тундра
};

// enum ресурсов
enum resource { Минералы, Энергия, Металлы, Блага };

// класс ресурсов
class Resources {
   public:
    resource name;
    int amount;
    Resources(resource name, int amount);
    int getPrice() const;
};

// структ для зданий
struct Building {
    string name;
    string description;
    vector<Resources> output;
};

// класс точки
class MapPoint {
   public:
    int x, y;
    MapPoint(int x, int y);
    MapPoint();
};

// структ линии
struct Line {
    MapPoint start, end;
    Line(MapPoint s, MapPoint e);
};

// вирт класс для звездных тел
class CelestialBody {
   protected:
    string name;
    int size;
    vector<Resources> resources;
   public:
    CelestialBody(string name, int size, vector<Resources> resources);
    virtual void fill() = 0;
    vector<Resources> getResources();
    string getName() const;
    int getSize() const;
};

// класс планет
class Planet : public CelestialBody {
   private:
    bool habitable;
    vector<Building> buildings;
    int population;
    habitableType type;

   public:
    Planet(string name, bool isHabitable = false);
    void fill() override;
    bool isHabitable() const;
    int getPopulation() const;
    vector<Building> getBuildings() const;
    habitableType getType() const;
};

// класс систем
class System : public CelestialBody {
   private:
    vector<Planet> planets;
    MapPoint location;
    int id;
    int power;

   public:
    System(string name, int size);
    void fill() override;
    bool hasHabitables() const;
    void setLocation(MapPoint point);
    void setId(int id);
    void setPower(int power);
    int getId() const;
    int getPower() const;
    MapPoint getLocation() const;
    vector<Planet> getPlanets() const;
};

// класс галактии
class Galaxy {
   private:
    int size;
    vector<System> systems;
    vector<Line> connections;

    void connectSystems();
    void drawLine(vector<vector<string>>& map, MapPoint start, MapPoint end) const;
    void generateMap();

   public:
    Galaxy(int size);
    void fill();
    void printMap() const;
    vector<System> getSystems() const;
    vector<Line> getConnections() const;
    string getMap() const;
    int getSize() const;
};

// функции
void loadStarnames(string filename);
vector<MapPoint> sunflower(int n, int alpha = 0, bool geodesic = false);
vector<Resources> addResources(vector<Resources> one, vector<Resources> two);
vector<System*> getNeighbors(System* targetSystem, Galaxy galaxy);

#endif  // MAP_H
