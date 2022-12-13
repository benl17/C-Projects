#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Structure that represents a magic square
typedef struct {
    int size;           // dimension of the square
    int **magic_square; // pointer to heap allocated magic square
} MagicSquare;

/* 
 * Prompts the user for the magic square's size, reads it,
 * checks if it's an odd number >= 3 (if not display the required
 * error message and exit), and returns the valid number.
 */
int getSize() {
    printf("%s", "Enter magic square's size (odd integer >=3)\n");
    
    int userSize = 0;
    scanf("%i", &userSize); //user input from terminal
    
    if(userSize % 2 == 0) { //checks that userSize is not even
        printf("%s", "Magic square size must be odd.\n");
        exit(1);
    } else if(userSize < 3) { //check that userSize is not less than three
        printf("%s", "Magic square size must be >= 3.\n");
        exit(1);
    }
    return userSize;   
} 
   
/* 
 * Makes a magic square of size n using the alternate 
 * Siamese magic square algorithm from assignment and 
 * returns a pointer to the completed MagicSquare struct.
 *
 * n the number of rows and columns
 */
MagicSquare *generateMagicSquare(int n) {
    MagicSquare *newMagicSquare; //create a new MagicSquare struct
    newMagicSquare = malloc(sizeof(MagicSquare)); //allocate space for new struct
    if(newMagicSquare == NULL) { 
        printf("Memory not initialized correctly.");
        exit(1);
    }

    newMagicSquare->size = n; //set newMagicSquare size to n

    //Dynamically allocate temp array and check that there are no null values
    newMagicSquare->magic_square = malloc(sizeof(int*) * n);
    if(newMagicSquare->magic_square == NULL) {
        printf("Memory not initialized correctly");
        exit(1);
    }
    //Dynamically allocate the next array for ints
    for(int t = 0; t < newMagicSquare->size; t++) {
        *(newMagicSquare->magic_square + t) = malloc(sizeof(int) * n);
        if(*(newMagicSquare->magic_square + t) == NULL) {
            printf("Memory not initialized correctly");
            exit(1);
        }
    }
    //set every element to 0 in the array
    for(int y = 0; y < newMagicSquare->size; y++) { 
        for(int r = 0; r < newMagicSquare->size; r++) {
            (*(*(newMagicSquare->magic_square + y) + r)) = 0;
        }
    }

    //fill up the array to create the magic square
    int size = n; //used for bound checking
    int number = 1; //first number to be added
    int row = 1; //current index
    int temp = size/2.0; //used for floor function
    int column = floor(temp) - 1; //gets the center index
    int currRow;
    int currColumn;
    
    //Siamese Method
    for(int e = 0; e < n * n; e++) {
        row = row - 1;
        column = column + 1;
        if(row < 0) { //checks bounds
            row = size - 1;
        }
        if(column >= size) {//checks bounds
            column = 0;
        }
        if(*(*(newMagicSquare->magic_square + row) + column) == 0) { //checks that the number at this index is 0
            *(*(newMagicSquare->magic_square + row) + column) = number;//assigns the current number to this index
            number++; //increment number
        } else {//The number at the previous index was not 0 so move down one row (or up depending on how you think of the square)
            column = currColumn;
            row = currRow + 1;
            *(*(newMagicSquare->magic_square + row) + column) = number;
            number++; //increment number
        }
        currRow = row; //used to help keep track 
        currColumn = column; //used to help keep track
    }
    return newMagicSquare;  
} 

/*   
 * Opens a new file (or overwrites the existing file)
 * and writes the square in the specified format.
 *
 * magic_square the magic square to write to a file
 * filename the name of the output file
 */
void fileOutputMagicSquare(MagicSquare *magic_square, char *filename) {
    FILE *fp = fopen(filename, "w");

    //check file isn't null
    if(fp == NULL) {
        printf("Cannot open file, please try again.\n");
        exit(1);
    }

    //write to file
    int size = magic_square->size;
    fprintf(fp, "%i\n", size);

    //write the contents of magic_square into the output file given 
    for(int i = 0; i < size; i++) { 
        for(int t = 0; t < size; t++) {
            if(t != 0) { //check to see if t is index 0, if isn't then add a comma for correct format
                fprintf(fp, ", %i", *(*(magic_square->magic_square + i) + t));
            } else { //else it is at 0 so you don't need to add a comma 
                fprintf(fp, "%i", *(*(magic_square->magic_square + i) + t));
            }
        }
        fprintf(fp, "%s", "\n");
    }

    //Check for errors when closing the file 
    if (fclose(fp) != 0) {
        printf("Error while closing the file.\n");
        exit(1);
    } 
}

/* 
 * Generates a magic square of the user specified size and
 * output the quare to the output filename
 *
 * argc: the number of command line args (CLAs)
 * argv: the CLA strings, includes the program name
 */
int main(int argc, char **argv) {
    // TODO: Check input arguments to get output filename
    if(argc != 2){
        printf("%s", "Usage: ./myMagicSquare <output_filename>\n");
        exit(1);
    }
    char *filename = *(argv + 1);

    // TODO: Get magic square's size from user
    int boardSize = getSize();

    // TODO: Generate the magic square
    MagicSquare *myMagicSquare = generateMagicSquare(boardSize);
    
    // TODO: Output the magic square
    fileOutputMagicSquare(myMagicSquare, filename); 

    //free memory so no memory leaks
    for(int r = 0; r < myMagicSquare->size; r++) {
        free(*(myMagicSquare->magic_square + r));
        *(myMagicSquare->magic_square + r) = NULL;
    }
    free(myMagicSquare->magic_square);
    myMagicSquare->magic_square = NULL;
    free(myMagicSquare);
    myMagicSquare = NULL;

    return 0;
}      
