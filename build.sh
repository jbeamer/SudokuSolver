#!/bin/bash

gcc -lc++ SudokuSolver.cpp SudokuBoard.cpp
mv a.out SudokuSolver
chmod +x SudokuSolver
./SudokuSolver ./puzzle.txt > output.txt
