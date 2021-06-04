import sys
import os
import itertools

STATE_INCOMPLETE = 1
STATE_INVALID    = 2
STATE_SOLVED     = 3

VERBOSE_CRITICAL = 0
VERBOSE_NORMAL   = 1
VERBOSE_WORDY    = 2
VERBOSE_EXTREME  = 3
g_verbosity      = VERBOSE_NORMAL

class SudokuCell:
    def __init__(self, i, j):
        self.m_value = 0
        # 1-indexed array of flags that indicates possible values for this cell.  e.g: "could I be X?"
        # TODO: change this to an array of integers 1 thru 9:
        self.m_possibleValues = [i for i in range(1,10)]
        self.m_location = (i,j)

    def __str__(self):
        if self.m_value != 0:
            return '{self.m_value}'.format(self=self)
        return " "

    def SetValue(self, newvalue):
        if (self.m_value == newvalue):
            # we are already set to this value, return False to indicate no change
            return False

        if (self.m_value != 0):
            print("ERROR: asked to set new value for a cell that already has a value")
            return False;
            
        # clear all of our flags except for our value:
        self.m_value = newvalue
        self.m_possibleValues = [newvalue]

        return True

    def ClearFlag(self, value):
        """SudokuCell::ClearFlag: clears the flag, returns if this is new info"""
        if value == 0:
            print(f"WARNING: trying to clear value 0")
            return 0
            
        # we don't allow flag to be cleared if this cell is this value
        # this actually happens a lot, as the parent group is lazy.
        if self.m_value == value:
            print(f"WARNING: trying to clear value for known cell")
            return 0
            
        if value in self.m_possibleValues:
            self.m_possibleValues.remove(value) 
            if (g_verbosity >= VERBOSE_EXTREME):
                print(f"cleared possibility {value} in row {self.m_location[0] + 1} column {self.m_location[1] + 1}")
            return 1
            
        return 0

    def UpdateValue(self):
        """SudokuCell::UpdateValue: at the cell level, UpdateValue justs
           checks to see if it can only be one possible value, and 
           updates itself.  Returns 1 if a change is made, 0 if not """
        # if we don't know what value we are:
        if (self.m_value == 0):
            # if we only have one possibility, set us to the possibility
            if (len(self.m_possibleValues) == 1):
                self.m_value = self.m_possibleValues[0]
                if (g_verbosity >= VERBOSE_WORDY):
                    print(f"Placed {self.m_value} in row {self.m_location[0] + 1} column {self.m_location[1] +1} because it is the only option for that cell")
                return 1

        # no change made
        return 0

    def NumPossible(self):
        return len(self.m_possibleValues)

    def CouldBe(self, value):
        return value in self.m_possibleValues

    def GetFlagChar(self, index):
        """used for printing the full version of the matrix, this helper function returns the printable 
           character for this flag, if it's possible for us, a space character if not, or our known value
           in the case of index 5 and we are known"""
        if self.m_value == 0:
            # we are not a known value, print appropriate flag value if we can be this one, space otherwise:
            if index in self.m_possibleValues:
                return str(index)
        elif index == 5:
            # we know our value and we are in the cener of the 3x3, so return our value char
            return str(self.m_value)
        return " "
                    
class SudokuGroup:
    def __init__(self, cellArray, cellIndices, groupType, index):
        """ cellIndices is an array of (x,y) tuples """
        self.m_type   = groupType
        self.m_index = index
        self.m_cells  = [cellArray[x][y] for (x,y) in cellIndices]    

    def SetValues(self, strValues):
        # used at start to set the values of the ROW from an input file
        assert(len(strValues) == 9)
        for i in range(9):
            ch = strValues[i]
            value = ord(strValues[i]) - ord('0')
            assert(value >= 0 and value <= 9)
            self.m_cells[i].SetValue(value)

    def __str__(self):
        """used for compact printing, just creates a row string like | 12|3 4|  5|"""
        groupString = "|"
        for i in range(9):
            groupString = f"{groupString}{self.m_cells[i]}"
            if i % 3 == 2:
                groupString += "|"
        return groupString

    def PrintSubrow(self, subrow):
        # this is called on a row, to print 1/3 of the flags for the entire row
        # subrow is an index 0-2
        outString = "|||"
        for col in range(9):
            for flag in range(subrow*3,subrow*3+3):
                outString = outString + self.m_cells[col].GetFlagChar(flag+1)
            outString = outString + "|"
            if col % 3 == 2:
                outString = outString + "|"
        print(outString + "|")
                
    def CurrentState(self):
        
        # check to see if it ever can be filled:
        possibles = [cell.NumPossible() for cell in self.m_cells]
        if 0 in possibles:
            return STATE_INVALID

        nNumbersFound = [cell.m_value for cell in self.m_cells]
        if 0 in nNumbersFound:
            return STATE_INCOMPLETE
        
        if (len(nNumbersFound) == 9):
            return STATE_SOLVED

        return STATE_INCOMPLETE
        
    def ClearFlags(self):
        # standard clearing: looks for numbers in row, col, group)
        nChanges = 0
        
        filled_cells   = [c for c in self.m_cells if c.m_value != 0]
        unfilled_cells = [c for c in self.m_cells if c.m_value == 0]
        
        # a set of all the values that are already placed in this group
        group_values = set([cell.m_value for cell in filled_cells])
        
        for val in group_values:
            for cell in unfilled_cells:
                nChanges += cell.ClearFlag(val)
                                
        return nChanges  

    def FindStackedCells(self):
        """SudokuGroup::FindStackedCells: when we find N cells in a group that only have N possibilities in common, 
           we can clear those possibilities in the entire group"""

        # for each group:
            # reduce to just unfilled cells
            # ^^ if more than 3, then for each combination of 3 cells
                # calculate total possible within this set of 3
                # if total possible < 3 then invalid
                # if total possible > 3 then nothing to do (expected case)
                # if total possible == 3 then this is a "stacked group" and we can clear these 3 possibilities in all other cells in the group
        nChanges = 0

        # reduce to just unfilled cells
        unfilled_cells = [c for c in self.m_cells if c.m_value == 0]
        
        # start with just pairs and triplets -- note I haven't found a puzzle that more than triplets helps to solve
        for N in range(2,4):  

            # can't have stacked triplet if we don't have at least 3 unfilled cells
            if len(unfilled_cells) <= N:
                return nChanges

            # iterate through 
            for s in itertools.combinations(unfilled_cells, N):
                # s is now a set of N cells
                # [c.m_possibleValues for c in s] # => e.g. [[1, 3, 5, 9], [3, 4, 6], [3, 4]]
                all_possible = set([item for sublist in [c.m_possibleValues for c in s] for item in sublist]) # => e.g. [[1, 3, 5, 9], [3, 4, 6], [3, 4]]
                if len(all_possible) == N:
                    if (g_verbosity >= VERBOSE_WORDY):
                        print(f"Found a stacked set of {N} cells in {self.m_type} {self.m_index+1}: {all_possible}")
                    for cell in unfilled_cells:
                        if not cell in s:
                            for value in all_possible:
                                nChanges += cell.ClearFlag(value)
        return nChanges    

    def UpdateValues(self):
        nChanges = 0

        # lone fills, for each cell, see if there's only one possible number to fill
        for cell in self.m_cells:
            nChanges += cell.UpdateValue()
            
        # group fills, for the numbers 1-9 see if there's only one space in this group
        for nNumberToLookFor in range(1,10):
            nCellWhereFound = 0
            numInstances = 0

            for i in range(9):
                cell = self.m_cells[i]
                if (cell.CouldBe(nNumberToLookFor)):
                    numInstances += 1
                    nCellWhereFound = i

            # if this number was only found once, fill it:
            if (1 == numInstances):
                if (self.m_cells[nCellWhereFound].SetValue(nNumberToLookFor)):
                    nChanges += 1
                    if (g_verbosity >= VERBOSE_WORDY):
                        print(f"Placed {nNumberToLookFor} in row {self.m_cells[nCellWhereFound].m_location[0] + 1} column {self.m_cells[nCellWhereFound].m_location[1] + 1} because it is the only place a {nNumberToLookFor} can go in {self.m_type} {self.m_index+1}")

        return nChanges

class SudokuBoard:
    """Defines board for the game Sudoku"""
    def __init__(self, desc: str):
        self.m_szBoardDescription = desc

        # initialize m_cells with some of that amazing array math
        self.m_cells = [[SudokuCell(i, j) for i in range(9)] for j in range(9)]

        # Creation of the arrays of indices that organize the 9 sets of rows, cols and boxes
        row_indices =  [[(x,y) for x in range(9)] for y in range(9)]
        col_indices =  [[(x,y) for y in range(9)] for x in range(9)]
        box_indices =  [[(x%3*3+y%3,x//3*3+y//3) for y in range(9)] for x in range(9)]
        
        # now we create our 3 groupings of nine cells each:
        self.m_rows  = [SudokuGroup(self.m_cells, row_indices[i], "row",    i) for i in range(9)]
        self.m_cols  = [SudokuGroup(self.m_cells, col_indices[i], "column", i) for i in range(9)]
        self.m_boxes = [SudokuGroup(self.m_cells, box_indices[i], "box",    i) for i in range(9)]

    def SetState(self, parent):
        for r in range(9):
            for c in range(9):
                self.m_cells[r][c].SetValue(parent.m_cells[r][c].m_value)
        self.ClearFlags()

    def ReadInFile(self, filename="puzzle.txt"):
        """open the file, read the lines"""
        if not os.path.isfile(filename):
            print("ERROR: No input file named " + filename)
            sys.exit(1)
        f = open(filename, 'r')
        for x in range(9):
            ln = f.readline()
            ln = ln.rstrip()
            if not ln:
                print("ERROR: badly formed input file: " + filename)
                sys.exit(1)
            if (g_verbosity >= VERBOSE_EXTREME):
                print(f"File line {x}: {ln}... setting row values")
            self.m_rows[x].SetValues(ln)
        f.close()

    def PrintWithFlags(self):
        """prints large version of the board, 3x3 grid with all flags"""
        print(f"\nBoard {self.m_szBoardDescription}:")
        print("|-----------------COLUMNS-----------------|")
        print("|||-1-|-2-|-3-||-4-|-5-|-6-||-7-|-8-|-9-|||")
        for row in range(9):
            for subrow in range(3):
                self.m_rows[row].PrintSubrow(subrow)
                if subrow == 2:
                    # end of the row, print appropriate divider (--- if minor, === if major)
                    if (row % 3 == 2):
                        print("|||===|===|===||===|===|===||===|===|===|||")
                    else:
                        print("|||---|---|---||---|---|---||---|---|---|||")    

    def PrintCompact(self):
        print("|---|---|---|")
        for row in range(9):
            print(self.m_rows[row])
            if ((row % 3) == 2):
                print("|---|---|---|")
        return

    def SetValue(self, loc, val):
        self.m_cells[loc[1]][loc[0]].SetValue(val)

    def Solve(self) -> int:
        """does all of the deterministic fills, loops until we're solved or stuck"""
        nChanges = self.ClearFlags()
        while (nChanges > 0):
            nChanges = self.SolverLoop()
            
        # check state (solved, invalid, or stuck) and act accordingly:
        if (self.CurrentState() == STATE_SOLVED):
            print(f"\nBoard {self.m_szBoardDescription}:  SOLVED")
            self.PrintCompact()
            return True
        elif (self.CurrentState() == STATE_INVALID):
            print(f"Board {self.m_szBoardDescription}: INVALID")
            return False
        else:
            print("No deterministic solution found.  Branching:")
            sub_board_index = 0
            for r in self.m_cells:
                for c in r:
                    if c.m_value == 0:
                        for v in c.m_possibleValues:
                            print(f"\nBranching with value {v} in row {c.m_location[0]+1} column {c.m_location[1]+1}")
                            sub_board_index += 1
                            sub_board = SudokuBoard(f"{self.m_szBoardDescription}.{sub_board_index}")
                            # copy the current board status and set sub_board's cell c to this value:
                            sub_board.SetState(self)
                            sub_board.SetValue(c.m_location, v)
                            if sub_board.Solve():
                                return True
        return False

    def UpdateValues(self) -> int:
        """goes through all rows, columns and boxes to Update Values"""
        nChanges = 0
        for row in self.m_rows:
            nChanges = nChanges + row.UpdateValues()

        for col in self.m_cols:
            nChanges = nChanges + col.UpdateValues()
    
        for box in self.m_boxes:
            nChanges = nChanges + box.UpdateValues()
            
        return nChanges

    def SolverLoop(self) -> int:
        """ Magic time -- we've been asked to iterate through our deterministic logic rules"""
        nChanges = 0
        if (STATE_INCOMPLETE == self.CurrentState()):
            # first go through and clear any flags
            nChanges += self.ClearFlags()

            # then any stacked cells we find
            nChanges += self.FindStackedCells()
            
            # now go through and Update Values
            nChanges += self.UpdateValues()

        return nChanges

    def CurrentState(self):
        bComplete = True
        for group in range(9):
            nState = self.m_rows[group].CurrentState()
            if (STATE_INVALID == nState):
                # no need to look further
                return STATE_INVALID

            if (STATE_INCOMPLETE == nState):
                bComplete = False

            nState = self.m_cols[group].CurrentState()
            if (STATE_INVALID == nState):
                # no need to look further
                return STATE_INVALID

            if (STATE_INCOMPLETE == nState):
                bComplete = False

            nState = self.m_boxes[group].CurrentState()
            if (STATE_INVALID == nState):
                # no need to look further
                return STATE_INVALID

            if (STATE_INCOMPLETE == nState):
                bComplete = False

        if bComplete:
            return STATE_SOLVED

        return STATE_INCOMPLETE

    def ClearFlags(self):
        # standard clearing: looks for numbers in row, col, group)
        bChanged = False

        for row in range(9):
            if(self.m_rows[row].ClearFlags()):
                bChanged = True

        for col in range(9):
            if (self.m_cols[col].ClearFlags()):
                bChanged = True

        for box in range(9):
            if (self.m_boxes[box].ClearFlags()):
                bChanged = True

        return bChanged
        
    def FindStackedCells(self):
        """SudokuBoard::FindStackedCells: find sets of N cells within the same grouping that only have N possibilities"""
        nChanges = 0

        for row in range(9):
            nChanges += self.m_rows[row].FindStackedCells()

        for col in range(9):
            nChanges += self.m_cols[col].FindStackedCells()
            
        for box in range(9):
            nChanges += self.m_boxes[box].FindStackedCells()
            
        return nChanges