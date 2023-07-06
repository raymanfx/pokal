
#include "tableread.h"
#include "fs.h"

lfs_soff_t size;
char *buffer;
char data[20];
int dataCount=0;

void readFile(char *fileName){

    lfs_file_t file;

    // read current db file
    lfs_mount(&lfs, &lfs_cfg);

    lfs_file_open(&lfs, &file, fileName, LFS_O_RDONLY);
    size=lfs_file_size(&lfs, &file);
    printf("buffersize: %i\n",size); 

    buffer = (char *) calloc(size,sizeof(char));
    if(buffer != NULL) {
		printf("Mem is reserved\n");
	}else {
		printf("No mem free!!!\n");
	}

    lfs_file_read(&lfs, &file, buffer, size*sizeof(buffer));

    lfs_file_close(&lfs, &file);
    lfs_unmount(&lfs);
}

char * getTableData(int row, int col){
    int rowCount=0;
    int colCount=0;
    dataCount=0;

    memset(&data[0], 0, sizeof(data));
    
    for (int i = 0; i < size; i++) {
        if(buffer[i] == ',')
        {
            colCount++;
        }
        else if(buffer[i] == '\n')
        {   
            colCount=0;
            rowCount++;
        }
        else if(colCount==col&&rowCount==row){
            //printf("%c", *(buffer + i));
            data[dataCount]=*(buffer + i);
            dataCount++;
        }
    }

    return data;
}

int getTableRowCount(){
    int rowCount=0;
    
    for (int i = 0; i < size; i++) {
        if(buffer[i] == '\n')
        {   
            rowCount++;
        }
    }
    return rowCount;
}

int getYearNow(){
    int rowCount=getTableRowCount();
    int yearnow;
    int year=0;

    for (int i = 0; i < rowCount; i++) {
        yearnow=2000;
        yearnow+=((*(getTableData(i,0) + 6))-48)*10;
        yearnow+=(*(getTableData(i,0) + 7)-48);
        if(yearnow>year)year=yearnow;
    }
    return year;
}

int getGamesInTheYear(int year){
    int rowCount=getTableRowCount();
    int yearnow;
    int gamesCount=0;

    for (int i = 0; i < rowCount; i++) {
        yearnow=2000;
        yearnow+=((*(getTableData(i,0) + 6))-48)*10;
        yearnow+=(*(getTableData(i,0) + 7)-48);
        if(yearnow==year)gamesCount++;
    }
    return gamesCount;
}

char * getWinnerOfTheYear(int year){
    int rowCount=getTableRowCount();
    int yearnow;
    char name[20];

    memset(&data[0], 0, sizeof(data));

    for (int i = 0; i < rowCount; i++) {
        yearnow=2000;
        yearnow+=((*(getTableData(i,0) + 6))-48)*10;
        yearnow+=(*(getTableData(i,0) + 7)-48);
        if(yearnow==year){
            for (int j = 0; j < 20; j++) {
                name[j]=*(getTableData(i,2) + j);
            }
            if(strcmp(name,data)==0)strcpy(data,name);
            //noch nicht fertig!
        }
    }

    return data;
}

void freeTable(){
    free(buffer);
    printf("Mem is free\n");
}