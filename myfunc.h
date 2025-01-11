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


struct json_object *getDayJson(struct json_object *monthJson, int wantedDay){
    struct json_object *day;
    struct json_object *date;

    int dayDate;

    int daysNumber = json_object_array_length(monthJson);

    for(int i = 0; i < daysNumber; i++){
        day = json_object_array_get_idx(monthJson, i);
        date = json_object_object_get(day, "date");
        dayDate = json_object_get_int(json_object_array_get_idx(date, 0));

        if(wantedDay == dayDate){
            return day;
        }
    }
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