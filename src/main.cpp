#include "../include/main.h"

#include "main.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int FONT_SIZE = 22;
const int DELAY = 4000;  // Интервал обновления силы (в миллисекундах)
const int FPS = 30;
const int CELL_SIZE = 8;
const int HOVER_SIZE = 12;
float difficulty = 1;
Empire *selectedEmpire = nullptr;
SDL_Color playerColor = {0, 0, 0, 0};

int main(int argc, char *args[]) {
    mt19937 rng(random_device{}());

    loadStarnames("/home/nahmaida/Ad-Astra/res/starnames.txt");
    int size = 100;
    int nEmpires = 9;
    try {
        cout << "Введите количество империй (от 1, 9 - рекомендуемое)\n>> ";
        cin >> nEmpires;
        if (nEmpires < 1) {
            throw invalid_argument(
                "Неверное количество! Будет установлено количество 9.");
        }
    } catch (const exception &e) {
        cout << e.what() << endl;
        nEmpires = 9;
    }

    try {
        cout << "Введите размер галактики (от " << nEmpires
             << ", 60-120 - рекомендуемый)\n>> ";
        cin >> size;
        if (size < nEmpires) {
            throw invalid_argument(
                "Неверный размер! Будет установлен размер 100.");
        }
    } catch (const exception &e) {
        cout << e.what() << endl;
        size = 100;
    }

    Galaxy galaxy(size);
    galaxy.fill();

    while (1) {
        int nHabitables = 0;
        for (System *system : galaxy.getSystems()) {
            if (system->hasHabitables()) {
                nHabitables++;
            }
        }
        if (nEmpires > nHabitables) {
            cout << "Недостаточно планет для генерации, пробуем еще раз..."
                 << endl;
            galaxy.clear();
            galaxy = Galaxy(size);
            galaxy.fill();
        } else {
            break;
        }
    }

    vector<Empire *> empires;
    while (empires.size() < nEmpires) {
        generateEmpire(empires, galaxy);
    }

    try {
        cout << "Введите сложность (от 1 до 5)\n>> ";
        cin >> difficulty;
        if (difficulty < 1 || difficulty > 5) {
            throw invalid_argument(
                "Неверная сложность! Будет установлена сложность 3.");
        } else {
            difficulty = 1 / (difficulty / 2);
        }
    } catch (const exception &e) {
        cout << e.what() << endl;
        difficulty = 1 / 3;
    }

    displayGalaxy(galaxy, empires, rng);

    for (Empire *empire : empires) {
        delete empire;
    }

    return 0;
}

void displayGalaxy(const Galaxy &galaxy, vector<Empire *> &empires,
                   mt19937 &rng) {
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

    vector<System *> systems = galaxy.getSystems();
    const auto &connections = galaxy.getConnections();

    TTF_Font *font = TTF_OpenFont(
        "/home/nahmaida/Ad-Astra/res/bytebounce_medium.ttf", FONT_SIZE);
    TTF_Font *largeFont =
        TTF_OpenFont("/home/nahmaida/Ad-Astra/res/bytebounce_medium.ttf",
                     FONT_SIZE * 1.5);  // Шрифт побольше
    if (!font || !largeFont) {
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
    Empire *victorsNation = nullptr;

    bool quitWelcomeScreen = false;
    bool empireSelected = false;

    vector<pair<System *, System *>> powerTransfers = {};
    unordered_map<const Empire *, SDL_Color> empireColors;

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
    const Uint32 conquerUpdateInterval = DELAY;  // 2000 ms

    while (!quitWelcomeScreen) {
        renderWelcomeScreen(largeFont, renderer, quitWelcomeScreen);
    }

    while (!quit) {
        frameStart = SDL_GetTicks();

        unordered_map<int, SDL_Color> systemColors =
            getSystemColors(systems, empires, empireColors);

        // Проверяет поражение
        if (empireSelected && selectedEmpire->getSystems().empty()) {
            renderGameOverScreen(renderer, font, largeFont);
            quit = true;
        }

        // Проверяет победу
        if (checkVictory(systems, empires, victorsNation, empireColors,
                         systemColors)) {
            string victoryMessage = victorsNation->getName();
            renderVictoryScreen(renderer, font, largeFont, victoryMessage);
            quit = true;  // Выходит из игры
        }

        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        hoveredSystem = nullptr;

        // Проверка на наведенный объект
        for (System *system : systems) {
            MapPoint location = system->getLocation();
            int starX = location.x * CELL_SIZE;
            int starY = location.y * CELL_SIZE;

            SDL_Rect rect = {starX - CELL_SIZE / 2, starY - CELL_SIZE / 2,
                             CELL_SIZE, CELL_SIZE};
            if (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                mouseY >= rect.y && mouseY < rect.y + rect.h) {
                hoveredSystem = system;
                break;
            }
        }

        // Обработка событий
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN &&
                       e.button.button == SDL_BUTTON_LEFT) {
                if (!empireSelected) {
                    handleEmpireSelection(e, empires, selectedSystem,
                                          empireColors);  // Выбираем империю
                    empireSelected = true;
                }

                handlePowerTransfer(hoveredSystem, selectedSystem, galaxy,
                                    empires, empireColors, systemColors,
                                    powerTransfers, true);
            }
        }

        // Обновление силы
        if (empireSelected &&
            SDL_GetTicks() - lastPowerUpdate >= powerUpdateInterval) {
            handlePowerUpdate(systems, systemColors, powerTransfers, galaxy,
                              empires, empireColors, lastPowerUpdate);
        }

        // Обновление захвата случайных соседей
        if (empireSelected &&
            SDL_GetTicks() - lastConquerUpdate >= conquerUpdateInterval) {
            conquerRandomNeighbor(empires, galaxy, empireColors, systemColors,
                                  powerTransfers, rng);
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

        for (const System *system : systems) {
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
    // Ищет владельца системы если есть
    SDL_Color color = systemColors[system->getId()];
    Empire *oldEmpire = nullptr;
    if (isOwned(color)) {
        auto it = find_if(begin(empires), end(empires), [&](const Empire *e) {
            return isSameColor(empireColors[e], color);
        });
        oldEmpire = *it;
        oldEmpire->removeSystem(system);
    }
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

void updatePower(unordered_map<int, SDL_Color> &systemColors, System *system) {
    if (!isOwned(systemColors[system->getId()])) {
        return;
    }
    system->setPower(system->getPower() + 1);
    if (system->hasHabitables()) {
        int revenue = 0;
        for (const Resources &resource : system->getResources()) {
            revenue += resource.getPrice();
        }
        if (isSameColor(systemColors[system->getId()], playerColor)) {
            revenue = revenue * difficulty;
        }
        system->setPower(system->getPower() + revenue);
    }
}

// Функция, которая обновляет и захватывает случайную соседнюю звезду для каждой
// империи
void conquerRandomNeighbor(
    vector<Empire *> &empires, const Galaxy &galaxy,
    unordered_map<const Empire *, SDL_Color> &empireColors,
    unordered_map<int, SDL_Color> &systemColors,
    vector<pair<System *, System *>> &powerTransfers, mt19937 &rng) {
    uniform_int_distribution<int> dist(0, 100);  // диапазон случайных чисел

    for (Empire *empire : empires) {
        // Пропускаем игрока
        if (empire == selectedEmpire) {
            continue;
        }

        vector<System *> systems = empire->getSystems();

        if (systems.empty()) {
            continue;
        }

        System *system = systems[dist(rng) % systems.size()];
        vector<System *> neighbors = getNeighbors(system, galaxy);
        if (neighbors.empty()) {
            continue;
        }

        // Выбираем случайного соседа
        System *randomNeighbor = system;
        while (randomNeighbor->getId() == system->getId()) {
            randomNeighbor = neighbors[dist(rng) % neighbors.size()];
        }

        // Если есть незахваченая система, выбираем ее
        SDL_Color color = systemColors[system->getId()];
        for (System *neighbor : neighbors) {
            if (!isSameColor(color, systemColors[neighbor->getId()])) {
                randomNeighbor = neighbor;
                break;
            }
        }

        if (system && randomNeighbor) {
            handlePowerTransfer(randomNeighbor, system, galaxy, empires,
                                empireColors, systemColors, powerTransfers,
                                false);
        }
    }
}

// Проверяет, победила ли какая нибудь империя
bool checkVictory(const vector<System *> &systems, vector<Empire *> &empires,
                  Empire *&victoriousEmpire,
                  unordered_map<const Empire *, SDL_Color> &empireColors,
                  unordered_map<int, SDL_Color> &systemColors) {
    SDL_Color previousColor = {0, 0, 0, 0};
    SDL_Color color = {0, 0, 0, 0};

    for (System *system : systems) {
        color = systemColors[system->getId()];
        if (!isSameColor(color, previousColor) &&
            !isSameColor(previousColor, {0, 0, 0, 0})) {
            return false;
        }
        previousColor = color;
    }

    auto it = find_if(begin(empires), end(empires), [&](const Empire *e) {
        return isSameColor(empireColors[e], color);
    });
    victoriousEmpire = *it;
    return true;
}

void renderWelcomeScreen(TTF_Font *font, SDL_Renderer *renderer,
                         bool &quitWelcomeScreen) {
    SDL_Color textColor = {205, 214, 244, 255};
    SDL_Color buttonColor = {166, 227, 161, 255};
    SDL_Color darkTextColor = {30, 30, 46, 255};

    SDL_Surface *surface;
    SDL_Texture *texture;

    SDL_SetRenderDrawColor(renderer, 30, 30, 46, 255);
    SDL_RenderClear(renderer);

    // Текст приветствия
    surface = TTF_RenderText_Solid(font, "Welcome! Select an empire to start",
                                   textColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect messageRect = {SCREEN_WIDTH / 2 - surface->w / 2,
                            SCREEN_HEIGHT / 2 - 50, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &messageRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Кнока ОК
    SDL_Rect buttonRect = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 20, 100,
                           40};
    SDL_SetRenderDrawColor(renderer, buttonColor.r, buttonColor.g,
                           buttonColor.b, buttonColor.a);
    SDL_RenderFillRect(renderer, &buttonRect);
    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b,
                           textColor.a);
    SDL_RenderDrawRect(renderer, &buttonRect);

    // Текст кнопки
    surface = TTF_RenderText_Solid(font, "Okay", darkTextColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect buttonTextRect = {buttonRect.x + 10, buttonRect.y + 10, surface->w,
                               surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &buttonTextRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);

    // Ждем нажатия
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN &&
                       e.button.button == SDL_BUTTON_LEFT) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;
                if (mouseX >= buttonRect.x &&
                    mouseX <= buttonRect.x + buttonRect.w &&
                    mouseY >= buttonRect.y &&
                    mouseY <= buttonRect.y + buttonRect.h) {
                    quitWelcomeScreen = true;
                    return;
                }
            }
        }
    }
}

void handleEmpireSelection(
    SDL_Event &e, vector<Empire *> &empires, System *&selectedSystem,
    unordered_map<const Empire *, SDL_Color> &empireColors) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int mouseX = e.button.x;
        int mouseY = e.button.y;

        // Ищем империю владельца
        for (Empire *empire : empires) {
            System *empireSystem = empire->getSystems()[0];
            MapPoint location = empireSystem->getLocation();
            int systemX = location.x * CELL_SIZE;
            int systemY = location.y * CELL_SIZE;
            int systemRadius = CELL_SIZE / 2;

            // Проверяем, попали ли в радиус системы
            if (abs(mouseX - systemX) < systemRadius &&
                abs(mouseY - systemY) < systemRadius) {
                selectedEmpire = empire;
                playerColor = empireColors[empire];
                selectedSystem = empireSystem;
                return;
            }
        }
    }
}

// Окно победы (шрифт не работает на русском)
void renderVictoryScreen(SDL_Renderer *renderer, TTF_Font *font,
                         TTF_Font *largeFont, const string &winnerName) {
    const int VICTORY_BOX_WIDTH = 400;
    const int VICTORY_BOX_HEIGHT = 200;
    const SDL_Color bgColor = {69, 71, 90, 255};
    const SDL_Color textColor = {205, 214, 244, 255};
    const SDL_Color darkTextColor = {30, 30, 46, 255};
    const SDL_Color buttonColor = {166, 227, 161, 255};

    SDL_Rect victoryBox = {SCREEN_WIDTH / 2 - VICTORY_BOX_WIDTH / 2,
                           SCREEN_HEIGHT / 2 - VICTORY_BOX_HEIGHT / 2,
                           VICTORY_BOX_WIDTH, VICTORY_BOX_HEIGHT};

    // Фон
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b,
                           bgColor.a);
    SDL_RenderFillRect(renderer, &victoryBox);

    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b,
                           textColor.a);
    SDL_RenderDrawRect(renderer, &victoryBox);

    string line1 = "Victory! You have successfully";
    string line2 = "conquered the entire galaxy!";
    int lineSpacing = 10;  // Пространство между строками

    // Строка 1
    SDL_Surface *surface =
        TTF_RenderText_Solid(largeFont, line1.c_str(), textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect1 = {victoryBox.x + 20, victoryBox.y + 40, surface->w,
                          surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect1);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Строка 2
    surface = TTF_RenderText_Solid(largeFont, line2.c_str(), textColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect2 = {victoryBox.x + 20,
                          textRect1.y + textRect1.h + lineSpacing, surface->w,
                          surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect2);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    TTF_CloseFont(largeFont);  // Закрываем шрифт

    // Кнопка конца игры
    SDL_Rect buttonRect = {victoryBox.x + VICTORY_BOX_WIDTH / 2 - 50,
                           victoryBox.y + VICTORY_BOX_HEIGHT - 60, 100, 40};
    SDL_SetRenderDrawColor(renderer, buttonColor.r, buttonColor.g,
                           buttonColor.b, buttonColor.a);
    SDL_RenderFillRect(renderer, &buttonRect);

    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b,
                           textColor.a);
    SDL_RenderDrawRect(renderer, &buttonRect);

    // Текст кнопки
    surface = TTF_RenderText_Solid(font, "End Game", darkTextColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect buttonTextRect = {buttonRect.x + 10, buttonRect.y + 10, surface->w,
                               surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &buttonTextRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);

    // Ждем нажатия
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN &&
                       e.button.button == SDL_BUTTON_LEFT) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;
                if (mouseX >= buttonRect.x &&
                    mouseX <= buttonRect.x + buttonRect.w &&
                    mouseY >= buttonRect.y &&
                    mouseY <= buttonRect.y + buttonRect.h) {
                    quit = true;
                }
            }
        }
    }
}

// Окно поражения (шрифт не работает на русском)
void renderGameOverScreen(SDL_Renderer *renderer, TTF_Font *font,
                          TTF_Font *largeFont) {
    const int VICTORY_BOX_WIDTH = 400;
    const int VICTORY_BOX_HEIGHT = 200;
    const SDL_Color bgColor = {69, 71, 90, 255};
    const SDL_Color textColor = {205, 214, 244, 255};
    const SDL_Color darkTextColor = {30, 30, 46, 255};
    const SDL_Color buttonColor = {243, 139, 168, 255};

    SDL_Rect victoryBox = {SCREEN_WIDTH / 2 - VICTORY_BOX_WIDTH / 2,
                           SCREEN_HEIGHT / 2 - VICTORY_BOX_HEIGHT / 2,
                           VICTORY_BOX_WIDTH, VICTORY_BOX_HEIGHT};

    // Фон
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b,
                           bgColor.a);
    SDL_RenderFillRect(renderer, &victoryBox);

    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b,
                           textColor.a);
    SDL_RenderDrawRect(renderer, &victoryBox);

    string line1 = "You have been defeated!";
    string line2 = "Your empire is no more.";
    int lineSpacing = 10;  // Пространство между строками

    // Строка 1
    SDL_Surface *surface =
        TTF_RenderText_Solid(largeFont, line1.c_str(), textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect1 = {victoryBox.x + 20, victoryBox.y + 40, surface->w,
                          surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect1);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Строка 2
    surface = TTF_RenderText_Solid(largeFont, line2.c_str(), textColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect2 = {victoryBox.x + 20,
                          textRect1.y + textRect1.h + lineSpacing, surface->w,
                          surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect2);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    TTF_CloseFont(largeFont);  // Закрываем шрифт

    // Кнопка конца игры
    SDL_Rect buttonRect = {victoryBox.x + VICTORY_BOX_WIDTH / 2 - 50,
                           victoryBox.y + VICTORY_BOX_HEIGHT - 60, 100, 40};
    SDL_SetRenderDrawColor(renderer, buttonColor.r, buttonColor.g,
                           buttonColor.b, buttonColor.a);
    SDL_RenderFillRect(renderer, &buttonRect);

    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b,
                           textColor.a);
    SDL_RenderDrawRect(renderer, &buttonRect);

    // Текст кнопки
    surface = TTF_RenderText_Solid(font, "End Game", darkTextColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect buttonTextRect = {buttonRect.x + 10, buttonRect.y + 10, surface->w,
                               surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &buttonTextRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);

    // Ждем нажатия
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN &&
                       e.button.button == SDL_BUTTON_LEFT) {
                int mouseX = e.button.x;
                int mouseY = e.button.y;
                if (mouseX >= buttonRect.x &&
                    mouseX <= buttonRect.x + buttonRect.w &&
                    mouseY >= buttonRect.y &&
                    mouseY <= buttonRect.y + buttonRect.h) {
                    quit = true;
                }
            }
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
    const vector<System *> &systems, vector<Empire *> &empires,
    unordered_map<const Empire *, SDL_Color> empireColors) {
    unordered_map<int, SDL_Color> systemColors;
    for (const System *system : systems) {
        SDL_Color systemColor = {249, 226, 175, 255};
        if (system->hasHabitables()) {
            systemColor = {166, 227, 161, 255};
        }
        for (const Empire *empire : empires) {
            for (const System *empireSystem : empire->getSystems()) {
                if (empireSystem->getId() == system->getId()) {
                    systemColor = empireColors[empire];
                }
            }
        }
        systemColors[system->getId()] = systemColor;
    }
    return systemColors;
}

void drawSystem(const System *system, const int CELL_SIZE,
                System *hoveredSystem, const int HOVER_SIZE,
                SDL_Renderer *renderer,
                unordered_map<int, SDL_Color> systemColors, TTF_Font *font) {
    MapPoint location = system->getLocation();
    int starX = location.x * CELL_SIZE;
    int starY = location.y * CELL_SIZE;
    SDL_Rect rect = {starX - CELL_SIZE / 2, starY - CELL_SIZE / 2, CELL_SIZE,
                     CELL_SIZE};

    SDL_Color systemColor = systemColors[system->getId()];
    bool isHovered = (system == hoveredSystem);
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
    sprintf(power, "%d", system->getPower());
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

void handlePowerUpdate(vector<System *> &systems,
                       unordered_map<int, SDL_Color> &systemColors,
                       vector<pair<System *, System *>> &powerTransfers,
                       const Galaxy &galaxy, vector<Empire *> &empires,
                       unordered_map<const Empire *, SDL_Color> &empireColors,
                       Uint32 &lastPowerUpdate) {
    for (System *system : systems) {
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
                         vector<pair<System *, System *>> &powerTransfers,
                         bool isPlayerClick) {
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
        if (isPlayerClick &&
            !isSameColor(systemColors[selectedSystem->getId()], playerColor)) {
            selectedSystem = nullptr;
            return;
        }

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

    if (!empire || !empire->getSystems()[0]) {
        delete empire;
        empire = nullptr;
        return;
    }

    for (const Empire *otherEmpire : empires) {
        if (otherEmpire->getSystems()[0]->getId() ==
            empire->getSystems()[0]->getId()) {
            delete empire;
            empire = nullptr;
            return;
        }
    }

    MapPoint location = empire->getSystems()[0]->getLocation();
    if (location.x <= 0 || location.y < 0 || location.x >= 10000 ||
        location.y >= 10000) {
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
