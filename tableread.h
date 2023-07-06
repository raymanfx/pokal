#ifndef TABLEREAD_H
#define TABLEREAD_H

void readFile(char *);
char * getTableData(int row, int col);
int getTableRowCount();
int getYearNow();
int getGamesInTheYear(int year);
char * getWinnerOfTheYear(int year);
void freeTable();

#endif // POKAL_EPAPER_GUI_H
