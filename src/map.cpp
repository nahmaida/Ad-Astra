#include "../include/map.h"
using namespace std;

const double phi = (1 + sqrt(5)) / 2;
vector<string> starNames = {"Alpha",  "Beta", "Gamma", "Delta", "Epsilon",
                            "Zeta",   "Eta",  "Theta", "Iota",  "Kappa",
                            "Lambda", "Mu",   "Nu",    "Xi",    "Omicron",
                            "Pi",     "Rho",  "Sigma", "Tau",   "Upsilon",
                            "Phi",    "Chi",  "Psi",   "Omega"};

Resources::Resources(resource name, int amount) : name(name), amount(amount) {}

MapPoint::MapPoint(int x, int y) : x(x), y(y) {}
MapPoint::MapPoint() : x(0), y(0) {}

Line::Line(MapPoint s, MapPoint e) : start(s), end(e) {}

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
vector<Resources> addResources(vector<Resources> one, vector<Resources> two) {
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
CelestialBody::CelestialBody(string name, int size, vector<Resources> resources,
                             Empire owner)
    : name(name), size(size), resources(resources), owner(owner) {}

vector<Resources> CelestialBody::getResources() { return resources; }
string CelestialBody::getName() const { return name; }
Empire CelestialBody::getOwner() const { return owner; }
int CelestialBody::getSize() const { return size; }

// планета. звезда тоже своего рода планета.
Planet::Planet(string name, bool isHabitable)
    : CelestialBody(name, 0, {}, Empire()),
      habitable(isHabitable),
      population(0),
      type(static_cast<habitableType>(0)) {}

void Planet::fill() {
    // случайный размер планеты от 10 до 30
    this->size = rand() % 30 + 10;

    // 50% что будут минералы
    if (rand() % 2) {
        int amount = rand() % 10 + 1;  // кол-во минералов от 1 до 10
        resources.push_back(Resources(Минералы, amount));
    }

    // 33% что будет энергия
    if (!(rand() % 3)) {
        int amount = rand() % 10 + 1;  // кол-во энергии от 1 до 10
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
    : CelestialBody(name, size, {}, Empire()),
      planets({}),
      location(MapPoint()) {}

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
        resources = addResources(resources, planet->getResources());
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
MapPoint System::getLocation() const { return location; }
vector<Planet> System::getPlanets() const { return planets; }

// галактика с вектором систем
Galaxy::Galaxy(int size) : size(size), systems({}), connections({}), map({}) {}

// заполняет галактику
void Galaxy::fill() {
    for (int i = 0; i < size; i++) {
        // выбирает случайное название и кол-во планет
        string name = starNames[rand() % starNames.size()];
        int planetCount = rand() % 6 + 1;

        System system(name, planetCount + 1);

        // Заполняет каждую систему
        system.fill();
        systems.push_back(system);
    }

    // генерирует расположение для каждой системы
    vector<MapPoint> points = sunflower(systems.size());
    for (int j = 0; j < points.size(); j++) {
        systems[j].setLocation(points[j]);
    }

    // соединяет системы с соседями
    connectSystems();

    // генерирует карту
    generateMap();
}

// выводит карту
void Galaxy::printMap() const {
    // печатаем карту
    for (const auto& row : map) {
        for (const auto& cell : row) {
            cout << cell;
        }
        cout << endl;
    }
}

vector<System> Galaxy::getSystems() const { return systems; }
vector<Line> Galaxy::getConnections() const { return connections; }
int Galaxy::getSize() const { return size; }

void Galaxy::connectSystems() {
    set<pair<int, int>>
        addedConnections;  // чтобы избежать дублирования соединений
    const int connectionRadius =
        10;  // максимальная дистанция для поиска соседей
    random_device rd;
    mt19937 rng(rd());  // генератор случайных чисел

    for (int i = 0; i < systems.size(); i++) {
        vector<int> nearbySystems;

        // поиск соседей в пределах радиуса
        MapPoint currentLocation = systems[i].getLocation();
        for (int j = 0; j < systems.size(); j++) {
            if (i == j) continue;  // Пропуск самой себя
            MapPoint neighborLocation = systems[j].getLocation();

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
        int numConnections = min((int)nearbySystems.size(), rand() % 3 + 1);
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
            MapPoint start = systems[i].getLocation();
            MapPoint end = systems[neighbor].getLocation();
            connections.emplace_back(start, end);
        }
    }
}

void Galaxy::drawLine(vector<vector<string>>& map, MapPoint start,
                      MapPoint end) const {
    int x0 = start.x, y0 = start.y;
    int x1 = end.x, y1 = end.y;

    // алгоритм Брезенхэма для рисования линии
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if (x0 >= 0 && x0 < map.size() && y0 >= 0 && y0 < map[0].size() &&
            map[x0][y0] == " ") {
            map[x0][y0] = ".";  // отметка пути линии
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Galaxy::generateMap() {
    const int maxHeight = 50;  // максимальная высота карты
    const int maxWidth = 150;  // максимальная ширина карты

    // динамическое создание карты
    vector<vector<string>> map(maxHeight, vector<string>(maxWidth, " "));

    // рисуем системы
    for (const System& system : systems) {
        string uninhabitableName = "S";  // желтая если нет обитаемых систем
        string inhabitableName = "H";  // зеленая если есть
        string name =
            (system.hasHabitables()) ? inhabitableName : uninhabitableName;

        MapPoint location = system.getLocation();

        // пропорционально масштабируем координаты под размер карты
        int scaledX = (location.x * maxHeight) / 100;
        int scaledY = (location.y * maxWidth) / 100;

        if (scaledX >= 0 && scaledX < maxHeight && scaledY >= 0 &&
            scaledY < maxWidth) {
            map[scaledX][scaledY] = name;
        }
    }

    // рисуем соединения
    for (const Line& line : connections) {
        MapPoint start((line.start.x * maxHeight) / 100,
                       (line.start.y * maxWidth) / 100);
        MapPoint end((line.end.x * maxHeight) / 100,
                     (line.end.y * maxWidth) / 100);
        drawLine(map, start, end);
    }

    this->map = map;
}

string Galaxy::getMap() const {
    string mapString;
    for (const auto& row : map) {
        for (const auto& cell : row) {
            mapString += cell;
        }
        mapString += '\n';
    }
    return mapString;
}