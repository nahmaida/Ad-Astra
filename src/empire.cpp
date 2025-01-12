#include <../include/empire.h>
#include <../include/map.h>
using namespace std;

const vector<string> empireNames = {
    "Королевство Андреева",   "Королевство Борисова",  "Королевство Васильева",
    "Королевство Герасимова", "Королевство Данилова",  "Королевство Евгениева",
    "Королевство Захарова",   "Королевство Иванова",   "Королевство Кириллова",
    "Королевство Михайлова",  "Королевство Николаева", "Королевство Олегова",
    "Королевство Павлова",    "Королевство Романова",  "Королевство Сергеева",
    "Королевство Тимофеева",  "Королевство Федорова",  "Королевство Харитонова",
    "Королевство Юрьева",     "Королевство Яковлева"};

Empire::Empire(int id, string name, habitableType preferredType, System *homeSystem) {
    this->id = id;
    this->name = name;
    this->preferredType = preferredType;
    this->systems = {homeSystem};
}

Empire::Empire(const Empire& other) {
    this->id = other.id;
    this->name = other.name;
    this->preferredType = other.preferredType;
    this->systems = other.systems;
}

Empire::Empire(int id) {
    this->id = id;
    this->name = "";
    this->preferredType = habitableType::Континентальный;
    this->systems = {};
}

int Empire::getId() const { return this->id; }
string Empire::getName() const { return this->name; }
habitableType Empire::getPreferredType() const { return this->preferredType; }
vector<System*> Empire::getSystems() const { return this->systems; }

void Empire::fill(Galaxy galaxy) {
    // Выбираем случайную систему в качестве домашней
    vector<System> gsystems = galaxy.getSystems();
    while (this->systems.size() < 1) {
        int systemIndex = rand() % gsystems.size();
        if (gsystems[systemIndex].hasHabitables()) {
            this->systems.push_back(&gsystems[systemIndex]);
        }
    }

    // Добавляем тип
    for (Planet &planet : this->systems[0]->getPlanets()) {
        if (planet.isHabitable()) {
            this->preferredType = planet.getType();
        }
    }

    // Выбирааем имя
    this->name = empireNames[rand() % empireNames.size()];
}

Empire& Empire::operator=(const Empire& other) {
    if (this != &other) {
        this->id = other.id;
        this->name = other.name;
        this->preferredType = other.preferredType;
        this->systems = other.systems;
    }
    return *this;
}

Empire::~Empire() {
    this->systems.clear();
    this->systems.shrink_to_fit();
    this->name.clear();
    this->name.shrink_to_fit();
    this->preferredType = habitableType::Континентальный;
    this->id = 0;
}