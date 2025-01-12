typedef struct entryStruct_ {
    char *entryName;
    int entryValue;
}entryStruct;


typedef struct entriesTabStruct_ {
    entryStruct *entries;
    int entriesNumber;
}entriesTabStruct;

typedef struct existingDateTabStruct_ {
    int* date;
    int dateNumber;
}existingDateTabStruct;
