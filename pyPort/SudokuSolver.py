# SudokuSolver.py
# Written by Jonathan Beamer
# Copyright (c) 2005-2016
# --
# python port September, 2016

import sys
import SudokuBoard

# global variables
VERSION_STRING = "1.3"
COPYRIGHT_STRING = "Copyright (c) 2005-2021"
USAGE = "Usage: python SudokuSolver.py [puzzle_filename]"


def main():
    print(f'Sudoku Solver v{VERSION_STRING}\nBy Jonathan Beamer\n{COPYRIGHT_STRING}')

    if len(sys.argv) < 2:
        print(USAGE)
        sys.exit(1)

    board = SudokuBoard.SudokuBoard("1")
    if (board.ReadInFile(sys.argv[1])):
        print(USAGE)
        sys.exit(1)

    print('\nUnsolved Puzzle:')
    board.PrintCompact()

    nSolutionsFound = board.Solve()


if __name__ == "__main__":
    main()
