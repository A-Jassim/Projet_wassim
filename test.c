/*
pour compiler sur windows:
gcc src/main.c -o bin/prog -I include -L lib -lmingw32 -lSDL2main -lSDL2

linux:
gcc main.c -o prog $(sdl2-config --cflags --libs) -ljson-c -lSDL_ttf
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

int main(int argc, char* argv[]){
    SDL_Window *window = NULL;  
    SDL_Renderer *renderer = NULL;

    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;

    /*textures textes*/
    SDL_Texture *monthTexture = NULL;
    SDL_Texture *dayTexture = NULL;
    SDL_Texture *entriesTexture = NULL;

    /*lancement sdl*/
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        return 1;
    }

    // Initialisation de SDL_ttf
    if (TTF_Init() == -1) {
        return 1;
    }


    /*création de la fenêtre*/
    window = SDL_CreateWindow("Calendrier", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W_WIDTH, W_HEIGHT, 0);
    if(window == NULL){
        printf("Erreur création de la fenêtre : %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /*création du rendu*/
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(renderer == NULL){
        printf("Erreur création du rendu : %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /*textes*/
    // Charger une police
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-Th.ttf", 24);
    if (font == NULL) {
        // Gestion d'erreur
        return 1;
    }

    // Couleur du texte
    SDL_Color color = {255, 255, 255}; // Noir


    // Rectangle pour positionner le texte
    SDL_Rect monthRect;
    monthRect.x = 120;
    monthRect.y = 80;
    monthRect.w = 3*40;
    monthRect.h = 40;


    /*main*/
    int quit = 0;
    SDL_Event event;
    while(!quit){
        /*effacer ecran*/
        SDL_RenderClear(renderer);

        // Créer une surface contenant le texte
        surface = TTF_RenderText_Solid(font, "DEC", color);

        // Créer une texture à partir de la surface
        monthTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        // Copier la texture sur le renderer
        SDL_RenderCopy(renderer, monthTexture, NULL, &monthRect);

        /*afficher le rendu*/
        SDL_RenderPresent(renderer);
        /*evenement*/
        while(SDL_PollEvent(&event)){
            switch (event.type){
                case SDL_QUIT:
                    quit = 1;
                    break;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}