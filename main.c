#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>

#define BLOCK_SIZE 4096
#define BLOCK_HEADER 4
#define INFO_BLOCK_SIZE 128
#define TABLE_NAME_SIZE 16
#define COLUMN_NAME_SIZE 12
#define FIXED_RECORD_SIZE 16

void MakeTable();
int MainMenu();
int SelectTable();
void InsertRecord(int);
void InitTable(FILE*);
int GetFreeSpace(unsigned char *);
int SetFreeSpace(unsigned char *, int);
void SearchRecord(int);

int main() {
    int mainOption, tableOption, subOption;
    char test[10] = {0,};

    while (1) {
        mainOption = MainMenu();

        switch (mainOption) {
            case 1:
                MakeTable();
                break;
            case 2:
                tableOption = SelectTable();

                printf("1. Insert record\n");
                printf("2. Search record\n");
                scanf_s(" %d", &subOption);
                getchar();

                switch (subOption) {
                    case 1:
                        InsertRecord(tableOption);
                        break;
                    case 2:
                        SearchRecord(tableOption);
                        break;
                    default:
                        printf("Wrong input. Try again\n\n");
                        break;
                }
                break;
            case 3:
                exit(1);
                break;
            default:
                printf("Wrong input. Try again\n\n");
                break;
        }
    }
}

void SearchRecord(int tableIndex) {
    char *tableName = calloc(TABLE_NAME_SIZE, sizeof(char));
    char *columnNum = calloc(1, sizeof(char));
    int attrNum, selectColumn;
    int findFlag = 0;

    FILE *info = fopen("info.txt", "r+");

    fseek(info, (tableIndex - 1) * INFO_BLOCK_SIZE, SEEK_SET);

    fread(tableName, TABLE_NAME_SIZE, 1, info);
    fread(columnNum, 1, 1, info);

    printf("You are now searching records in %s...\n", tableName);
    printf("Select one column that you want to find.\n");
    printf("Column List ---------- \n");

    strcat(tableName, ".txt");

    FILE *table = fopen(tableName, "r+");

    attrNum = atoi(columnNum);

    char *columnList[attrNum];
    char *columnType[attrNum];

    //print column list
    for (int i = 0; i < attrNum; i++) {
        columnList[i] = calloc(COLUMN_NAME_SIZE, sizeof(char));
        columnType[i] = calloc(1, sizeof(char));

        fread(columnType[i], 1, 1, info);
        fread(columnList[i], COLUMN_NAME_SIZE, 1, info);

        printf("%d. %s(type: %s)\n", i + 1, columnList[i], columnType[i]);
    }

    printf("-> ");
    scanf_s(" %d", &selectColumn);
    selectColumn--;

    unsigned char *blockEntry = calloc(4, sizeof(char));
    char findData[16] = {0,};
    int recordNum;

    printf("Enter what you want to find -> ");
    scanf_s(" %s", findData);

    fread(blockEntry, BLOCK_HEADER, 1, table);
    recordNum = blockEntry[0];

    for (int i = 0; i < recordNum; i++) {
        short *recordHeader = calloc(2, sizeof(short));
        char buffer[16] = {0,};
        int recordSize, recordOffset;
        unsigned char *bitmap = calloc(1, sizeof(char));
        int bitmapArray[attrNum];
        int columnOffset = 0;
        int offsetFlag = 0;
        int dataSize;
        memset(bitmapArray, 0, attrNum * sizeof(int));

        fseek(table, (i + 1) * 4, SEEK_SET);
        fread(recordHeader, 4, 1, table);

        recordSize = recordHeader[0];
        recordOffset = recordHeader[1];

        fseek(table, recordOffset, SEEK_SET);

        //variable column
        if (atoi(columnType[selectColumn]) != 0) {
            for (int j = 0; j < selectColumn; j++) {
                if (atoi(columnType[j]) != 0) {
                    fseek(table, 4, SEEK_CUR);
                } else {
                    fseek(table, FIXED_RECORD_SIZE, SEEK_CUR);
                }
            }
            short *rSize = calloc(2, sizeof(short));
            fread(rSize, 4, 1, table);
            columnOffset = rSize[0];
            dataSize = rSize[1];

        //fixed column
        } else {
            for (int j = 0; j < selectColumn; j++) {
                if (atoi(columnType[selectColumn]) != 0) {
                    columnOffset += 4;
                } else {
                    columnOffset += FIXED_RECORD_SIZE;
                }
            }
            dataSize = 16;
            columnOffset += recordOffset;
        }

        fseek(table, columnOffset, SEEK_SET);
        fread(buffer, dataSize, 1, table);

        //If system found data
        if (strcmp(findData, buffer) == 0) {
            findFlag = 1;
            printf("Found data...\n");

            for (int j = 0; j < attrNum; j++) {
                printf("%10s", columnList[j]);
            }

            printf("\n-------------------------------------\n");

            fseek(table, recordOffset, SEEK_SET);

            for (int j = 0; j < attrNum; j++) {
                if (atoi(columnType[j]) != 0) {
                    short *aSize = calloc(2, sizeof(short));
                    fread(aSize, 4, 1, table);

                    int cOffset = aSize[0];
                    int cSize = aSize[1];

                    int temp = ftell(table);
                    fseek(table, cOffset, SEEK_SET);

                    char *data = calloc(cSize, sizeof(char));
                    fread(data, cSize, 1, table);
                    printf("%10s", data);
                    fseek(table, temp, SEEK_SET);
                } else {
                    char *data = calloc(16, sizeof(char));
                    fread(data, 16, 1, table);
                    printf("%10s", data);
                }
            }
            printf("\n");

        }
    }

    if (findFlag == 0) {
        printf("Cannot find...\n");
    }

    fclose(info);
    fclose(table);
}

int GetFreeSpace(unsigned char *blockEntry) {
    int result =  ((int)blockEntry[1] << 16) |  ((int)blockEntry[2] << 8) | (int) blockEntry[3];
    return result;
}

int SetFreeSpace(unsigned char *blockEntry, int size) {
    int currFreeSpace = GetFreeSpace(blockEntry);
    int newFreeSpace = currFreeSpace - size;
    blockEntry[0] += 1;
    blockEntry[1] = (unsigned char)(newFreeSpace >> 16);
    blockEntry[2] = (unsigned char)(newFreeSpace >> 8);
    blockEntry[3] = (unsigned char)newFreeSpace;

    return newFreeSpace;
}

void InsertRecord(int tableIndex) {
    int infoSize, tableNum, tableOption;
    char *tableName = calloc(TABLE_NAME_SIZE, sizeof(char));
    char *columnNum = calloc(1, sizeof(char));
    char buffer[16] = {0,};
    int attrNum;
    unsigned char bitmap = 0;
    unsigned char *blockEntry = calloc(4, sizeof(char));

    FILE *info = fopen("info.txt", "r+");

    fseek(info, (tableIndex - 1) * INFO_BLOCK_SIZE, SEEK_SET);

    fread(tableName, TABLE_NAME_SIZE, 1, info);
    fread(columnNum, 1, 1, info);

    printf("You are now inserting records in %s...\n", tableName);
    printf("If you want to enter NULL value, just press enter.\n\n");
    printf("Column Name          Your input\n");
    printf("-------------------------------\n");

    strcat(tableName, ".txt");
    FILE *table = fopen(tableName, "r+");

    attrNum = atoi(columnNum);

    char *columnName[attrNum];
    char *columnType[attrNum];
    char *record[attrNum];
    int recordSize = 1;
    int variableIndex = 1;
    int bitmapArray[attrNum];
    memset(bitmapArray, 0, attrNum * sizeof(int));

    for (int i = 0; i < attrNum; i++) {
        columnName[i] = calloc(COLUMN_NAME_SIZE, sizeof(char));
        columnType[i] = calloc(1, sizeof(char));
        record[i] = calloc(16, sizeof(char));

        fread(columnType[i], 1, 1, info);
        fread(columnName[i], COLUMN_NAME_SIZE, 1, info);

        printf("%10s          -> ", columnName[i]);
        gets(buffer);

        if (buffer[0] == 0) {
            record[i] = NULL;
            bitmapArray[i] = 1;
        } else {
            strcpy(record[i], buffer);
            unsigned char bitmask = (unsigned char) pow(2, (attrNum - i - 1));
            bitmap |= bitmask;
            bitmapArray[i] = 0;

            //column type is variable
            if (strcmp(columnType[i], "1") == 0) {
                recordSize += ((int)strlen(record[i]) + 4);
                variableIndex += 4;
            //column type is fixed
            } else {
                recordSize += FIXED_RECORD_SIZE;
                variableIndex += FIXED_RECORD_SIZE;
            }
        }
        memset(buffer, 0, 16);
    }

    //new free space
    int freeSpace;
    short *newRecordHeader = calloc(2, sizeof(short));
    rewind(table);
    fread(blockEntry, BLOCK_HEADER, 1, table);
    freeSpace = SetFreeSpace(blockEntry, recordSize);
    variableIndex += freeSpace;

    //set new block Entry
    rewind(table);
    fwrite(blockEntry, _msize(blockEntry), 1, table);

    //write record header
    fseek(table, blockEntry[0] * BLOCK_HEADER, SEEK_SET);
    newRecordHeader[0] = (short) recordSize;
    newRecordHeader[1] = (short) freeSpace;
    fwrite(newRecordHeader, _msize(newRecordHeader), 1, table);

    //write record
    for (int i = 0; i < attrNum; i++) {
        //if column is null
        if (bitmapArray[i] == 1) {
            continue;
        //if not
        } else {
            //if column type is variable
            if (strcmp(columnType[i], "1") == 0) {
                short *variableOffset = calloc(2, sizeof(short));
                fseek(table, freeSpace, SEEK_SET);
                variableOffset[0] = (short) variableIndex;
                variableOffset[1] = (short) strlen(record[i]);
                fwrite(variableOffset, _msize(variableOffset), 1, table);
                freeSpace += 4;

                fseek(table, variableIndex, SEEK_SET);
                fwrite(record[i], strlen(record[i]), 1, table);
                variableIndex += (int) strlen(record[i]);

                free(variableOffset);

            //if column type is fixed
            } else {
                fseek(table, freeSpace, SEEK_SET);
                fwrite(record[i], _msize(record[i]), 1, table);
                freeSpace += FIXED_RECORD_SIZE;
            }
        }
    }

    fclose(info);
    fclose(table);

}

void InitTable(FILE *fp) {
    unsigned char *blockEntry = calloc(4, sizeof(char));
    char *block = calloc(BLOCK_SIZE, sizeof(char));

    fwrite(block, BLOCK_SIZE, 1, fp);
    rewind(fp);

    blockEntry[0] = 0;
    blockEntry[1] = (char)(BLOCK_SIZE >> 16);
    blockEntry[2] = (char)(BLOCK_SIZE >> 8);
    blockEntry[3] = (char)BLOCK_SIZE;

    fwrite(blockEntry, _msize(blockEntry), 1, fp);
    free(blockEntry);
}

int SelectTable() {
    int infoSize, tableNum, tableOption;
    FILE *info = fopen("info.txt", "r+");

    fseek(info, 0, SEEK_END);
    infoSize = ftell(info);
    tableNum = (infoSize / INFO_BLOCK_SIZE);

    printf("--------Table List--------\n");

    //print table list
    for (int i = 0; i < tableNum; i++) {
        char *tableName = calloc(TABLE_NAME_SIZE, sizeof(char));
        fseek(info, i * INFO_BLOCK_SIZE, SEEK_SET);
        fread(tableName, TABLE_NAME_SIZE, 1, info);
        printf("%d. %s\n", i+1, tableName);

        free(tableName);
    }
    fclose(info);

    printf("-> ");
    scanf_s(" %d", &tableOption);
    getchar();

    return tableOption;
}

void MakeTable() {
    char *tableName = calloc(TABLE_NAME_SIZE, sizeof(char));
    char *columnNum = calloc(1,  sizeof(char));
    char *buffer    = calloc(16, sizeof(char));
    char *blankInfo = calloc(INFO_BLOCK_SIZE, sizeof(char));
    int infoSize, tableNum;

    FILE *info = fopen("info.txt", "r+");

    fseek(info, 0, SEEK_END);
    infoSize = ftell(info);
    tableNum = (infoSize / INFO_BLOCK_SIZE);

    fseek(info, infoSize, SEEK_SET);
    fwrite(blankInfo, INFO_BLOCK_SIZE, 1, info);

    fseek(info, infoSize, SEEK_SET);

    //table name
    printf("Please write table name(Maximum length is 10): ");
    scanf_s(" %s", buffer);
    strcpy(tableName, buffer);
    memset(buffer, 0, 10);

    fwrite(tableName, _msize(tableName), 1, info);

    //column num
    printf("Please write attribute number(Maximum 9): ");
    scanf_s(" %s", buffer);
    strcpy(columnNum, buffer);
    memset(buffer, 0, 10);

    fwrite(columnNum, _msize(columnNum), 1, info);

    //column type, name
    for (int i = 0; i < atoi(columnNum); i++) {
        char *attrName = calloc(12, sizeof(char));
        char *attrType = calloc(1, sizeof(char));
        printf("Please enter attribute type(variable: 1, fixed: 0) and name: \n");
        printf("-attribute type: ");
        scanf_s(" %s", buffer);
        strcpy(attrType, buffer);
        memset(buffer, 0, 10);

        printf("-attribute name: ");
        scanf_s(" %s", buffer);
        strcpy(attrName, buffer);
        memset(buffer, 0, 10);

        fwrite(attrType, _msize(attrType), 1, info);
        fwrite(attrName, _msize(attrName), 1, info);

        free(attrName);
        free(attrType);
    }

    strcat(tableName, ".txt");

    //make table
    FILE *fp = fopen(tableName, "w+");

    InitTable(fp);

    free(tableName);
    free(columnNum);
    free(buffer);
    free(blankInfo);

    fclose(fp);
    fclose(info);
}

int MainMenu() {
    int mainOption;

    printf("--------MENU--------\n");
    printf("1. Make table\n");
    printf("2. Select table\n");
    printf("3. Exit\n");
    printf("-> ");

    scanf_s(" %d", &mainOption);
    getchar();

    return mainOption;
}




