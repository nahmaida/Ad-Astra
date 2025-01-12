#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "../include/empire.h"
#include "../include/map.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int FONT_SIZE = 24;

void renderSystemInfo(TTF_Font *font, System *selectedSystem,
                      SDL_Renderer *renderer, SDL_Rect &infoBox,
                      const vector<Planet> &planets) {
    // Цвета и т.п.
    SDL_Color textColor = {205, 214, 244, 255};
    SDL_Color darkTextColor = {30, 30, 46, 255};
    SDL_Color bgGreen = {166, 227, 161, 255};
    SDL_Color bgYellow = {249, 226, 175, 255};
    SDL_Surface *surface;
    SDL_Texture *texture;

    // Выводим имя системы
    surface = TTF_RenderText_Solid(font, selectedSystem->getName().c_str(),
                                   textColor);
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect = {infoBox.x + 10, infoBox.y + 10, surface->w,
                         surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &textRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Выводим планеты
    int yOffset = 30;
    for (const Planet &planet : planets) {
        if (planet.isHabitable()) {
            // обитаемые планеты
            surface = TTF_RenderText_Shaded(
                font, ("-" + planet.getName()).c_str(), darkTextColor, bgGreen);
        } else if (planet.getName()[0] == '*') {
            // звезды
            surface = TTF_RenderText_Shaded(font, (planet.getName()).c_str(),
                                            darkTextColor, bgYellow);
        } else {
            // обычные
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

void displayEmpires(SDL_Renderer *renderer, const vector<Empire*> &empires,
                    int cellSize,
                    unordered_map<const Empire *, SDL_Color> empireColors) {
    // Render systems owned by each empire with their respective color
    for (const Empire *empire : empires) {
        SDL_Color color = empireColors[empire];
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

        MapPoint location = empire->getSystems()[0]->getLocation();
        int starX = location.x * cellSize;
        int starY = location.y * cellSize;

        SDL_Rect rect = {starX - cellSize / 2, starY - cellSize / 2, cellSize,
                         cellSize};

        SDL_RenderFillRect(renderer, &rect);
    }
}

void displayGalaxy(const Galaxy &galaxy, vector<Empire*> &empires) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
             << endl;
        return;
    }

    SDL_Window *window = SDL_CreateWindow("Galaxy Map", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                                          SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (window == nullptr) {
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
    if (renderer == nullptr) {
        cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError()
             << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    const int CELL_SIZE = 8;
    const int HOVER_SIZE = 12;
    const auto &systems = galaxy.getSystems();
    const auto &connections = galaxy.getConnections();

    bool quit = false;
    SDL_Event e;

    System *hoveredSystem = nullptr;

    // Create a map to store the color for each empire
    unordered_map<const Empire *, SDL_Color> empireColors;

    // Generate random colors for each empire
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> dist(50,
                                       255);  // Avoid too dark or light colors

    for (const Empire *empire : empires) {
        empireColors[empire] = SDL_Color{static_cast<Uint8>(dist(rng)),
                                          static_cast<Uint8>(dist(rng)),
                                          static_cast<Uint8>(dist(rng)), 255};
    }

    while (!quit) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        hoveredSystem = nullptr;  // Reset hovered system each frame

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Detect hovered system
        for (const System &system : systems) {
            MapPoint location = system.getLocation();
            int starX = location.x * CELL_SIZE;
            int starY = location.y * CELL_SIZE;

            SDL_Rect rect = {starX - CELL_SIZE / 2, starY - CELL_SIZE / 2,
                             CELL_SIZE, CELL_SIZE};

            if (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                mouseY >= rect.y && mouseY < rect.y + rect.h) {
                hoveredSystem = const_cast<System *>(&system);
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 46, 255);
        SDL_RenderClear(renderer);

        // Draw connections
        SDL_SetRenderDrawColor(renderer, 205, 214, 244, 255);
        for (const Line &line : connections) {
            SDL_RenderDrawLine(renderer, line.start.x * CELL_SIZE,
                               line.start.y * CELL_SIZE, line.end.x * CELL_SIZE,
                               line.end.y * CELL_SIZE);
        }

        // Draw systems
        for (const System &system : systems) {
            MapPoint location = system.getLocation();
            int starX = location.x * CELL_SIZE;
            int starY = location.y * CELL_SIZE;
            SDL_Rect rect = {starX - CELL_SIZE / 2, starY - CELL_SIZE / 2,
                             CELL_SIZE, CELL_SIZE};

            bool isHovered = (&system == hoveredSystem);
            if (isHovered) {
                rect.x = starX - HOVER_SIZE / 2;
                rect.y = starY - HOVER_SIZE / 2;
                rect.w = HOVER_SIZE;
                rect.h = HOVER_SIZE;
                SDL_SetRenderDrawColor(renderer, 243, 139, 168, 255);
            } else if (system.hasHabitables()) {
                SDL_SetRenderDrawColor(renderer, 166, 227, 161, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 249, 226, 175, 255);
            }

            SDL_RenderFillRect(renderer, &rect);
        }

        // выводим империи
        displayEmpires(renderer, empires, HOVER_SIZE, empireColors);

        // Display system info if hovered
        if (hoveredSystem != nullptr) {
            const vector<Planet> &planets = hoveredSystem->getPlanets();
            const int INFO_BOX_WIDTH = 200;
            const int INFO_BOX_HEIGHT = (planets.size() + 1) * FONT_SIZE;
            // Position info box next to mouse pointer
            int infoBoxX = mouseX + 10;
            int infoBoxY = mouseY + 10;

            // Ensure it doesn't go off-screen
            if (infoBoxX + INFO_BOX_WIDTH > SCREEN_WIDTH) {
                infoBoxX = mouseX - INFO_BOX_WIDTH - 10;
            }
            if (infoBoxY + INFO_BOX_HEIGHT > SCREEN_HEIGHT) {
                infoBoxY = mouseY - INFO_BOX_HEIGHT - 10;
            }

            SDL_Rect infoBox = {infoBoxX, infoBoxY, INFO_BOX_WIDTH,
                                INFO_BOX_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 49, 50, 68, 255);
            SDL_RenderFillRect(renderer, &infoBox);

            SDL_SetRenderDrawColor(renderer, 205, 214, 244, 255);
            SDL_RenderDrawRect(renderer, &infoBox);

            TTF_Font *font = TTF_OpenFont(
                "/home/nahmaida/Ad-Astra/res/bytebounce_medium.ttf", 24);
            if (font == nullptr) {
                cerr << "Failed to load font: " << TTF_GetError() << endl;
            } else {
                renderSystemInfo(font, hoveredSystem, renderer, infoBox,
                                 planets);
                TTF_CloseFont(font);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(32);
    }

    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *args[]) {
    // инициализируем названия звезд
    loadStarnames("/home/nahmaida/Ad-Astra/res/starnames.txt");
    // создаем галактику
    Galaxy galaxy(100);
    galaxy.fill();

    // создаем империи
    vector<Empire*> empires;
    while (empires.size() < 9) {
        Empire* empire = new Empire(empires.size() + 1);
        empire->fill(galaxy);
        bool valid = true;
        for (Empire* other : empires) {
            if (other->getId() != empire->getId() &&
                other->getSystems()[0] == empire->getSystems()[0]) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            delete empire;  // удаляем неправильную империю
            continue;
        }
        MapPoint location = empire->getSystems()[0]->getLocation();
        cout << "координаты империи: " << location.x << " " << location.y << endl;
        empires.push_back(empire);
    }

    for (const Empire* empire : empires) {
        MapPoint location = empire->getSystems()[0]->getLocation();
        cout << "империя " << empire->getId() << " в " << location.x << ", "
             << location.y << endl;
    }

    // выводим карту
    displayGalaxy(galaxy, empires);

    // чистим память
    for (Empire* empire : empires) {
        delete empire;
    }

    return 0;
}