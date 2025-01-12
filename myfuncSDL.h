#include "mystructSDL.h"

/*il faut ajouter le fait que ce soit un lundi mardi etc pour pouvoir les positionner correctement sur le tableau c'est pour ca que ya wantedYear*/
dayButtonsTabStruct refreshDayButtons(int wantedMonth, int wantedYear){
    dayButtonsTabStruct dayButtonsTab;
    int dayButtonNumb;

    /*regarde le nombre de jour dans le mois*/
    dayButtonsTab.dayButtonNumber = 31;

    dayButtonsTab.dayButtons = (dayButton *)malloc(dayButtonsTab.dayButtonNumber * sizeof(dayButton));
    
    /*création des boutons*/
    for(int i = 0; i < dayButtonsTab.dayButtonNumber; i++){
        int row = (i / 7);
        int column = (i % 7);
        dayButtonsTab.dayButtons[i].id = i + 1;
        dayButtonsTab.dayButtons[i].h = 40;
        dayButtonsTab.dayButtons[i].w = 40;
        dayButtonsTab.dayButtons[i].x = 40 + (column * 40);
        dayButtonsTab.dayButtons[i].y = 160 + (row * 40);
        dayButtonsTab.dayButtons[i].usable = 0;
    }
    return dayButtonsTab;
}

char* getMonthStr(int month) {
    static char* months[] = {
        "JAN", "FEB", "MAR", "APR",
        "MAI", "JUN", "JUL", "AUG",
        "SEP", "OCT", "NOV", "DEC"
    };

    return months[month - 1];
}