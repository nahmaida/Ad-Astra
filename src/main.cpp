#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <iostream>
#include <string>
#include <vector>

#include "../include/map.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int FONT_SIZE = 24;

void renderSystemInfo(TTF_Font *font, System *selectedSystem,
                      SDL_Renderer *renderer, SDL_Rect &infoBox,
                      const std::vector<Planet> &planets) {
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
            surface = TTF_RenderText_Shaded(
                font, ("-" + planet.getName()).c_str(), darkTextColor, bgGreen);
        } else if (planet.getName()[0] == '*') {
            surface = TTF_RenderText_Shaded(font, (planet.getName()).c_str(),
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

void displayGalaxy(const Galaxy &galaxy) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError()
                  << std::endl;
        return;
    }

    SDL_Window *window = SDL_CreateWindow("Galaxy Map", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                                          SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: "
                  << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! SDL_ttf Error: "
                  << TTF_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: "
                  << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    const int CELL_SIZE = 8;  // базовый размер звезды в пикселях
    const int HOVER_SIZE = 12;  // размер когда мышь парит
    const auto &systems = galaxy.getSystems();
    const auto &connections = galaxy.getConnections();

    bool quit = false;
    SDL_Event e;

    System *selectedSystem = nullptr;  // тут выбранная система

    while (!quit) {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEBUTTONDOWN &&
                       e.button.button == SDL_BUTTON_LEFT) {
                // Проверяем выбрана ли система
                for (const System &system : systems) {
                    MapPoint location = system.getLocation();

                    // коорды на экране
                    int starX = location.x * CELL_SIZE;
                    int starY = location.y * CELL_SIZE;

                    // коробка звезды
                    SDL_Rect rect = {starX - CELL_SIZE / 2,
                                     starY - CELL_SIZE / 2, CELL_SIZE,
                                     CELL_SIZE};

                    if (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                        mouseY >= rect.y && mouseY < rect.y + rect.h) {
                        selectedSystem = const_cast<System *>(
                            &system);  // Храним выбранную систему
                        break;
                    }
                }
            }
        }

        // Чистый экран
        SDL_SetRenderDrawColor(renderer, 30, 30, 46, 255);  // Фон
        SDL_RenderClear(renderer);

        // Рисуем соединения
        SDL_SetRenderDrawColor(renderer, 205, 214, 244, 255);  // Белые линии
        for (const Line &line : connections) {
            SDL_RenderDrawLine(renderer, line.start.x * CELL_SIZE,
                               line.start.y * CELL_SIZE, line.end.x * CELL_SIZE,
                               line.end.y * CELL_SIZE);
        }

        // Рисуем системы
        for (const System &system : systems) {
            MapPoint location = system.getLocation();

            // Коорды на экране
            int starX = location.x * CELL_SIZE;
            int starY = location.y * CELL_SIZE;

            // коробка звезды
            SDL_Rect rect = {starX - CELL_SIZE / 2, starY - CELL_SIZE / 2,
                             CELL_SIZE, CELL_SIZE};

            // провнряем парит ли мышь над звездой
            bool isHovered = (mouseX >= rect.x && mouseX < rect.x + rect.w &&
                              mouseY >= rect.y && mouseY < rect.y + rect.h);
            // выделяем если да
            if (isHovered) {
                rect.x = starX - HOVER_SIZE / 2;
                rect.y = starY - HOVER_SIZE / 2;
                rect.w = HOVER_SIZE;
                rect.h = HOVER_SIZE;
                SDL_SetRenderDrawColor(renderer, 243, 139, 168,
                                       255);  // Красный - парит
            } else if (&system == selectedSystem) {
                SDL_SetRenderDrawColor(renderer, 137, 180, 250,
                                       255);  // Синий - выбранная
            } else if (system.hasHabitables()) {
                SDL_SetRenderDrawColor(renderer, 166, 227, 161,
                                       255);  // Зеленый - обитаемая
            } else {
                SDL_SetRenderDrawColor(renderer, 249, 226, 175,
                                       255);  // Желтый - обычнвя
            }

            SDL_RenderFillRect(renderer, &rect);
        }

        // Выводим инфу о системе
        if (selectedSystem != nullptr) {
            const vector<Planet> &planets = selectedSystem->getPlanets();

            const int INFO_BOX_WIDTH = 200;
            const int INFO_BOX_HEIGHT = (planets.size() + 2) * FONT_SIZE;

            // Фон коробки
            SDL_Rect infoBox = {SCREEN_WIDTH - INFO_BOX_WIDTH - 10, 10,
                                INFO_BOX_WIDTH, INFO_BOX_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 49, 50, 68, 255);  // фон
            SDL_RenderFillRect(renderer, &infoBox);

            SDL_SetRenderDrawColor(renderer, 205, 214, 244, 255);  // граница
            SDL_RenderDrawRect(renderer, &infoBox);

            // Выводим планетки
            TTF_Font *font = TTF_OpenFont(
                "/home/nahmaida/Ad-Astra/res/bytebounce_medium.ttf", 24);
            if (font == nullptr) {
                std::cerr << "Failed to load font: " << TTF_GetError()
                          << std::endl;
            } else {
                renderSystemInfo(font, selectedSystem, renderer, infoBox,
                                 planets);
                TTF_CloseFont(font);
            }
        }

        // обновляем экран
        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }

    // все чистим
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *args[]) {
    // создаем галактику
    Galaxy galaxy(100);
    galaxy.fill();

    // выводим карту
    displayGalaxy(galaxy);

    return 0;
}
