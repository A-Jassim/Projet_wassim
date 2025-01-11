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
    /*mois année affiché par le tableau*/
    int currentMonth = 12;
    char* currentMonthString = "DEC";
    int currentYear = 2024;
    int currentMonthYear[2] = {currentMonth, currentYear};



    SDL_Window *window = NULL;  
    SDL_Renderer *renderer = NULL;

    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;

    /*textures textes*/
    SDL_Texture *monthTexture = NULL;
    SDL_Texture *dayTexture = NULL;
    SDL_Texture *entriesTexture = NULL;

    /*boutons*/
    dayButtonsTabStruct dayButtonsTab = refreshDayButtons(currentMonth, currentYear);
    /*boutons pour les fleches des mois : h, w, x, y*/
    int previousMonthButton[4] = {40, 40, 80, 80};
    int nextMonthButton[4] = {40, 40, 240, 80};

    /*json parsé*/
    struct json_object *parsedJson;
    parsedJson = getParsedJson();
    struct json_object *monthJson;
    struct json_object *dayJson;
    struct json_object *entriesJson;
    entriesTabStruct entriesTab;


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


    /*chargement image*/
    surface = SDL_LoadBMP("bg.bmp");

    if(surface == NULL){
        printf("Erreur chargement image : %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /*création de la texture*/
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);


    /*application au fond de la fenetre*/
    SDL_Rect bgRect;
    bgRect.h = W_HEIGHT;
    bgRect.w = W_WIDTH;
    bgRect.x = 0;
    bgRect.y = 0;


    /*textes*/
    // Charger une police
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/ubuntu/Ubuntu-Th.ttf", 40);
    if (font == NULL) {
        // Gestion d'erreur
        return 1;
    }

    // Couleur du texte
    SDL_Color color = {0, 0, 0}; // Noir


    // Rectangle pour positionner le texte
    int w, h;
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
        /*afficher fond*/
        SDL_RenderCopy(renderer, texture, NULL, &bgRect);



        // Créer une surface contenant le texte
        surface = TTF_RenderText_Solid(font, currentMonthString, color);

        // Créer une texture à partir de la surface
        monthTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        SDL_QueryTexture(monthTexture, NULL, NULL, &w, &h);
        monthRect.w = w;
        monthRect.h = h;

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
                
                case SDL_MOUSEBUTTONDOWN:
                    /*mois*/
                    /*mois précédent*/
                    if(
                        (event.motion.x <= (previousMonthButton[2] + previousMonthButton[1])) &&
                        (event.motion.x >= (previousMonthButton[2])) &&
                        (event.motion.y <= (previousMonthButton[3] + previousMonthButton[0])) &&
                        (event.motion.y >= (previousMonthButton[3])))
                        {
                            if(currentMonth == 1){
                                currentMonth = 12;
                                currentYear--;
                            }
                            else{
                                currentMonth--;
                            }
                            currentMonthYear[0] = currentMonth;
                            currentMonthYear[1] = currentYear;
                            printf("mois precedent %d %d\n", currentMonth, currentYear);
                            /*actualise le json, les buttons*/
                            monthJson = getMonthJson(parsedJson, currentMonthYear);
                            dayButtonsTab = refreshDayButtons(currentMonth, currentYear);
                            currentMonthString = getMonthStr(currentMonth);
                        }
                    /*mois prochain*/
                    else if(
                        (event.motion.x <= (nextMonthButton[2] + nextMonthButton[1])) &&
                        (event.motion.x >= (nextMonthButton[2])) &&
                        (event.motion.y <= (nextMonthButton[3] + nextMonthButton[0])) &&
                        (event.motion.y >= (nextMonthButton[3])))
                        {
                            if(currentMonth == 12){
                                currentMonth = 1;
                                currentYear++;
                            }
                            else{
                                currentMonth++;
                            }
                            currentMonthYear[0] = currentMonth;
                            currentMonthYear[1] = currentYear;
                            printf("mois prochain %d %d\n", currentMonth, currentYear);
                            /*actualise le json, les buttons*/
                            monthJson = getMonthJson(parsedJson, currentMonthYear);
                            dayButtonsTab = refreshDayButtons(currentMonth, currentYear);
                            currentMonthString = getMonthStr(currentMonth);
                        }
                    /*boutons jour*/
                    else{
                        for(int i = 0; i < dayButtonsTab.dayButtonNumber; i++){
                            if(
                                (event.motion.x <= (dayButtonsTab.dayButtons[i].x + dayButtonsTab.dayButtons[i].w)) && 
                                (event.motion.x >= (dayButtonsTab.dayButtons[i].x)) &&
                                (event.motion.y <= (dayButtonsTab.dayButtons[i].y + dayButtonsTab.dayButtons[i].h)) && 
                                (event.motion.y >= (dayButtonsTab.dayButtons[i].y)))
                                {
                                printf("bouton %d cliqué\n", dayButtonsTab.dayButtons[i].id);
                                dayJson = getDayJson(monthJson, dayButtonsTab.dayButtons[i].id);
                                entriesJson = getEntriesJson(dayJson);
                                entriesTab = getEntriesTab(entriesJson);
                                /*ici normalement on a un tableau structure avec tous les entrées d'un jour d'un mois*/
                                for(int j = 0; j < entriesTab.entriesNumber; j++){
                                printf("%s : %d\n", entriesTab.entries[j].entryName, entriesTab.entries[j].entryValue);
                                }
                            }
                        }
                    }
                    continue;
                
                case SDL_MOUSEMOTION:
                    printf("%d %d\n", event.motion.x, event.motion.y);
                    continue;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}