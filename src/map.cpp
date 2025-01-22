#include "../include/map.h"
using namespace std;

const double phi = (1 + sqrt(5)) / 2;
vector<string> starNames = {};

Resources::Resources(resource name, int amount) : name(name), amount(amount) {}
int Resources::getPrice() const {
    switch (name) {
        case resource::Минералы:
            return MINERAL_PRICE * amount;
        case resource::Энергия:
            return amount;
        default:
            return 0;
    }
}

MapPoint::MapPoint(int x, int y) : x(x), y(y) {}
MapPoint::MapPoint() : x(0), y(0) {}

Line::Line(MapPoint s, MapPoint e) : start(s), end(e) {}

void loadStarnames(string filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "ОШИБКА: Не удалось открыть файл " << filename << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        starNames.push_back(line);
    }

    file.close();
}

// распределяет n точек в круг
vector<MapPoint> sunflower(int n, int alpha, bool geodesic) {
    vector<MapPoint> points;
    double angleStride = (geodesic) ? 360 * phi : 2 * M_PI / pow(phi, 2);
    int b = round(alpha * sqrt(n));  // кол-во точек на границе

    for (int k = 1; k < n + 1; k++) {
        double radius = sqrt(k - 0.5) / sqrt(n - (b + 1) / 2);
        double theta = k * angleStride;
        int x = round(49.5 * (radius * cos(theta) + 1));
        int y = round(49.5 * (radius * sin(theta) + 1));

        x = max(0, min(99, x));
        y = max(0, min(99, y));
        points.push_back(MapPoint(x, y));
    }
    return points;
}

// прибавляет вместе ресурсы двух тел
vector<Resources> addResources(vector<Resources>& one, vector<Resources>& two) {
    for (int j = 0; j < two.size(); j++) {
        bool found = false;
        for (int i = 0; i < one.size(); i++) {
            if (one[i].name == two[j].name) {
                one[i].amount += two[j].amount;
                found = true;
                break;
            }
        }
        if (!found) one.push_back(two[j]);
    }
    return one;
}

// вирт класс небесное тело
CelestialBody::CelestialBody(string name, int size, vector<Resources> resources)
    : name(name), size(size), resources(resources) {}

vector<Resources> CelestialBody::getResources() { return resources; }
string CelestialBody::getName() const { return name; }
int CelestialBody::getSize() const { return size; }

// планета. звезда тоже своего рода планета.
Planet::Planet(string name, bool isHabitable)
    : CelestialBody(name, 0, {}),
      habitable(isHabitable),
      population(0),
      type(static_cast<habitableType>(0)) {}

void Planet::fill() {
    // случайный размер планеты от 10 до 30
    this->size = rand() % 30 + 10;

    // 50% что будут минералы
    if (rand() % 2) {
        int amount = rand() % 3 + 1;  // кол-во минералов от 1 до 3
        resources.push_back(Resources(Минералы, amount));
    }

    // 33% что будет энергия
    if (!(rand() % 3)) {
        int amount = rand() % 5 + 1;  // кол-во энергии от 1 до 5
        resources.push_back(Resources(Энергия, amount));
    }

    // если планета обитаемая, добавить дополнительные свойства
    if (habitable) {
        // случайный тип планеты
        type = static_cast<habitableType>(rand() % 9);
    }
}

bool Planet::isHabitable() const { return habitable; }
int Planet::getPopulation() const { return population; }
vector<Building> Planet::getBuildings() const { return buildings; }
habitableType Planet::getType() const { return type; }

// звездная система с планетками
System::System(string name, int size)
    : CelestialBody(name, size, {}),
      planets({}),
      location(MapPoint()),
      id(NULL),
      power(100) {}

void System::fill() {
    Planet* star = new Planet("* " + name, false);
    star->fill();
    planets.push_back(*star);
    for (int i = 0; i < size; i++) {
        bool isHabitable =
            (rand() % HAB_CHANCE == 0);  // определяем обитаемость
        Planet* planet = new Planet(name + " " + to_string(i + 1), isHabitable);

        planet->fill();

        // добавляет ресурсы планеты к системе
        vector<Resources> planetResources = planet->getResources();
        resources = addResources(resources, planetResources);
        planets.push_back(*planet);

        delete planet;  // очищаем память
    }
}

// возвращает, есть ли в планете обитаемые системы
bool System::hasHabitables() const {
    for (Planet planet : planets) {
        if (planet.isHabitable()) {
            return true;
        }
    }
    return false;
}

void System::setLocation(MapPoint point) { location = point; }
void System::setId(int id) { this->id = id; }
void System::setPower(int power) { this->power = power; }
int System::getId() const { return id; }
int System::getPower() const { return power; }
MapPoint System::getLocation() const { return location; }
vector<Planet> System::getPlanets() const { return planets; }

// галактика с вектором систем
Galaxy::Galaxy(int size) : size(size), systems({}), connections({}) {}

// Заполняет галактику
void Galaxy::fill() {
    for (int i = 0; i < size; i++) {
        string name = starNames[rand() % starNames.size()];
        int planetCount = rand() % 6 + 1;

        // Создаем новую систему и задаем уникальный ID
        System* system = new System(name, planetCount);
        system->setId(i);

        // Заполняем систему
        system->fill();
        systems.push_back(system);
    }

    // Расставляем системы по координатам
    vector<MapPoint> points = sunflower(systems.size());
    for (size_t j = 0; j < systems.size(); j++) {
        systems[j]->setLocation(points[j]);
    }

    // Создаем соединения между системами
    connectSystems();
}

vector<System*> Galaxy::getSystems() const { return systems; }
vector<Line> Galaxy::getConnections() const { return connections; }
int Galaxy::getSize() const { return size; }

void Galaxy::connectSystems() {
    set<pair<int, int>>
        addedConnections;  // чтобы избежать дублирования соединений
    float connectionRadius = 10;  // максимальная дистанция для поиска соседей
    if (size < 100) {
        connectionRadius = 10.0f / (size / 100.0f);
    }

    random_device rd;
    mt19937 rng(rd());  // генератор случайных чисел

    for (int i = 0; i < systems.size(); i++) {
        vector<int> nearbySystems;

        // поиск соседей в пределах радиуса
        MapPoint currentLocation = systems[i]->getLocation();
        for (int j = 0; j < systems.size(); j++) {
            if (i == j) continue;  // Пропуск самой себя
            MapPoint neighborLocation = systems[j]->getLocation();

            // вычисление евклидова расстояния
            double distance =
                sqrt(pow(neighborLocation.x - currentLocation.x, 2) +
                     pow(neighborLocation.y - currentLocation.y, 2));

            if (distance <= connectionRadius) {
                nearbySystems.push_back(j);
            }
        }

        // перемешивание соседних систем для случайного выбора
        std::shuffle(nearbySystems.begin(), nearbySystems.end(), rng);
        // случайный выбор 1–3 соседей для соединения
        int numConnections = max(1, min((int)nearbySystems.size(), rand() % 3 + 2 - (int) (connectionRadius / 10)));
        for (int k = 0; k < numConnections; k++) {
            int neighbor = nearbySystems[k];

            // избегание дублирования соединений
            if (addedConnections.count({i, neighbor}) ||
                addedConnections.count({neighbor, i})) {
                continue;
            }

            // запись соединения
            addedConnections.insert({i, neighbor});

            // добавление линии между системами
            MapPoint start = systems[i]->getLocation();
            MapPoint end = systems[neighbor]->getLocation();
            connections.emplace_back(start, end);
        }
    }
}

void Galaxy::clear() {
    for (System* system : systems) {
        delete system;
    }
    systems.clear();
    connections.clear();
}

vector<System*> getNeighbors(System* targetSystem, const Galaxy& galaxy) {
    vector<System*> neighbors;
    MapPoint location = targetSystem->getLocation();

    for (Line connection : galaxy.getConnections()) {
        if (connection.start.x == location.x &&
            connection.start.y == location.y) {
            for (System* system : galaxy.getSystems()) {
                if (connection.end.x == system->getLocation().x &&
                    connection.end.y == system->getLocation().y) {
                    neighbors.push_back(system);
                }
            }
        } else if (connection.end.x == location.x &&
                   connection.end.y == location.y) {
            for (System* system : galaxy.getSystems()) {
                if (connection.start.x == system->getLocation().x &&
                    connection.start.y == system->getLocation().y) {
                    neighbors.push_back(system);
                }
            }
        }
    }
    return neighbors;
}
