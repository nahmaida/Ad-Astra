#include "../include/main.h"

#include "main.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int FONT_SIZE = 22;
const int DELAY = 2000;  // Интервал обновления силы (в миллисекундах)
const int FPS = 30;
const int CELL_SIZE = 8;
const int HOVER_SIZE = 12;
float difficulty = 1;

int main(int argc, char *args[]) {
    loadStarnames("/home/nahmaida/Ad-Astra/res/starnames.txt");
    int size = 100;
    cout << "Введите размер галактики (от 90, 100 - рекомендуемый)\n>> ";
    cin >> size;

    Galaxy galaxy(size);
    galaxy.fill();

    vector<Empire *> empires;
    while (empires.size() < 9) {
        generateEmpire(empires, galaxy);
    }

    cout << "Введите сложность (от 1 до 5)\n>> ";
    cin >> difficulty;
    difficulty = 1 / difficulty;

    displayGalaxy(galaxy, empires);

    for (Empire *empire : empires) {
        delete empire;
    }

    return 0;
}

void displayGalaxy(const Galaxy &galaxy, vector<Empire *> &empires) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
             << endl;
        return;
    }

    SDL_Window *window = SDL_CreateWindow("Ad Astra!", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                                          SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Window could not be created! SDL_Error: " << SDL_GetError()
             << endl;
        SDL_Quit();
        return;
    }

    if (TTF_Init() == -1) {
        cerr << "SDL_ttf could not initialize! SDL_ttf Error: "
             << TTF_GetError() << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError()
             << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    vector<System> systems = galaxy.getSystems();
    const auto &connections = galaxy.getConnections();

    TTF_Font *font = TTF_OpenFont(
        "/home/nahmaida/Ad-Astra/res/bytebounce_medium.ttf", FONT_SIZE);
    if (!font) {
        cerr << "Failed to load font: " << TTF_GetError() << endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return;
    }

    bool quit = false;
    SDL_Event e;
    System *hoveredSystem = nullptr;
    System *selectedSystem = nullptr;

    vector<pair<System *, System *>> powerTransfers = {};
    unordered_map<const Empire *, SDL_Color> empireColors;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(50, 255);

    for (const Empire *empire : empires) {
        empireColors[empire] = SDL_Color{static_cast<Uint8>(dist(rng)),
                                         static_cast<Uint8>(dist(rng)),
                                         static_cast<Uint8>(dist(rng)), 255};
    }

    // Задержка для силы
    Uint32 lastPowerUpdate = SDL_GetTicks();
    const Uint32 powerUpdateInterval = DELAY;  // 2000 ms

    // 30 FPS
    const Uint32 frameDelay = 1000 / FPS;  // ~33 ms
    Uint32 frameStart, frameTime;

    Uint32 lastConquerUpdate = SDL_GetTicks();
    const Uint32 conquerUpdateInterval = 2000;  // 2000 ms

    while (!quit) {
        frameStart = SDL_GetTicks();

        unordered_map<int, SDL_Color> systemColors =
            getSystemColors(systems, empires, empireColors);

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        hoveredSystem = nullptr;

        // Проверка на наведенный объект
        for (System &system : systems) {
            MapPoint location = system.getLocation();
            int starX = location.x * CELL_SIZE;
            int starY = location.y * CELL_SIZE;

            SDL_Rect rect = {starX - CELL_SIZE / 2, starY - CELL_SIZE / 2,
                             CELL_SIZE, CELL_SIZE};
            if (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                mouseY >= rect.y && mouseY < rect.y + rect.h) {
                hoveredSystem = &system;
                break;
            }
        }

        // Обработка событий
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN &&
                       e.button.button == SDL_BUTTON_LEFT) {
                handlePowerTransfer(hoveredSystem, selectedSystem, galaxy,
                                    empires, empireColors, systemColors,
                                    powerTransfers);
            }
        }

        // Обновление силы
        if (SDL_GetTicks() - lastPowerUpdate >= powerUpdateInterval) {
            handlePowerUpdate(systems, systemColors, powerTransfers, galaxy,
                              empires, empireColors, lastPowerUpdate);
        }

        // Обновление захвата случайных соседей
        if (SDL_GetTicks() - lastConquerUpdate >= conquerUpdateInterval) {
            conquerRandomNeighbor(empires, galaxy, empireColors, systemColors,
                                  powerTransfers);
            lastConquerUpdate = SDL_GetTicks();
        }

        // Отображение
        SDL_SetRenderDrawColor(renderer, 30, 30, 46, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 205, 214, 244, 255);
        for (const Line &line : connections) {
            SDL_RenderDrawLine(renderer, line.start.x * CELL_SIZE,
                               line.start.y * CELL_SIZE, line.end.x * CELL_SIZE,
                               line.end.y * CELL_SIZE);
        }

        drawPowerTransferArrows(renderer, powerTransfers);

        for (const System &system : systems) {
            drawSystem(system, CELL_SIZE, hoveredSystem, HOVER_SIZE, renderer,
                       systemColors, font);
        }

        if (hoveredSystem) {
            drawSystemHover(hoveredSystem, mouseX, mouseY, renderer, font);
        }

        SDL_RenderPresent(renderer);

        // Ограничение кадровой частоты
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void conquer(System *system, Empire *empire, vector<Empire *> &empires,
             unordered_map<const Empire *, SDL_Color> &empireColors,
             unordered_map<int, SDL_Color> &systemColors) {
    systemColors[system->getId()] = empireColors[empire];
    empire->addSystem(system);
}

bool isSameColor(SDL_Color color1, SDL_Color color2) {
    return color1.r == color2.r && color1.g == color2.g &&
           color1.b == color2.b && color1.a == color2.a;
}

bool isOwned(SDL_Color color) {
    SDL_Color defaultStar = {249, 226, 175, 255};
    SDL_Color defaultHabitable = {166, 227, 161, 255};

    return !(color.r == defaultStar.r && color.g == defaultStar.g &&
             color.b == defaultStar.b && color.a == defaultStar.a) &&
           !(color.r == defaultHabitable.r && color.g == defaultHabitable.g &&
             color.b == defaultHabitable.b && color.a == defaultHabitable.a);
}

void updatePower(unordered_map<int, SDL_Color> &systemColors, System &system) {
    if (!isOwned(systemColors[system.getId()])) {
        return;
    }
    system.setPower(system.getPower() + 1);
    if (system.hasHabitables()) {
        int revenue = 0;
        for (const Resources &resource : system.getResources()) {
            revenue += resource.getPrice();
        }
        system.setPower(system.getPower() + revenue * difficulty);
    }
}

// Функция, которая обновляет и захватывает случайную соседнюю звезду для каждой
// империи
void conquerRandomNeighbor(
    vector<Empire *> &empires, const Galaxy &galaxy,
    unordered_map<const Empire *, SDL_Color> &empireColors,
    unordered_map<int, SDL_Color> &systemColors,
    vector<pair<System *, System *>> &powerTransfers) {
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(0, 100);  // диапазон случайных чисел

    for (Empire *empire : empires) {
        cout << "Empire: " << empire->getName() << endl;
        vector<System *> systems = empire->getSystems();
        System *system = systems[dist(rng) % systems.size()];
        vector<System *> neighbors = getNeighbors(system, galaxy);
        MapPoint syslocation = system->getLocation();
        if (neighbors.empty()) {
            cout << "No neighbors found for system at " << syslocation.x << " "
                 << syslocation.y << endl;
            return;
        }

        // Выбираем случайного соседа
        System *randomNeighbor = neighbors[dist(rng) % neighbors.size()];
        MapPoint location = randomNeighbor->getLocation();
        cout << location.x << " " << location.y << endl;

        if (system && randomNeighbor) {
            handlePowerTransfer(system, randomNeighbor, galaxy, empires,
                                empireColors, systemColors, powerTransfers);
        }
    }
}

void renderSystemInfo(TTF_Font *font, System *selectedSystem,
                      SDL_Renderer *renderer, SDL_Rect &infoBox,
                      const vector<Planet> &planets) {
    SDL_Color textColor = {205, 214, 244, 255};
    SDL_Color darkTextColor = {30, 30, 46, 255};
    SDL_Color bgGreen = {166, 227, 161, 255};
    SDL_Color bgYellow = {249, 226, 175, 255};

    SDL_Surface *surface;
    SDL_Texture *texture;

    // Отображаем имя системы
    surface = TTF_RenderText_Solid(font, selectedSystem->getName().c_str(),
                                   textColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect = {infoBox.x + 10, infoBox.y + 10, surface->w,
                         surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Отображаем планеты
    int yOffset = 30;
    for (const Planet &planet : planets) {
        if (planet.isHabitable()) {
            surface = TTF_RenderText_Shaded(
                font, ("-" + planet.getName()).c_str(), darkTextColor, bgGreen);
        } else if (planet.getName()[0] == '*') {
            surface = TTF_RenderText_Shaded(font, planet.getName().c_str(),
                                            darkTextColor, bgYellow);
        } else {
            surface = TTF_RenderText_Solid(
                font, ("-" + planet.getName()).c_str(), textColor);
        }
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        textRect = {infoBox.x + 20, infoBox.y + yOffset, surface->w,
                    surface->h};
        SDL_RenderCopy(renderer, texture, nullptr, &textRect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        yOffset += 20;
    }
}

unordered_map<int, SDL_Color> getSystemColors(
    const vector<System> &systems, vector<Empire *> &empires,
    unordered_map<const Empire *, SDL_Color> empireColors) {
    unordered_map<int, SDL_Color> systemColors;
    for (const System &system : systems) {
        SDL_Color systemColor = {249, 226, 175, 255};
        if (system.hasHabitables()) {
            systemColor = {166, 227, 161, 255};
        }
        for (const Empire *empire : empires) {
            for (const System *empireSystem : empire->getSystems()) {
                if (empireSystem->getId() == system.getId()) {
                    systemColor = empireColors[empire];
                }
            }
        }
        systemColors[system.getId()] = systemColor;
    }
    return systemColors;
}

void drawSystem(const System &system, const int CELL_SIZE,
                System *hoveredSystem, const int HOVER_SIZE,
                SDL_Renderer *renderer,
                unordered_map<int, SDL_Color> systemColors, TTF_Font *font) {
    MapPoint location = system.getLocation();
    int starX = location.x * CELL_SIZE;
    int starY = location.y * CELL_SIZE;
    SDL_Rect rect = {starX - CELL_SIZE / 2, starY - CELL_SIZE / 2, CELL_SIZE,
                     CELL_SIZE};

    SDL_Color systemColor = systemColors[system.getId()];
    bool isHovered = (&system == hoveredSystem);
    if (isHovered || isOwned(systemColor)) {
        rect.x = starX - HOVER_SIZE / 2;
        rect.y = starY - HOVER_SIZE / 2;
        rect.w = HOVER_SIZE;
        rect.h = HOVER_SIZE;
    }
    if (isHovered) {
        systemColor = {243, 139, 168, 255};
    }

    SDL_SetRenderDrawColor(renderer, systemColor.r, systemColor.g,
                           systemColor.b, systemColor.a);
    SDL_RenderFillRect(renderer, &rect);

    // Отображаем силу под звездой
    SDL_Color textColor = {205, 214, 244, 255};
    char power[32];
    sprintf(power, "%d", system.getPower());
    SDL_Surface *surface = TTF_RenderText_Solid(font, power, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect = {starX - surface->w / 2, starY + HOVER_SIZE / 2,
                         surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void drawSystemHover(System *hoveredSystem, int mouseX, int mouseY,
                     SDL_Renderer *renderer, TTF_Font *font) {
    const vector<Planet> &planets = hoveredSystem->getPlanets();
    const int INFO_BOX_WIDTH = 150;
    const int INFO_BOX_HEIGHT = (planets.size() + 1) * FONT_SIZE + 10;
    int infoBoxX = mouseX + 10;
    int infoBoxY = mouseY + 10;

    if (infoBoxX + INFO_BOX_WIDTH > SCREEN_WIDTH) {
        infoBoxX = mouseX - INFO_BOX_WIDTH - 10;
    }
    if (infoBoxY + INFO_BOX_HEIGHT > SCREEN_HEIGHT) {
        infoBoxY = mouseY - INFO_BOX_HEIGHT - 10;
    }

    SDL_Rect infoBox = {infoBoxX, infoBoxY, INFO_BOX_WIDTH, INFO_BOX_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 49, 50, 68, 255);
    SDL_RenderFillRect(renderer, &infoBox);

    SDL_SetRenderDrawColor(renderer, 205, 214, 244, 255);
    SDL_RenderDrawRect(renderer, &infoBox);

    renderSystemInfo(font, hoveredSystem, renderer, infoBox, planets);
}

void handlePowerUpdate(vector<System> &systems,
                       unordered_map<int, SDL_Color> &systemColors,
                       vector<pair<System *, System *>> &powerTransfers,
                       const Galaxy &galaxy, vector<Empire *> &empires,
                       unordered_map<const Empire *, SDL_Color> &empireColors,
                       Uint32 &lastPowerUpdate) {
    for (System &system : systems) {
        updatePower(systemColors, system);
    }

    for (pair<System *, System *> &powerTransfer : powerTransfers) {
        System *from = powerTransfer.first;
        System *to = powerTransfer.second;

        if (from && to) {
            SDL_Color empireColor = systemColors[from->getId()];
            transferPower(from, to, galaxy, empires, empireColors, systemColors,
                          empireColor);
        }
    }

    lastPowerUpdate = SDL_GetTicks();
}

void handlePowerTransfer(System *&hoveredSystem, System *&selectedSystem,
                         const Galaxy &galaxy, vector<Empire *> &empires,
                         unordered_map<const Empire *, SDL_Color> &empireColors,
                         unordered_map<int, SDL_Color> &systemColors,
                         vector<pair<System *, System *>> &powerTransfers) {
    if (!hoveredSystem) {
        return;
    }

    if (!selectedSystem) {
        // Выбираем систему если нет выбраной
        selectedSystem = hoveredSystem;
    } else if (selectedSystem == hoveredSystem) {
        // Отменяем выбор
        selectedSystem = nullptr;
    } else {
        SDL_Color empireColor = systemColors[selectedSystem->getId()];
        if (!isOwned(empireColor)) {
            selectedSystem = nullptr;
            return;
        }

        // Проверяем что системы соседи
        vector<System *> neighbors = getNeighbors(selectedSystem, galaxy);
        bool isNeighbor = false;
        for (System *neighbor : neighbors) {
            if (neighbor->getId() == hoveredSystem->getId()) {
                isNeighbor = true;
                break;
            }
        }

        if (!isNeighbor) {
            selectedSystem = nullptr;
            return;  // Можно перевести только соседу
        }

        // Переводим силу
        transferPower(selectedSystem, hoveredSystem, galaxy, empires,
                      empireColors, systemColors, empireColor);

        for (pair<System *, System *> transfer : powerTransfers) {
            if (transfer.first == selectedSystem) {
                powerTransfers.erase(remove(powerTransfers.begin(),
                                            powerTransfers.end(), transfer),
                                     powerTransfers.end());
            }
        }

        // Регистрируем перевод силы
        powerTransfers.push_back({selectedSystem, hoveredSystem});

        // Отменяем выбор
        selectedSystem = nullptr;
    }
}

void drawPowerTransferArrows(SDL_Renderer *renderer,
                             vector<pair<System *, System *>> &powerTransfers) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);  // Зеленые стрелочки

    for (auto &transfer : powerTransfers) {
        System *source = transfer.first;
        System *target = transfer.second;

        MapPoint sourceLocation = source->getLocation();
        MapPoint targetLocation = target->getLocation();

        int startX = sourceLocation.x * CELL_SIZE;
        int startY = sourceLocation.y * CELL_SIZE;
        int endX = targetLocation.x * CELL_SIZE;
        int endY = targetLocation.y * CELL_SIZE;

        // Рмсуем линию
        SDL_RenderDrawLine(renderer, startX, startY, endX, endY);

        // Рисуем наконечник стрелки
        int arrowSize = 10;
        int arrowAngle = 45;

        double angle = atan2(endY - startY, endX - startX);
        int arrowX = endX - arrowSize * cos(angle + arrowAngle);
        int arrowY = endY - arrowSize * sin(angle + arrowAngle);

        SDL_RenderDrawLine(renderer, endX, endY, arrowX, arrowY);

        arrowX = endX - arrowSize * cos(angle - arrowAngle);
        arrowY = endY - arrowSize * sin(angle - arrowAngle);

        SDL_RenderDrawLine(renderer, endX, endY, arrowX, arrowY);
    }
}

void generateEmpire(vector<Empire *> &empires, Galaxy &galaxy) {
    int i = empires.size();
    Empire *empire = new Empire(i + 1);
    empire->fill(galaxy);
    for (const Empire *otherEmpire : empires) {
        if (otherEmpire->getSystems()[0] == empire->getSystems()[0]) {
            delete empire;
            empire = nullptr;
            break;
        }
    }

    if (!empire || !empire->getSystems()[0]) {
        delete empire;
        empire = nullptr;
        return;
    }
    
    empires.push_back(empire);
}

void transferPower(System *&from, System *&to, const Galaxy &galaxy,
                   vector<Empire *> &empires,
                   unordered_map<const Empire *, SDL_Color> &empireColors,
                   unordered_map<int, SDL_Color> &systemColors,
                   SDL_Color &empireColor) {
    int transferAmount = from->getPower();
    int targetAmount = to->getPower();

    if (transferAmount > targetAmount) {
        from->setPower(0);
        to->setPower(transferAmount + targetAmount);
    }

    // Проверяем, принадлежит ли система империи
    Empire *empire = nullptr;
    for (Empire *other : empires) {
        if (isSameColor(empireColors[other], empireColor)) {
            empire = other;
            break;
        }
    }

    // Если да, и целевая система ей не принадлежит, а силы хватает,
    // завоевываем ее
    if (empire &&
        !(isSameColor(systemColors[from->getId()],
                      systemColors[to->getId()])) &&
        transferAmount > targetAmount) {
        to->setPower(transferAmount - targetAmount);
        conquer(to, empire, empires, empireColors, systemColors);
    }
}
