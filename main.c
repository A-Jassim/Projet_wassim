/*
pour compiler sur windows:
gcc src/main.c -o bin/prog -I include -L lib -lmingw32 -lSDL2main -lSDL2

linux:
gcc main.c -o prog $(sdl2-config --cflags --libs) -ljson-c -lSDL2_ttf
gcc main.c -o prog -ljson-c -lSDL2 -lSDL2_ttf -I/usr/lib/

*/

#include <stdio.h>
#include <stdlib.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <json-c/json.h>
#include "myfunc.h"
#include "myfuncSDL.h"
#include <string.h>


#define W_HEIGHT 480
#define W_WIDTH 640
#define CHILD_W 320
#define CHILD_H 240


void refreshScreen();

void extractDate(const char *input, int *day, int *month, int *year) {
    char buffer[20]; 
    strcpy(buffer, input);

    // Utiliser strtok pour séparer les parties de la date
    char *t = strtok(buffer, "/");
    if (t != NULL) {
        *day = atoi(t); // Convertit la chaîne en entier
        t = strtok(NULL, "/"); // Récupérer la partie suivante (mois)
    }
    if (t != NULL) {
        *month = atoi(t);
        t = strtok(NULL, "/"); // Récupérer la partie suivante (année)
    }
    if (t != NULL) {
        *year = atoi(t);
    }
}

void write_to_json(const char *filename, int day, int month, int year, const char *entryName, int entryValue) {
    // Charger le contenu existant du fichier JSON
    struct json_object *root = NULL;
    FILE *file = fopen(filename, "r");

    if (file) {
        // Charger le fichier existant
        char buffer[8192];
        fread(buffer, sizeof(char), sizeof(buffer), file);
        fclose(file);
        root = json_tokener_parse(buffer);
    }

    if (!root || !json_object_is_type(root, json_type_array)) {
        // Si le fichier n'existe pas ou n'est pas un tableau, créer un tableau vide
        root = json_object_new_array();
    }

    // Créer une chaîne pour représenter la date au format "jour-mois-année"
    char dateStr[16];
    snprintf(dateStr, sizeof(dateStr), "%d-%d-%d", day, month, year);

    // Chercher si la date existe déjà
    int found = 0;
    for (int i = 0; i < json_object_array_length(root); i++) {
        struct json_object *entry = json_object_array_get_idx(root, i);
        struct json_object *dateObj;

        if (json_object_object_get_ex(entry, "date", &dateObj)) {
            // Vérifier si la date correspond
            char existingDateStr[16];
            snprintf(existingDateStr, sizeof(existingDateStr), "%d-%d-%d",
                     json_object_get_int(json_object_array_get_idx(dateObj, 0)),
                     json_object_get_int(json_object_array_get_idx(dateObj, 1)),
                     json_object_get_int(json_object_array_get_idx(dateObj, 2)));

            if (strcmp(existingDateStr, dateStr) == 0) {
                // Date trouvée, ajouter l'entrée à la liste des entrées existantes
                struct json_object *entriesObj;
                if (json_object_object_get_ex(entry, "entries", &entriesObj)) {
                    struct json_object *newEntry = json_object_new_array();
                    json_object_array_add(newEntry, json_object_new_string(entryName));
                    json_object_array_add(newEntry, json_object_new_int(entryValue));
                    json_object_array_add(entriesObj, newEntry);
                }
                found = 1;
                break;
            }
        }
    }

    // Si la date n'existe pas encore, ajouter une nouvelle entrée pour cette date
    if (!found) {
        struct json_object *newDateObj = json_object_new_object();

        // Ajouter la date
        struct json_object *dateArray = json_object_new_array();
        json_object_array_add(dateArray, json_object_new_int(day));
        json_object_array_add(dateArray, json_object_new_int(month));
        json_object_array_add(dateArray, json_object_new_int(year));
        json_object_object_add(newDateObj, "date", dateArray);

        // Ajouter l'entrée
        struct json_object *entriesArray = json_object_new_array();
        struct json_object *newEntry = json_object_new_array();
        json_object_array_add(newEntry, json_object_new_string(entryName));
        json_object_array_add(newEntry, json_object_new_int(entryValue));
        json_object_array_add(entriesArray, newEntry);
        json_object_object_add(newDateObj, "entries", entriesArray);

        // Ajouter l'objet à la racine
        json_object_array_add(root, newDateObj);
    }

    // Sauvegarder le fichier
    if (json_object_to_file_ext(filename, root, JSON_C_TO_STRING_PRETTY) == -1) {
        printf("Erreur lors de l'écriture dans le fichier JSON\n");
    } else {
        printf("Données JSON mises à jour avec succès dans le fichier %s\n", filename);
    }

    // Libérer la mémoire
    json_object_put(root);
}


void inputBox_withchild(SDL_Renderer *renderer, TTF_Font *font, SDL_Color textColor, SDL_Color boxColor, 
                        int mouseX, int mouseY, int mainWindowX, int mainWindowY, 
                        int windowWidth, int windowHeight, SDL_Window **childWindow, SDL_Renderer **childRenderer) {
    if (mouseX >= mainWindowX + windowWidth - 100 && mouseY >= mainWindowY + windowHeight - 100) {
        if (*childWindow == NULL) {
            *childWindow = SDL_CreateWindow(
                "Fenêtre Fils",
                mainWindowX + W_WIDTH / 2 - CHILD_W / 2,
                mainWindowY + W_HEIGHT / 2 - CHILD_H / 2,
                CHILD_W, CHILD_H,
                SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS
            );

            if (*childWindow == NULL) {
                printf("Erreur création de la fenêtre fils : %s\n", SDL_GetError());
                return;
            } else {
                *childRenderer = SDL_CreateRenderer(*childWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
                if (*childRenderer == NULL) {
                    printf("Erreur création du rendu fils : %s\n", SDL_GetError());
                    SDL_DestroyWindow(*childWindow);
                    *childWindow = NULL;
                    return;
                }
            }
        }
    }

    if (*childWindow && *childRenderer) {
        int quit = 0;
        SDL_Event event;

        // Texte d'entrée
        char inputText[3][128] = {"", "", ""}; // Zones réduites
        SDL_Surface *textSurface = NULL;
        SDL_Texture *textTexture = NULL;

        // Positions et dimensions des boîtes d'entrée (réduites pour être visibles)
        SDL_Rect inputBoxes[3] = {
            {10, 30, CHILD_W - 20, 40},  // "Nom de l'abonnement"
            {10, 100, CHILD_W - 20, 40}, // "Date"
            {10, 170, CHILD_W - 20, 40}  // "Prix"
        };

        // Étiquettes au-dessus des boîtes
        const char *labels[3] = {"Nom de l'abonnement", "Date(DD/MM/YYYY)", "Prix"};

        // Texte sélectionné (0 = "Nom de l'abonnement", 1 = "Date", 2 = "Prix")
        int selectedBox = 0;

        SDL_StartTextInput(); // Activer la saisie de texte
        while (!quit) {
            // Gestion des événements
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    quit = 1;
                } else if (event.type == SDL_KEYDOWN) {
                    // Gestion des touches
                    if (event.key.keysym.sym == SDLK_TAB) {
                        // Passer à la boîte suivante
                        selectedBox = (selectedBox + 1) % 3;
                    } else if (event.key.keysym.sym == SDLK_BACKSPACE && strlen(inputText[selectedBox]) > 0) {
                        inputText[selectedBox][strlen(inputText[selectedBox]) - 1] = '\0'; // Supprimer le dernier caractère
                    } 
                } else if (event.type == SDL_TEXTINPUT) {
                    // Ajouter du texte si pas de modificateur (Ctrl, Alt, etc.)
                    if (!(SDL_GetModState() & KMOD_CTRL) && !(SDL_GetModState() & KMOD_ALT)) {
                        if (strlen(inputText[selectedBox]) + strlen(event.text.text) < sizeof(inputText[selectedBox])) {
                            strcat(inputText[selectedBox], event.text.text); // Ajouter le texte à l'entrée active
                        }
                    }
                } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    int childMouseX, childMouseY;
                    SDL_GetGlobalMouseState(&childMouseX, &childMouseY);

                    int childX, childY;
                    SDL_GetWindowPosition(*childWindow, &childX, &childY);

                    // Coordonnées de la zone de fermeture (en bas à droite de la fenêtre enfant)
                    int closeZoneXStart = childX + CHILD_W - 50;
                    int closeZoneYStart = childY + CHILD_H - 50;
                    int closeZoneXEnd = childX + CHILD_W;
                    int closeZoneYEnd = childY + CHILD_H;

                    // Coordonnées de la zone pour enregistrer
                    int saveZoneXStart = childX;
                    int saveZoneYStart = childY;
                    int saveZoneXEnd = childX + 50;
                    int saveZoneYEnd = childY + 50;

                    // Vérifiez si le clic est dans la zone de sauvegarde
                    if (childMouseX >= saveZoneXStart && childMouseX <= saveZoneXEnd &&
                        childMouseY >= saveZoneYStart && childMouseY <= saveZoneYEnd) {
                        printf("Sauvegarde des données : Nom: %s, Date: %s, Prix: %s\n", inputText[0], inputText[1], inputText[2]);

                        int day, month, year;
                        extractDate(inputText[1], &day, &month, &year);

                        int price = atoi(inputText[2]);

                        write_to_json("data.json", day, month, year, inputText[0], price);

                        strcpy(inputText[0], "");
                        strcpy(inputText[1], "");
                        strcpy(inputText[2], "");
                        selectedBox = 0;
                    }
                    // Vérifiez si le clic est dans la zone de fermeture
                    if (childMouseX >= closeZoneXStart && childMouseX <= closeZoneXEnd &&
                        childMouseY >= closeZoneYStart && childMouseY <= closeZoneYEnd) {
                        SDL_DestroyRenderer(*childRenderer);
                        SDL_DestroyWindow(*childWindow);
                        *childWindow = NULL;
                        *childRenderer = NULL;
                        printf("Fenêtre fils fermée depuis la zone\n");
                        return;
                    }
                    

                }
            }

            // Effacer l'écran de la fenêtre enfant
            SDL_SetRenderDrawColor(*childRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(*childRenderer);

            // Dessiner les étiquettes et les boîtes d'entrée
            for (int i = 0; i < 3; i++) {
                // Dessiner l'étiquette
                textSurface = TTF_RenderText_Solid(font, labels[i], textColor);
                textTexture = SDL_CreateTextureFromSurface(*childRenderer, textSurface);
                SDL_Rect labelRect = {inputBoxes[i].x, inputBoxes[i].y - 25, 0, 0};
                SDL_QueryTexture(textTexture, NULL, NULL, &labelRect.w, &labelRect.h);
                SDL_RenderCopy(*childRenderer, textTexture, NULL, &labelRect);
                SDL_FreeSurface(textSurface);
                SDL_DestroyTexture(textTexture);

                // Dessiner la boîte d'entrée
                SDL_SetRenderDrawColor(*childRenderer, boxColor.r, boxColor.g, boxColor.b, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(*childRenderer, &inputBoxes[i]);

                // Afficher le texte de l'entrée
                if (strlen(inputText[i]) > 0) {
                    textSurface = TTF_RenderText_Solid(font, inputText[i], textColor);
                    textTexture = SDL_CreateTextureFromSurface(*childRenderer, textSurface);
                    SDL_Rect textRect = {inputBoxes[i].x + 10, inputBoxes[i].y + 10, 0, 0};
                    SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
                    SDL_RenderCopy(*childRenderer, textTexture, NULL, &textRect);
                    SDL_FreeSurface(textSurface);
                    SDL_DestroyTexture(textTexture);
                }
            }

            // Afficher le rendu
            SDL_RenderPresent(*childRenderer);
        }
        SDL_StopTextInput(); // Désactiver la saisie de texte
    }
}





int main(int argc, char* argv[]){
    /*mois année affiché par le tableau*/
    int currentMonth = 12;
    char* currentMonthString = "DEC";
    int currentYear = 2024;
    int currentMonthYear[2] = {currentMonth, currentYear};



    SDL_Window *window = NULL;  
    SDL_Window *childWindow = NULL;  // Fenêtre fils
    SDL_Renderer *renderer = NULL;
    SDL_Renderer *childRenderer = NULL;  // Rendu de la fenêtre fils

    SDL_Surface *surface = NULL;
    SDL_Texture *texture = NULL;

    /*textures textes*/
    SDL_Texture *monthTexture = NULL;
    SDL_Texture *dayTexture = NULL;
    SDL_Texture *entriesTexture = NULL;

    /*json parsé*/
    struct json_object *parsedJson;
    struct json_object *monthJson;
    struct json_object *dayJson;
    struct json_object *entriesJson;
    entriesTabStruct entriesTab;
    dayButtonsTabStruct dayButtonsTab;
    existingDateTabStruct existingDateTab;

    /* Charger et parser le JSON initial */
    parsedJson = json_object_from_file("data.json");
    if (!parsedJson) {
        printf("Erreur : Impossible de charger le fichier JSON\n");
        SDL_Quit();
        return 1;
    }

    monthJson = getMonthJson(parsedJson, currentMonthYear);

    /* Boutons pour les jours et les dates existantes */
    dayButtonsTab = refreshDayButtons(currentMonth, currentYear);
    existingDateTab = getExistingDateTab(monthJson);

    /* Attribuer aux boutons s'ils sont cliquables ou non */
    for (int i = 0; i < dayButtonsTab.dayButtonNumber; i++) {
        dayButtonsTab.dayButtons[i].usable = existingDateTab.date[i];
    }

    

    /*boutons pour les fleches des mois : h, w, x, y*/
    int previousMonthButton[4] = {40, 40, 80, 80};
    int nextMonthButton[4] = {40, 40, 240, 80};


    /*lancement sdl*/
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        return 1;
    }

    // Initialisation de SDL_ttf
    if (TTF_Init() == -1) {
        return 1;
    }


    /* Création de la fenêtre principale */
    window = SDL_CreateWindow("Calendrier", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W_WIDTH, W_HEIGHT, 0);
    if (window == NULL) {
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

       
    int parentX, parentY;
    SDL_GetWindowPosition(window, &parentX, &parentY);
    

    

    /*textes*/
    // Charger une police
    TTF_Font* font = TTF_OpenFont("DejaVuSans.ttf", 40);
    if (font == NULL) {
        // Gestion d'erreur
        perror("[font]");
        return 1;
    }
    TTF_Font* font2 = TTF_OpenFont("DejaVuSans.ttf", 20);
    if (font2 == NULL) {
        // Gestion d'erreur
        perror("[font]");
        return 1;
    }

    // Couleur du texte
    SDL_Color color = {0, 0, 0}; // Noir


    // Rectangle pour positionner le texte
    int w, h;
    SDL_Rect monthRect;
    monthRect.x = 120;
    monthRect.y = 80;
    
    SDL_Rect dayRect;
    dayRect.x = 360;
    dayRect.y = 120;
    
    SDL_Rect entriesRect;
    entriesRect.x = 360;
    entriesRect.y = 200;
    

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


    /*main*/
    int quit = 0;
    SDL_Event event;
    while(!quit){
        /*evenement*/
        while(SDL_PollEvent(&event)){
            switch (event.type){
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                        if (event.window.windowID == SDL_GetWindowID(childWindow)) {
                            SDL_DestroyWindow(childWindow);
                            childWindow = NULL;
                        } else if (event.window.windowID == SDL_GetWindowID(window)) {
                            quit = 1;
                        }
                    }
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
                            
                            existingDateTab = getExistingDateTab(monthJson);
                            /*attribut aux boutons le fait qu'ils soient cliquables ou non*/
                            for(int i = 0; i < dayButtonsTab.dayButtonNumber; i++){
                                dayButtonsTab.dayButtons[i].usable = existingDateTab.date[i];
                            }
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

                            existingDateTab = getExistingDateTab(monthJson);
                            /*attribut aux boutons le fait qu'ils soient cliquables ou non*/
                            for(int i = 0; i < dayButtonsTab.dayButtonNumber; i++){
                                dayButtonsTab.dayButtons[i].usable = existingDateTab.date[i];
                            }
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
                        }
                    /*boutons jour*/
                    else{
                        for(int i = 0; i < dayButtonsTab.dayButtonNumber; i++){
                            if(
                                (event.motion.x <= (dayButtonsTab.dayButtons[i].x + dayButtonsTab.dayButtons[i].w)) && 
                                (event.motion.x >= (dayButtonsTab.dayButtons[i].x)) &&
                                (event.motion.y <= (dayButtonsTab.dayButtons[i].y + dayButtonsTab.dayButtons[i].h)) && 
                                (event.motion.y >= (dayButtonsTab.dayButtons[i].y)) &&
                                (dayButtonsTab.dayButtons[i].usable == 1))
                                {
                                printf("bouton %d cliqué\n", dayButtonsTab.dayButtons[i].id);
                                dayJson = getDayJson(monthJson, dayButtonsTab.dayButtons[i].id);
                                entriesJson = getEntriesJson(dayJson);
                                entriesTab = getEntriesTab(entriesJson);
                                /*ici normalement on a un tableau structure avec tous les entrées d'un jour d'un mois*/
                                for(int j = 0; j < entriesTab.entriesNumber; j++){
                                    printf("%s : %d\n", entriesTab.entries[j].entryName, entriesTab.entries[j].entryValue);
                                }
                                /*effacer ecran*/
                                SDL_RenderClear(renderer);
                                /*afficher fond*/
                                SDL_RenderCopy(renderer, texture, NULL, &bgRect);
                                //mois
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
                                
                                //jour
                                char dateSTR[8];
                                sprintf(dateSTR, "%d/%d/%d", dayButtonsTab.dayButtons[i].id, currentMonthYear[0], currentMonthYear[1]);
                                // Créer une surface contenant le texte
                                surface = TTF_RenderText_Solid(font, dateSTR, color);
                                // Créer une texture à partir de la surface
                                dayTexture = SDL_CreateTextureFromSurface(renderer, surface);
                                SDL_FreeSurface(surface);
                                SDL_QueryTexture(dayTexture, NULL, NULL, &w, &h);
                                dayRect.w = w;
                                dayRect.h = h;
                                // Copier la texture sur le renderer
                                SDL_RenderCopy(renderer, dayTexture, NULL, &dayRect);

                                //entries
                                char entriesSTR[1024] = "";
                                for(int i = 0; i < entriesTab.entriesNumber; i++){
                                    char entrySTR[64];
                                    sprintf(entrySTR, "-%s : %d \n", entriesTab.entries[i].entryName, entriesTab.entries[i].entryValue);
                                    strcat(entriesSTR, entrySTR);
                                }
                                // Créer une surface contenant le texte
                                surface = TTF_RenderText_Blended_Wrapped(font2, entriesSTR, color, 240);
                                // Créer une texture à partir de la surface
                                entriesTexture = SDL_CreateTextureFromSurface(renderer, surface);
                                SDL_FreeSurface(surface);
                                SDL_QueryTexture(entriesTexture, NULL, NULL, &w, &h);
                                entriesRect.w = w;
                                entriesRect.h = h;
                                // Copier la texture sur le renderer
                                SDL_RenderCopy(renderer, entriesTexture, NULL, &entriesRect);

                                /*afficher le rendu*/
                                SDL_RenderPresent(renderer);
                            }
                        }
                    }
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int mouseX, mouseY;
                        SDL_GetGlobalMouseState(&mouseX, &mouseY);

                        // Vérifiez si le clic est en bas à droite
                        if (!childWindow) {
                            int windowWidth, windowHeight;
                            SDL_GetWindowSize(window, &windowWidth, &windowHeight);

                            // Si le clic est en bas à droite de la fenêtre principale
                            int mainWindowX, mainWindowY;
                            SDL_GetWindowPosition(window, &mainWindowX, &mainWindowY);
                            SDL_Color textColor = {0, 0, 0}; // Noir
                            SDL_Color boxColor = {200, 200, 200}; // Gris clair
                            inputBox_withchild(childRenderer, font2, textColor, boxColor,mouseX, mouseY, mainWindowX, mainWindowY, windowWidth, windowHeight,&childWindow,&childRenderer);
                            json_object_put(parsedJson);  // Libérer la mémoire du JSON précédent
                            parsedJson = json_object_from_file("data.json");
                            if (parsedJson) {
                                monthJson = getMonthJson(parsedJson, currentMonthYear);
                                dayButtonsTab = refreshDayButtons(currentMonth, currentYear);
                                existingDateTab = getExistingDateTab(monthJson);

                                for (int i = 0; i < dayButtonsTab.dayButtonNumber; i++) {
                                    dayButtonsTab.dayButtons[i].usable = existingDateTab.date[i];
                                }
                            } 
                }  
                continue;
                
                case SDL_MOUSEMOTION:
                    printf("%d %d\n", event.motion.x, event.motion.y);
                    continue;
                default:
                    break;
            }

        }
    }
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (parsedJson){
        json_object_put(parsedJson);// Libérer la mémoire du JSON 
    }
    SDL_Quit();
    return 0;
}

