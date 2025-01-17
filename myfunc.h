#include <string.h>
#include "mystruct.h"

struct json_object *getParsedJson(){
    struct json_object *parsedJson = NULL;
    
    FILE *data;
    char buffer[1024];

    data = fopen("data.json", "r");
    fread(buffer, 1024, 1, data);
    fclose(data);

    parsedJson = json_tokener_parse(buffer);

    return parsedJson;
}


struct json_object *getMonthJson(struct json_object *parsedJson, int wantedMonth[2]){
    struct json_object *monthJson = json_object_new_array();
    struct json_object *day;
    struct json_object *date;

    int dayDate[2];

    int daysNumber = json_object_array_length(parsedJson);

    for(int i = 0; i < daysNumber; i++){
        day = json_object_array_get_idx(parsedJson, i);
        date = json_object_object_get(day, "date");
        dayDate[0] = json_object_get_int(json_object_array_get_idx(date, 1));
        dayDate[1] = json_object_get_int(json_object_array_get_idx(date, 2));
        if(wantedMonth[0] == dayDate[0] && wantedMonth[1] == dayDate[1]){
            json_object_array_add(monthJson, day);
        }
    }
    return monthJson;
}

/*créer une structure tableau qui va servir a savoir si un bouton est cliquable ou non*/
existingDateTabStruct getExistingDateTab(struct json_object *monthJson){
    existingDateTabStruct tab;
    struct json_object *day;
    struct json_object *date;
    int dateNbr;

    int daysNumber = json_object_array_length(monthJson);

    tab.dateNumber = 31;

    tab.date = (int *)malloc(tab.dateNumber * sizeof(int));

    for(int i = 0; i < tab.dateNumber; i++){
        tab.date[i] = 0;
    }
    
    for(int i = 0; i < daysNumber; i++){
        day = json_object_array_get_idx(monthJson, i);
        date = json_object_object_get(day, "date");
        dateNbr = json_object_get_int(json_object_array_get_idx(date, 0));
        tab.date[dateNbr - 1] = 1;
    }
    return tab;
}


// Fonction mise à jour pour obtenir les données d'un jour spécifique
struct json_object *getDayJson(struct json_object *monthJson, int day) {
    for (int i = 0; i < json_object_array_length(monthJson); i++) {
        struct json_object *entry = json_object_array_get_idx(monthJson, i);
        struct json_object *dateObj;

        if (json_object_object_get_ex(entry, "date", &dateObj)) {
            if (json_object_get_int(json_object_array_get_idx(dateObj, 0)) == day) {
                return entry;
            }
        }
    }
    return NULL;
}


struct json_object *getEntriesJson(struct json_object *dayJson){
    struct json_object *entries = json_object_object_get(dayJson, "entries");
    
    return entries;
}


entriesTabStruct getEntriesTab(struct json_object *entriesJson){
    struct json_object *entry;
    entriesTabStruct entriesTab;
    entriesTab.entriesNumber = 0;
    int entriesNumber = json_object_array_length(entriesJson);

    entriesTab.entries = malloc(entriesNumber * sizeof(entryStruct));
    
    for(int i = 0; i < entriesNumber; i++){
        entry = json_object_array_get_idx(entriesJson, i);
        entriesTab.entries[i].entryName = strdup(json_object_get_string(json_object_array_get_idx(entry, 0)));
        entriesTab.entries[i].entryValue = json_object_get_int(json_object_array_get_idx(entry, 1));
        entriesTab.entriesNumber++;
    }
    return entriesTab;
}