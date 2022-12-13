#include <stdio.h>
#include <stdlib.h>
#include <string.h>
     
char *DELIM = ",";  // commas ',' are a common delimiter character for data strings
     
/*      
 * Read the first line of input file to get the size of that board.
 * 
 * PRE-CONDITION #1: file exists
 * PRE-CONDITION #2: first line of file contains valid non-zero integer value
 *
 * fptr: file pointer for the board's input file
 * size: a pointer to an int to store the size
 */
void get_board_size(FILE *fptr, int *size) {      
    char *line1 = NULL;
    size_t len = 0;

    if ( getline(&line1, &len, fptr) == -1 ) {
        printf("Error reading the input file.\n");
		free(line1);
        exit(1);
    }

    char *size_chars = NULL;
    size_chars = strtok(line1, DELIM);
    *size = atoi(size_chars);

	// free memory allocated for reading first link of file
	free(line1);
	line1 = NULL;
}



/* 
 * Returns 1 if and only if the board is in a valid Sudoku board state.
 * Otherwise returns 0.
 * 
 * A valid row or column contains only blanks or the digits 1-size, 
 * with no duplicate digits, where size is the value 1 to 9.
 * 
 * Note: p2A requires only that each row and each column are valid.
 * 
 * board: heap allocated 2D array of integers 
 * size:  number of rows and columns in the board
 */
int valid_board(int **board, int size) {
    for(int u = 0; u < size; u++) {
        for(int k = 0; k < size; k++) {
            if(*(*(board + u) + k) == 0) {
                continue;
            }
            for(int b = k + 1; b < size; b++) { //checks columns 
                if(*(*(board + u) + k) == (*(*(board + u) + b))) {
                    return 0; //duplicate, invalid board
                }
            }
            for(int c = u +1; c < size; c++) { //checks rows
                if(*(*(board + c) + k) == (*(*(board + u)+ k))) { 
                    return 0;//duplicate, invalid board
                }
            }
        }
    }
    return 1;//valid board if reached
}    
  
 
   
/* 
 * This program prints "valid" (without quotes) if the input file contains
 * a valid state of a Sudoku puzzle board wrt to rows and columns only.
 *
 * A single CLA which is the name of the file that contains board data 
 * is required.
 *
 * argc: the number of command line args (CLAs)
 * argv: the CLA strings, includes the program name
 */
int main( int argc, char **argv ) { 
                
    //Check if number of command-line arguments is correct.
	if(argc != 2) {
		printf("%s\n", "invalid");
		exit(1);
	}
    // Open the file and check if it opened successfully.
    FILE *fp = fopen(*(argv + 1), "r");
    if (fp == NULL) {
        printf("Can't open file for reading.\n");
        exit(1);
    }

    // Declare local variables.
    int size;

    //Call get_board_size to read first line of file as the board size.
	get_board_size(fp, &size);
    if(size > 9 || size < 1) { //makes sure size is within bounds of 1-9 
        printf("%s\n", "invalid");
        exit(0);
    }
    //Dynamically allocate a 2D array for given board size.
	int  **sudokuBoard;
    sudokuBoard = malloc(sizeof(int*) * size);
    for(int r = 0; r < size; r++) {
        *(sudokuBoard + r) = malloc(sizeof(int)*size);
    }

    // Read the rest of the file line by line.
    // Tokenize each line wrt the delimiter character 
    // and store the values in your 2D array.
    char *line = NULL;
    size_t len = 0;
    char *token = NULL;
    for (int i = 0; i < size; i++) {

        if (getline(&line, &len, fp) == -1) {
            printf("Error while reading line %i of the file.\n", i+2);
            exit(1);
        }

        token = strtok(line, DELIM);
        for (int j = 0; j < size; j++) {
            int currInt = atoi(token); //current int 
            (*(*(sudokuBoard + i) + j)) = currInt; //put int into current array location
            token = strtok(NULL, DELIM);
        }
    }

    //checks to make sure that sudokuBoard is not an array of 0's or null values
    if(sudokuBoard == NULL) {
        printf("%s\n", "invalid");
        for(int z = 0; z < size; z++) {
            free(*(sudokuBoard + z));
            *(sudokuBoard + z) = NULL;
        }
        free(sudokuBoard);
        sudokuBoard = NULL;
        exit(0);
    }

    // Call the function valid_board and print the appropriate
    // output depending on the function's return value.
    int boardValidity = valid_board(sudokuBoard, size);
    if(boardValidity == 1) { 
        printf("%s\n", "valid");
        exit(0); //valid board
    } else {
        printf("%s\n", "invalid");
        exit(0); //invalid board
    }

    //Free all dynamically allocated memory.
    for(int z = 0; z < size; z++) {
        free(*(sudokuBoard + z));
        *(sudokuBoard + z) = NULL;
    }
    free(sudokuBoard);
    sudokuBoard = NULL;

    //Close the file.
    if (fclose(fp) != 0) {
        printf("Error while closing the file.\n");
        exit(1);
    } 
    return 0;       
}       
