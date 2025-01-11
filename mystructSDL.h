typedef struct dayButton_ {
    int id;
    int h;
    int w;
    int x;
    int y;
    int usable;
}dayButton;

typedef struct dayButtonsTabStruct_ {
    dayButton *dayButtons;
    int dayButtonNumber;
}dayButtonsTabStruct;
