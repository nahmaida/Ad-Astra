#include <../include/empire.h>
#include <../include/map.h>
using namespace std;

// Список возможных названий империй
const vector<string> empireNames = {
    "Королевство Андреева", "Королевство Борисова", "Королевство Васильева",
    "Королевство Герасимова", "Королевство Данилова", "Королевство Евгениева",
    "Королевство Захарова", "Королевство Иванова", "Королевство Кириллова"};

// Конструктор для создания империи с заданными параметрами
Empire::Empire(int id, string name, habitableType preferredType, System *homeSystem) {
    this->id = id;
    this->name = name;
    this->preferredType = preferredType;
    this->systems = {homeSystem};
}

// Конструктор копирования
Empire::Empire(const Empire &other) {
    this->id = other.id;
    this->name = other.name;
    this->preferredType = other.preferredType;
    this->systems = other.systems;
}

// Конструктор с заданным ID
Empire::Empire(int id) {
    this->id = id;
    this->name = "";
    this->preferredType = habitableType::Континентальный;
    this->systems = {};
}

// Получение ID империи
int Empire::getId() const { return this->id; }

// Получение названия империи
string Empire::getName() const { return this->name; }

// Получение предпочтительного типа планеты
habitableType Empire::getPreferredType() const { return this->preferredType; }

// Получение систем, принадлежащих империи
vector<System *> Empire::getSystems() const { return this->systems; }

void Empire::addSystem(System *system) { this->systems.push_back(system); }

void Empire::removeSystem(System *system) {
    auto it = std::find(this->systems.begin(), this->systems.end(), system);
    if (it != this->systems.end()) {
        this->systems.erase(it);
    }
}

// Заполнение данных империи
void Empire::fill(Galaxy &galaxy) {
    vector<System*> gsystems = galaxy.getSystems();
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(gsystems.begin(), gsystems.end(), g);

    // Находим систему с обитаемыми планетами
    for (System* system : gsystems) {
        if (system->hasHabitables()) {
            this->systems.push_back(system);
            break;
        }
    }

    // Устанавливаем предпочтительный тип планеты
    if (!this->systems.empty()) {
        for (Planet &planet : this->systems[0]->getPlanets()) {
            if (planet.isHabitable()) {
                this->preferredType = planet.getType();
                break;
            }
        }
    }

    // Задаем случайное имя из списка
    this->name = empireNames[rand() % empireNames.size()];
}

// Оператор присваивания
Empire &Empire::operator=(const Empire &other) {
    if (this != &other) {
        this->id = other.id;
        this->name = other.name;
        this->preferredType = other.preferredType;
        this->systems = other.systems;
    }
    return *this;
}

// Деструктор для освобождения ресурсов
Empire::~Empire() {
    this->systems.clear();
    this->name.clear();
    this->preferredType = habitableType::Континентальный;
    this->id = 0;
}
