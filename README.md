# SudokuSolver

Initially built in 2006, I wrote this program when Sudoku craze started and I got frustrated by the rote application of predetermined rules.

Early versions relied on a brute force splitting of boards and recursively finding the valid solutions amongst the many possible combinations.

The program has evolved to incorporate more deterministic strategies so that very few (only the most difficult) puzzles still rely on branching the board and solving recursively.

Now the longest part of solving a sudoku puzzle is creating the input file -- something that looks like this: 
    001004000
    090200005
    000080000
    000007040
    600000900
    207000860
    006010000
    000000453
    500830070

Note that I use 0s to show unknown/blank spaces in the input file.

The SudokuBoard.cpp file has a setting for verbosity of output.  

Many possible extensions / improvements could be made:
* web interface and publish tool to the world (others exist)
* more determistic rules to minimize branching
* more command line options, like verbosity setting
* support different shaped puzzles (6x6)

-jb

Jonathan Beamer
jbeamer@gmail.com