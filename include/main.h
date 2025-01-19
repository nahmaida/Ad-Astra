#ifndef MAIN_H
#define MAIN_H

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

void conquer(System &system, Empire *empire, vector<Empire *> &empires,
             unordered_map<const Empire *, SDL_Color> empireColors,
             unordered_map<int, SDL_Color> &systemColors, Galaxy galaxy);

bool isOwned(SDL_Color color);

void updatePower(unordered_map<int, SDL_Color> &systemColors, System &system);

void renderSystemInfo(TTF_Font *font, System *selectedSystem,
                      SDL_Renderer *renderer, SDL_Rect &infoBox,
                      const vector<Planet> &planets);

unordered_map<int, SDL_Color> getSystemColors(
    const vector<System> &systems, vector<Empire *> &empires,
    unordered_map<const Empire *, SDL_Color> empireColors);

void drawSystem(const System &system, const int CELL_SIZE,
                System *hoveredSystem, const int HOVER_SIZE,
                SDL_Renderer *renderer,
                unordered_map<int, SDL_Color> systemColors, TTF_Font *font);

void drawSystemHover(System *hoveredSystem, int mouseX, int mouseY,
                     SDL_Renderer *renderer, TTF_Font *font);

void displayGalaxy(const Galaxy &galaxy, vector<Empire *> &empires);

void handlePowerTransfer(System *&hoveredSystem, System *&selectedSystem,
                         const Galaxy &galaxy, vector<Empire *> &empires,
             unordered_map<const Empire *, SDL_Color> &empireColors,
             unordered_map<int, SDL_Color> &systemColors);

void generateEmpire(vector<Empire *> &empires, Galaxy &galaxy);

void conquerRandomNeighbor(
    vector<Empire *> &empires, const Galaxy &galaxy,
    unordered_map<const Empire *, SDL_Color> &empireColors,
    unordered_map<int, SDL_Color> &systemColors);

#endif  // MAIN_H