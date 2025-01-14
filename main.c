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
#include <time.h>
#include "myfunc.h"
#include "myfuncSDL.h"
#include <string.h>
#include <ctype.h>


#define W_HEIGHT 480
#define W_WIDTH 640
#define CHILD_W 420
#define CHILD_H 340


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

void populateFutureDates(struct json_object *parsedJson, int day, int month, int year, const char *entryName, int entryValue, int recurrenceType) {
    if (!parsedJson) return;

    if (recurrenceType == 1) { // Récurrence hebdomadaire
    for (int futureYear = year; futureYear <= year + 10; futureYear++) {
        for (int futureWeek = 0; futureWeek < 52; futureWeek++) {
            struct tm date = {0};
            date.tm_mday = day;
            date.tm_mon = month - 1; // Mois : 0 = Janvier
            date.tm_year = year - 1900;
            date.tm_hour = 12;

            time_t time = mktime(&date) + futureWeek * 7 * 24 * 60 * 60; // Ajouter une semaine
            struct tm *futureDate = localtime(&time);

            if (futureDate->tm_year + 1900 > year + 10) break;

            // Vérifier si la date existe déjà
            int found = 0;
            for (int i = 0; i < json_object_array_length(parsedJson); i++) {
                struct json_object *entry = json_object_array_get_idx(parsedJson, i);
                struct json_object *dateObj;

                if (json_object_object_get_ex(entry, "date", &dateObj)) {
                    if (json_object_get_int(json_object_array_get_idx(dateObj, 0)) == futureDate->tm_mday &&
                        json_object_get_int(json_object_array_get_idx(dateObj, 1)) == futureDate->tm_mon + 1 &&
                        json_object_get_int(json_object_array_get_idx(dateObj, 2)) == futureDate->tm_year + 1900) {
                        // Vérifier si l'abonnement existe déjà dans les entrées
                        struct json_object *entriesArray;
                        if (json_object_object_get_ex(entry, "entries", &entriesArray)) {
                            int exists = 0;
                            for (int j = 0; j < json_object_array_length(entriesArray); j++) {
                                struct json_object *existingEntry = json_object_array_get_idx(entriesArray, j);
                                if (strcmp(json_object_get_string(json_object_array_get_idx(existingEntry, 0)), entryName) == 0 &&
                                    json_object_get_int(json_object_array_get_idx(existingEntry, 1)) == entryValue) {
                                    exists = 1;
                                    break;
                                }
                            }

                            // Ajouter uniquement si l'abonnement n'existe pas encore
                            if (!exists) {
                                struct json_object *newEntry = json_object_new_array();
                                json_object_array_add(newEntry, json_object_new_string(entryName));
                                json_object_array_add(newEntry, json_object_new_int(entryValue));
                                json_object_array_add(entriesArray, newEntry);
                            }
                        }
                        found = 1;
                        break;
                    }
                }
            }

            if (!found) {
                // Ajouter une nouvelle date avec l'abonnement
                struct json_object *newDateObj = json_object_new_object();
                struct json_object *dateArray = json_object_new_array();
                json_object_array_add(dateArray, json_object_new_int(futureDate->tm_mday));
                json_object_array_add(dateArray, json_object_new_int(futureDate->tm_mon + 1));
                json_object_array_add(dateArray, json_object_new_int(futureDate->tm_year + 1900));
                json_object_object_add(newDateObj, "date", dateArray);

                struct json_object *entriesArray = json_object_new_array();
                struct json_object *newEntry = json_object_new_array();
                json_object_array_add(newEntry, json_object_new_string(entryName));
                json_object_array_add(newEntry, json_object_new_int(entryValue));
                json_object_array_add(entriesArray, newEntry);
                json_object_object_add(newDateObj, "entries", entriesArray);

                json_object_array_add(parsedJson, newDateObj);
            }
        }
    }
    } else if (recurrenceType == 2) { // Récurrence mensuelle
        for (int futureYear = year; futureYear <= year + 10; futureYear++) {
            for (int futureMonth = (futureYear == year ? month : 1); futureMonth <= 12; futureMonth++) {
                int found = 0;
                for (int i = 0; i < json_object_array_length(parsedJson); i++) {
                    struct json_object *entry = json_object_array_get_idx(parsedJson, i);
                    struct json_object *dateObj;
                    if (json_object_object_get_ex(entry, "date", &dateObj)) {
                        if (json_object_get_int(json_object_array_get_idx(dateObj, 0)) == day &&
                            json_object_get_int(json_object_array_get_idx(dateObj, 1)) == futureMonth &&
                            json_object_get_int(json_object_array_get_idx(dateObj, 2)) == futureYear) {
                            struct json_object *entriesArray;
                            if (json_object_object_get_ex(entry, "entries", &entriesArray)) {
                                struct json_object *newEntry = json_object_new_array();
                                json_object_array_add(newEntry, json_object_new_string(entryName));
                                json_object_array_add(newEntry, json_object_new_int(entryValue));
                                json_object_array_add(entriesArray, newEntry);
                            }
                            found = 1;
                            break;
                        }
                    }
                }

                if (!found) {
                    struct json_object *newDateObj = json_object_new_object();
                    struct json_object *dateArray = json_object_new_array();
                    json_object_array_add(dateArray, json_object_new_int(day));
                    json_object_array_add(dateArray, json_object_new_int(futureMonth));
                    json_object_array_add(dateArray, json_object_new_int(futureYear));
                    json_object_object_add(newDateObj, "date", dateArray);

                    struct json_object *entriesArray = json_object_new_array();
                    struct json_object *newEntry = json_object_new_array();
                    json_object_array_add(newEntry, json_object_new_string(entryName));
                    json_object_array_add(newEntry, json_object_new_int(entryValue));
                    json_object_array_add(entriesArray, newEntry);
                    json_object_object_add(newDateObj, "entries", entriesArray);

                    json_object_array_add(parsedJson, newDateObj);
                }
            }
        }
    } else if (recurrenceType == 3) { // Récurrence annuelle
        for (int futureYear = year; futureYear <= year + 10; futureYear++) {
            int found = 0;
            for (int i = 0; i < json_object_array_length(parsedJson); i++) {
                struct json_object *entry = json_object_array_get_idx(parsedJson, i);
                struct json_object *dateObj;
                if (json_object_object_get_ex(entry, "date", &dateObj)) {
                    if (json_object_get_int(json_object_array_get_idx(dateObj, 0)) == day &&
                        json_object_get_int(json_object_array_get_idx(dateObj, 1)) == month &&
                        json_object_get_int(json_object_array_get_idx(dateObj, 2)) == futureYear) {
                        struct json_object *entriesArray;
                        if (json_object_object_get_ex(entry, "entries", &entriesArray)) {
                            struct json_object *newEntry = json_object_new_array();
                            json_object_array_add(newEntry, json_object_new_string(entryName));
                            json_object_array_add(newEntry, json_object_new_int(entryValue));
                            json_object_array_add(entriesArray, newEntry);
                        }
                        found = 1;
                        break;
                    }
                }
            }

            if (!found) {
                struct json_object *newDateObj = json_object_new_object();
                struct json_object *dateArray = json_object_new_array();
                json_object_array_add(dateArray, json_object_new_int(day));
                json_object_array_add(dateArray, json_object_new_int(month));
                json_object_array_add(dateArray, json_object_new_int(futureYear));
                json_object_object_add(newDateObj, "date", dateArray);

                struct json_object *entriesArray = json_object_new_array();
                struct json_object *newEntry = json_object_new_array();
                json_object_array_add(newEntry, json_object_new_string(entryName));
                json_object_array_add(newEntry, json_object_new_int(entryValue));
                json_object_array_add(entriesArray, newEntry);
                json_object_object_add(newDateObj, "entries", entriesArray);

                json_object_array_add(parsedJson, newDateObj);
            }
        }
    } else {
        printf("Erreur : Type de récurrence invalide.\n");
    }
}

// Fonction pour vérifier si une chaîne est une date valide au format dd/mm/yyyy
int isValidDate(const char *date) {
    if (strlen(date) != 10) return 0; // Longueur exacte : 10 caractères (dd/mm/yyyy)

    // Vérification du format
    if (date[2] != '/' || date[5] != '/') return 0; // Les '/' doivent être aux positions 2 et 5

    // Extraction du jour, du mois et de l'année
    int day = atoi(date);
    int month = atoi(date + 3);
    int year = atoi(date + 6);

    // Vérification des plages valides
    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 1000 || year > 9999) return 0;

    // Gestion des mois avec 30 jours
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) return 0;

    // Gestion de février (année bissextile ou non)
    if (month == 2) {
        int isLeap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        if (day > 29 || (day == 29 && !isLeap)) return 0;
    }

    return 1; // La date est valide
}

// Fonction pour vérifier si une chaîne est un prix valide (composée uniquement de chiffres)
int isValidPrice(const char *price) {
    for (int i = 0; price[i] != '\0'; i++) {
        if (!isdigit(price[i])) return 0; // Si un caractère n'est pas un chiffre
    }
    return 1; // Le prix est valide
}


// Fonction pour dessiner les boîtes d'entrée et gérer le curseur
void drawInputBoxes(SDL_Renderer *renderer, TTF_Font *font, SDL_Color textColor, SDL_Color boxColor, 
                    SDL_Rect *inputBoxes, const char **labels, char inputText[][128], int selectedBox, int *cursorPosition, int showCursor) {
    for (int i = 0; i < 3; i++) {
        // Dessiner la boîte
        SDL_SetRenderDrawColor(renderer, boxColor.r, boxColor.g, boxColor.b, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &inputBoxes[i]);

        // Afficher l'étiquette
        SDL_Surface *textSurface = TTF_RenderText_Solid(font, labels[i], textColor);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect labelRect = {inputBoxes[i].x, inputBoxes[i].y - 25, 0, 0};
        SDL_QueryTexture(textTexture, NULL, NULL, &labelRect.w, &labelRect.h);
        SDL_RenderCopy(renderer, textTexture, NULL, &labelRect);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        // Afficher le texte d'entrée
        if (strlen(inputText[i]) > 0) {
            textSurface = TTF_RenderText_Solid(font, inputText[i], textColor);
            textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {inputBoxes[i].x + 10, inputBoxes[i].y + 10, 0, 0};
            SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
            SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);
        }

        // Dessiner le curseur
        if (selectedBox == i && showCursor) {
            int cursorX = inputBoxes[i].x + 10;
            if (strlen(inputText[i]) > 0) {
                TTF_SizeText(font, inputText[i], &cursorX, NULL);
                cursorX += inputBoxes[i].x + 10;
            }
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawLine(renderer, cursorX, inputBoxes[i].y + 10, cursorX, inputBoxes[i].y + inputBoxes[i].h - 10);
        }
    }
}

// Fonction pour dessiner les boîtes de récurrence
void drawRecurrenceBoxes(SDL_Renderer *renderer, TTF_Font *font, SDL_Color textColor, SDL_Color boxColor, 
                         SDL_Rect *weeklyBox, SDL_Rect *monthlyBox, SDL_Rect *yearlyBox, int recurrenceType) {
    SDL_SetRenderDrawColor(renderer, (recurrenceType == 1 ? 0 : boxColor.r), boxColor.g, boxColor.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, weeklyBox);

    SDL_SetRenderDrawColor(renderer, (recurrenceType == 2 ? 0 : boxColor.r), boxColor.g, boxColor.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, monthlyBox);

    SDL_SetRenderDrawColor(renderer, (recurrenceType == 3 ? 0 : boxColor.r), boxColor.g, boxColor.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, yearlyBox);

    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "H", textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {weeklyBox->x + 5, weeklyBox->y + 5, 0, 0};
    SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    textSurface = TTF_RenderText_Solid(font, "M", textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = (SDL_Rect){monthlyBox->x + 5, monthlyBox->y + 5, 0, 0};
    SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);

    textSurface = TTF_RenderText_Solid(font, "A", textColor);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    textRect = (SDL_Rect){yearlyBox->x + 5, yearlyBox->y + 5, 0, 0};
    SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

// Fonction pour dessiner la boîte Annuler
void drawCancelBox(SDL_Renderer *renderer, TTF_Font *font, SDL_Color textColor, SDL_Color boxColor, SDL_Rect *cancelBox) {
    SDL_SetRenderDrawColor(renderer, boxColor.r, boxColor.g, boxColor.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, cancelBox);

    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Annuler", textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {cancelBox->x + 5, cancelBox->y + 5, 0, 0};
    SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}
// Fonction pour dessiner la boîte Confirmer
void drawConfirmBox(SDL_Renderer *renderer, TTF_Font *font, SDL_Color textColor, SDL_Color boxColor, SDL_Rect *confirmBox) {
    SDL_SetRenderDrawColor(renderer, boxColor.r, boxColor.g, boxColor.b, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, confirmBox);

    SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Confirmer", textColor);
    SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {confirmBox->x + 5, confirmBox->y + 5, 0, 0};
    SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

// Fonction pour gérer les entrées clavier
void handleTextInput(SDL_Event *event, char inputText[][128], int *cursorPosition, int selectedBox) {
    if (selectedBox != -1) {
        if (event->type == SDL_KEYDOWN) {
            if (event->key.keysym.sym == SDLK_BACKSPACE && cursorPosition[selectedBox] > 0) {
                inputText[selectedBox][--cursorPosition[selectedBox]] = '\0';
            }
        } else if (event->type == SDL_TEXTINPUT) {
            if (strlen(inputText[selectedBox]) + strlen(event->text.text) < sizeof(inputText[selectedBox])) {
                strcat(inputText[selectedBox], event->text.text);
                cursorPosition[selectedBox]++;
            }
        }
    }
}

// Fonction pour gérer les clics souris
void handleMouseClick(SDL_Event *event, SDL_Rect *inputBoxes, int *selectedBox, SDL_Rect *weeklyBox, 
                      SDL_Rect *monthlyBox, SDL_Rect *yearlyBox, SDL_Rect *cancelBox, SDL_Rect *confirmBox,
                      int *recurrenceType, SDL_Renderer **childRenderer, SDL_Window **childWindow,
                      char inputText[][128], int *cursorPosition, int *quit, struct json_object *parsedJson) {
    int x, y;
    SDL_GetMouseState(&x, &y);

    for (int i = 0; i < 3; i++) {
        if (x >= inputBoxes[i].x && x <= inputBoxes[i].x + inputBoxes[i].w &&
            y >= inputBoxes[i].y && y <= inputBoxes[i].y + inputBoxes[i].h) {
            *selectedBox = i;
            cursorPosition[i] = strlen(inputText[i]);
        }
    }

    if (x >= weeklyBox->x && x <= weeklyBox->x + weeklyBox->w &&
        y >= weeklyBox->y && y <= weeklyBox->y + weeklyBox->h) {
        *recurrenceType = 1;
    } else if (x >= monthlyBox->x && x <= monthlyBox->x + monthlyBox->w &&
               y >= monthlyBox->y && y <= monthlyBox->y + monthlyBox->h) {
        *recurrenceType = 2;
    } else if (x >= yearlyBox->x && x <= yearlyBox->x + yearlyBox->w &&
               y >= yearlyBox->y && y <= yearlyBox->y + yearlyBox->h) {
        *recurrenceType = 3;
    } else if (x >= cancelBox->x && x <= cancelBox->x + cancelBox->w &&
               y >= cancelBox->y && y <= cancelBox->y + cancelBox->h) {
        SDL_DestroyRenderer(*childRenderer);
        SDL_DestroyWindow(*childWindow);
        *childWindow = NULL;
        *childRenderer = NULL;
        *quit = 1;
        printf("Fenêtre fils fermée depuis la zone\n");
    } else if (x >= confirmBox->x && x <= confirmBox->x + confirmBox->w &&
               y >= confirmBox->y && y <= confirmBox->y + confirmBox->h) {
        // Validation des données
        if (!isValidDate(inputText[1]) || !isValidPrice(inputText[2])) {
            // Afficher un message d'erreur dans la fenêtre enfant
            TTF_Font* font = TTF_OpenFont("DejaVuSans.ttf", 20);
            SDL_Color textColor = {255, 0, 0}; // Rouge
            SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Veuillez remplir avec le bon format.", textColor);
            if (textSurface) {
                SDL_Texture *textTexture = SDL_CreateTextureFromSurface(*childRenderer, textSurface);
                if (textTexture) {
                    SDL_Rect textRect = {0, 0, 0, 0};
                    SDL_QueryTexture(textTexture, NULL, NULL, &textRect.w, &textRect.h);
                    textRect.x = (CHILD_W - textRect.w) / 2; // Centré horizontalement
                    textRect.y = CHILD_H - textRect.h - 10; // Position en bas avec un padding de 10 pixels
                    SDL_RenderCopy(*childRenderer, textTexture, NULL, &textRect);

                    // Libérer la texture après l'affichage
                    SDL_DestroyTexture(textTexture);
                }
                SDL_FreeSurface(textSurface);
            }
            SDL_RenderPresent(*childRenderer); // Rafraîchir l'affichage
            return;
        }

        // Si les données sont valides, sauvegarder
        if (strlen(inputText[0]) > 0 && strlen(inputText[1]) > 0 && strlen(inputText[2]) > 0) {
            int day, month, year;
            extractDate(inputText[1], &day, &month, &year);
            int price = atoi(inputText[2]);

            if (parsedJson != NULL) {
                populateFutureDates(parsedJson, day, month, year, inputText[0], price, *recurrenceType);

                // Sauvegarder les modifications dans le fichier JSON
                if (json_object_to_file_ext("data.json", parsedJson, JSON_C_TO_STRING_PRETTY) == -1) {
                    printf("Erreur lors de la sauvegarde après ajout des dates récurrentes.\n");
                } else {
                    printf("Données sauvegardées avec succès avec récurrence.\n");
                }
            } else {
                printf("Erreur : JSON principal non initialisé.\n");
            }

            // Réinitialiser les champs de texte
            strcpy(inputText[0], "");
            strcpy(inputText[1], "");
            strcpy(inputText[2], "");
            *selectedBox = -1;

            SDL_DestroyRenderer(*childRenderer);
            SDL_DestroyWindow(*childWindow);
            *childWindow = NULL;
            *childRenderer = NULL;
            *quit = 1;
            printf("Fenêtre fils fermée depuis la zone\n");
        } else {
            printf("Erreur : Champs vides détectés.\n");
        }
    }
}



void inputBox_withchild(SDL_Renderer *renderer, TTF_Font *font, 
                        int mouseX, int mouseY, int mainWindowX, int mainWindowY, 
                        int windowWidth, int windowHeight, SDL_Window **childWindow, SDL_Renderer **childRenderer, struct json_object *parsedJson) {

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

        char inputText[3][128] = {"", "", ""}; // Zones réduites
        int cursorPosition[3] = {0, 0, 0};     // Position du curseur dans chaque champ
        int showCursor = 1;                    // Clignotement du curseur
        Uint32 lastBlinkTime = SDL_GetTicks(); // Temps pour le clignotement
        SDL_Color textColor = {0, 0, 0};       // Noir
        SDL_Color boxColor = {200, 200, 200};  // Gris clair

        SDL_Rect inputBoxes[3] = {
            {10, 30, CHILD_W - 20, 40},  // "Nom de l'abonnement"
            {10, 100, CHILD_W - 20, 40}, // "Date"
            {10, 170, CHILD_W - 20, 40}  // "Prix"
        };

        SDL_Rect weeklyBox = {10, 220, 40, 40};  // Hebdomadaire
        SDL_Rect monthlyBox = {120, 220, 40, 40}; // Mensuel
        SDL_Rect yearlyBox = {230, 220, 40, 40};  // Annuel
        SDL_Rect cancelBox = {10, 280, 100, 40};  // Annuler
        SDL_Rect ConfirmBox = {300, 280, 110, 40};  // Confirmer


        const char *labels[3] = {"Nom de l'abonnement", "Date(DD/MM/YYYY)", "Prix"};
        int recurrenceType = 1; // 1 = Hebdomadaire, 2 = Mensuel, 3 = Annuel
        int selectedBox = -1;

        SDL_StartTextInput(); // Activer la saisie de texte

        while (!quit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    quit = 1;
                } else if (event.type == SDL_KEYDOWN || event.type == SDL_TEXTINPUT) {
                    handleTextInput(&event, inputText, cursorPosition, selectedBox);
                } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    handleMouseClick(&event, inputBoxes, &selectedBox, &weeklyBox, &monthlyBox, &yearlyBox, &cancelBox,&ConfirmBox, 
                                     &recurrenceType, childRenderer,childWindow, inputText, cursorPosition,&quit,parsedJson);
                }
            }

            // Gestion du clignotement du curseur
            if (SDL_GetTicks() - lastBlinkTime > 500) {
                showCursor = !showCursor;
                lastBlinkTime = SDL_GetTicks();
            }

            // Effacer l'écran
            SDL_SetRenderDrawColor(*childRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(*childRenderer);

            // Dessiner les composants
            drawInputBoxes(*childRenderer, font, textColor, boxColor, inputBoxes, labels, inputText, selectedBox, cursorPosition, showCursor);
            drawRecurrenceBoxes(*childRenderer, font, textColor, boxColor, &weeklyBox, &monthlyBox, &yearlyBox, recurrenceType);
            drawCancelBox(*childRenderer, font, textColor, boxColor, &cancelBox);
            drawConfirmBox(*childRenderer, font, textColor, boxColor, &ConfirmBox);

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
                            quit=1;
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
                            
                            inputBox_withchild(childRenderer, font2,mouseX, mouseY, mainWindowX, mainWindowY, windowWidth, windowHeight,&childWindow,&childRenderer,parsedJson);
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

