// SudokuSolver.cpp
// Written by Jonathan Beamer
// Copyright (c) 2005-2008

#include <stdio.h>
#include "sudokuboard.h"

#define VERSION_STRING		"1.1"

int
main(int argc, char* argv[])
{
	int iterations;

	printf("Sudoku Solver v%s\nBy Jonathan Beamer\nCopyright (c) 2005-2016\n", VERSION_STRING);
	printf("Input file: %s\n", (argc>1 ? argv[1] : "none"));

	// TODO -- board size should be set by the puzzle file read in
	CSudokuBoard cBoard("1", 9);
	if (!cBoard.ReadInFile( (argc>1 ? argv[1] : NULL) ))
		return 1;

	printf("\nUnsolved Puzzle:\n");
	cBoard.Print();

	iterations = cBoard.Solve();

	printf("\nSummary:\n  Solutions:  %d\n  Branches:   %d\n  Iterations: %d\n\n", g_nSolutionsFound, g_nBranchEndpoitsFound, iterations);
	return 0;
}
