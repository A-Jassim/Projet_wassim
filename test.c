/*
pour compiler sur windows:
gcc src/main.c -o bin/prog -I include -L lib -lmingw32 -lSDL2main -lSDL2

linux:
gcc main.c -o prog $(sdl2-config --cflags --libs) -ljson-c -lSDL2_ttf
*/

#include <stdio.h>
#include <stdlib.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <json-c/json.h>
#include "myfunc.h"
#include "myfuncSDL.h"


#define W_HEIGHT 480
#define W_WIDTH 640

#include <SDL2/SDL.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erreur d'initialisation de la SDL : %s\n", SDL_GetError());
        return 1;
    }

    // Création de la fenêtre principale (père)
    SDL_Window *window_pere = SDL_CreateWindow(
        "Fenêtre Père",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (!window_pere) {
        printf("Erreur lors de la création de la fenêtre père : %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Création de la fenêtre fils (sans bordures)
    SDL_Window *window_fils = SDL_CreateWindow(
        "Fenêtre Fils",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        400, 300,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
    );

    if (!window_fils) {
        printf("Erreur lors de la création de la fenêtre fils : %s\n", SDL_GetError());
        SDL_DestroyWindow(window_pere);
        SDL_Quit();
        return 1;
    }

    // Positionner la fenêtre fils relativement à la fenêtre père
    int pere_x, pere_y, pere_w, pere_h;
    SDL_GetWindowPosition(window_pere, &pere_x, &pere_y);
    SDL_GetWindowSize(window_pere, &pere_w, &pere_h);

    SDL_SetWindowPosition(window_fils,
                          pere_x + pere_w / 2 - 200,  // Décalage X pour centrer
                          pere_y + pere_h / 2 - 150); // Décalage Y pour centrer

    // Boucle principale
    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: // L'utilisateur ferme une fenêtre
                    running = 0;
                    break;

                case SDL_WINDOWEVENT: // Gestion des événements liés aux fenêtres
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        // Fermer une fenêtre spécifique
                        if (event.window.windowID == SDL_GetWindowID(window_fils)) {
                            SDL_DestroyWindow(window_fils);
                            window_fils = NULL; // Marque la fenêtre comme fermée
                        } else if (event.window.windowID == SDL_GetWindowID(window_pere)) {
                            running = 0; // Quitter la boucle si la fenêtre père est fermée
                        }
                    }
                    break;
            }
        }
    }

    // Nettoyage
    if (window_fils) SDL_DestroyWindow(window_fils);
    SDL_DestroyWindow(window_pere);
    SDL_Quit();

    return 0;
}

