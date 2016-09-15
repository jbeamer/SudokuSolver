// SudokuSolver.cpp
// Written by Jonathan Beamer
// Copyright (c) 2005-2008

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "SudokuBoard.h"

#define STATE_INCOMPLETE	1
#define STATE_INVALID		2
#define STATE_SOLVED		3

int g_nSolutionsFound=0;
int g_nBranchEndpoitsFound=0;


#define GROUPTYPE_ROW	0
#define GROUPTYPE_COL	1
#define GROUPTYPE_BOX	2
static const char* g_szGroupTypes[3] = { "row", "column", "box" };

#define VERBOSE_CRITICAL	0
#define VERBOSE_NORMAL		1
#define VERBOSE_WORDY		2
#define VERBOSE_EXTREME		3
static const int g_verbosity = VERBOSE_WORDY;

CCell::CCell()
{
	value=0;
}

void
CCell::Init(int boardsize /*=9*/)
{
	m_nBoardSize = boardsize;

	// first set all our flags to false
	for (int i=0; i<10; i++)
		couldbe[i] = false;

	// then to true for the ones within the size of the board
	for (int i=0; i<(m_nBoardSize+1); i++)
		couldbe[i] = true;
}

// CCell::SetValue
// takes a new value
// returns whether that is a change
bool
CCell::SetValue(int newvalue)
{
	if (value == newvalue)
		return false;

	value = newvalue;
	for (int i=0; i<(m_nBoardSize+1); i++)
		couldbe[i] = false;
	couldbe[value] = true;

	return true;
}

// CCell::ClearCouldBeFlag
// clears the flag
// returns if this is new info
bool
CCell::ClearCouldBeFlag(int value)
{
	bool bChanged=false;

	if (couldbe[value])
	{
		couldbe[value]=false;
		bChanged = true;
	}

	return bChanged;
}

// CCell::UpdateValue
// at the cell level, UpdateValue justs checks to see if it can only be one possible
// value, and updates itself.
// returns if a change is made
bool
CCell::UpdateValue()
{
	int nValue;
	int nPossible=0;

	if (value == 0)
	{
		// go through the numbers and see how many we could be
		for (int i=1; i<(m_nBoardSize+1); i++)
		{
			if (couldbe[i])
			{
				nValue = i;
				nPossible++;
			}
		}

		// if we only have one possibility, set us to the possibility
		if (nPossible == 1)
		{
			value = nValue;
			return true;
		}
	}

	return false;
}

// CCell::NumPossible
int
CCell::NumPossible()
{
	int nPossible = 0;

	for (int i=1; i<(m_nBoardSize+1);i++)
	{
		if (couldbe[i])
			nPossible++;
	}
	return nPossible;
}

// CCell::GetNthPossibility
int
CCell::GetNthPossibility(int n)
{
	for (int i=1; i<(m_nBoardSize+1);i++)
	{
		if (couldbe[i])
		{
			if ( n-- == 0)
				return i;
		}
	}
	return 0;
}


// CCellGroup class
// ----------------
// - a CellGroup is a group of 9 cells that may be a row, column or nine-box
// - it does not house the cells, is simply hold the coordinates of its nine
//   cells, which are stored elsewhere
void
CGroup::Init (CCell* cellarray, int* cellIndexes, int type, int index /* 1 based */, int nBoardSize)
{
	m_nBoardSize = nBoardSize;
	m_CellArray  = cellarray;

	for (int i=0; i<m_nBoardSize; i++)
		m_CellIndexes[i] = cellIndexes[i];

	m_nType  = type;
	m_szType = (char *)(g_szGroupTypes[m_nType]);
	m_nIndex = index;


}

void
CGroup::SetValues(char *szValues)
{
	for (int i=0;i<m_nBoardSize;i++)
		m_CellArray[m_CellIndexes[i]].SetValue(szValues[i]-'0');
}

bool
CGroup::ClearCouldBeFlags()
{
	// doing this by group
	int i, nExistsInGroup[10];
	bool bChanged = false;

	for (i=0;i<m_nBoardSize+1;i++)
		nExistsInGroup[i]=false;

	// go thru our cells, and collect all our values
	for (int cell=0;cell<m_nBoardSize;cell++)
	{
		if (m_CellArray[m_CellIndexes[cell]].value)
			nExistsInGroup[m_CellArray[m_CellIndexes[cell]].value] = true;
	}

	// now go back through the numbers and clear flags in all cells for numbers we found:
	for (i=1;i<m_nBoardSize+1;i++)
	{
		if (nExistsInGroup[i])
		{
			for (int cell=0;cell<m_nBoardSize;cell++)
			{
				if (m_CellArray[m_CellIndexes[cell]].ClearCouldBeFlag(i))
				{
					if (g_verbosity >= VERBOSE_EXTREME)
						printf("Cell R%dC%d can not be %d by simple elimination\n", i, m_CellIndexes[cell]/10, m_CellIndexes[cell] % 10);
					bChanged = true;
				}
			}
		}
	}
	return bChanged;
}

int
CGroup::GetValue(int groupindex)
{
	return m_CellArray[m_CellIndexes[groupindex]].value;
}

bool
CGroup::UpdateValues()
{
	bool bChanged = false;
	int cell;

	// lone fills, for each cell, see if there's only one possible number to fill
	for (cell=0;cell<m_nBoardSize;cell++)
	{
		if (m_CellArray[m_CellIndexes[cell]].UpdateValue())
		{
			if (g_verbosity >= VERBOSE_WORDY)
				printf ("Placed %d in cell R%dC%d because that was its only possibility\n", m_CellArray[m_CellIndexes[cell]].value, m_CellIndexes[cell]/10, m_CellIndexes[cell]%10);
			bChanged = true;
			return true;
		}
	}

	// group fills, for the numbers 1-9 see if there's only one space in this group
	for (int nNumberToLookFor=1; nNumberToLookFor<(1+m_nBoardSize); nNumberToLookFor++)
	{
		int nCellWhereFound =0;
		int numInstances = 0;

		for (cell=0;cell<m_nBoardSize;cell++)
		{
			if (m_CellArray[m_CellIndexes[cell]].couldbe[nNumberToLookFor])
			{
				numInstances++;
				nCellWhereFound = cell;
			}
		}

		// if this number was only found once, fill it:
		if (1 == numInstances)
		{
			if (m_CellArray[m_CellIndexes[nCellWhereFound]].SetValue(nNumberToLookFor))
			{
				bChanged = true;
				if (g_verbosity >= VERBOSE_WORDY)
					printf ("Placed %d in cell R%dC%d because it only existed once in %s %d\n",
						nNumberToLookFor,
						m_CellIndexes[nCellWhereFound]/10,
						m_CellIndexes[nCellWhereFound]%10,
						m_szType,
						m_nIndex);
			}
		}
	}

	return bChanged;
}


void
CGroup::Print()
{
	if (m_nBoardSize == 9)
	{
		for (int i=0;i<3;i++)
			printf("|%c%c%c",
			(m_CellArray[m_CellIndexes[3*i+0]].value ? m_CellArray[m_CellIndexes[3*i+0]].value + '0' : '.'),
			(m_CellArray[m_CellIndexes[3*i+1]].value ? m_CellArray[m_CellIndexes[3*i+1]].value + '0' : '.'),
			(m_CellArray[m_CellIndexes[3*i+2]].value ? m_CellArray[m_CellIndexes[3*i+2]].value + '0' : '.'));
	}
	else if (m_nBoardSize == 6)
	{
		for (int i=0;i<3;i++)
			printf("|%c%c",
			(m_CellArray[m_CellIndexes[2*i+0]].value ? m_CellArray[m_CellIndexes[2*i+0]].value + '0' : '.'),
			(m_CellArray[m_CellIndexes[2*i+1]].value ? m_CellArray[m_CellIndexes[2*i+1]].value + '0' : '.'));
	}
	else
	{
		assert (false);
	}

	printf("|\n");
}

void
CGroup::Print(char *buffer, int buffer_size)
{
	if (m_nBoardSize == 9)
	{
		for (int i=0;i<3;i++)
		{
			sprintf(buffer, "%s|%c%c%c",
				buffer,
				(m_CellArray[m_CellIndexes[3*i+0]].value ? m_CellArray[m_CellIndexes[3*i+0]].value + '0' : '.'),
				(m_CellArray[m_CellIndexes[3*i+1]].value ? m_CellArray[m_CellIndexes[3*i+1]].value + '0' : '.'),
				(m_CellArray[m_CellIndexes[3*i+2]].value ? m_CellArray[m_CellIndexes[3*i+2]].value + '0' : '.'));
		}
	}
	else if (m_nBoardSize == 6)
	{
		for (int i=0;i<3;i++)
			sprintf(buffer, "%s|%c%c",
				buffer,
				(m_CellArray[m_CellIndexes[2*i+0]].value ? m_CellArray[m_CellIndexes[2*i+0]].value + '0' : '.'),
				(m_CellArray[m_CellIndexes[2*i+1]].value ? m_CellArray[m_CellIndexes[2*i+1]].value + '0' : '.'));
	}
	else
	{
		assert (false);
	}

	sprintf(buffer, "%s|\n", buffer);
}


void
CGroup::PrintCouldBeFlags()
{
	for (int row=0;row<3;row++)
	{
		printf("|");
		for (int cell=0; cell<m_nBoardSize; cell++)
		{
			char chMiddle;

			chMiddle = (m_CellArray[m_CellIndexes[cell]].couldbe[row*3+2] ? (row*3+'2') : ' ');
			if ((1 == row) && (m_CellArray[m_CellIndexes[cell]].value))
				chMiddle = '0' + m_CellArray[m_CellIndexes[cell]].value;

			printf("%s%c%c%c",
				( (m_nBoardSize==9 && cell%3) || (m_nBoardSize==6 && cell%2) ? "|" : "||"),
				(m_CellArray[m_CellIndexes[cell]].couldbe[row*3+1] ? (row*3+'1') : ' '),
				chMiddle,
				(m_CellArray[m_CellIndexes[cell]].couldbe[row*3+3] ? (row*3+'3') : ' '));
		}
		printf("|||\n");
	}
}

int
CGroup::CurrentState()
{
	bool bComplete = true;
	int nNumbersFound[10];

	memset(nNumbersFound, 0, 10*sizeof(int));

	for (int cell=0;cell<m_nBoardSize;cell++)
	{
		if (0 == m_CellArray[m_CellIndexes[cell]].value)
		{
			// puzzle is not yet finished
			bComplete = false;

			// check to see if it ever can be filled:
			if (0 == m_CellArray[m_CellIndexes[cell]].NumPossible() )
				return STATE_INVALID;
		}
		else
		{
			nNumbersFound[m_CellArray[m_CellIndexes[cell]].value]++;
		}
	}

	// check to see if there's more than one of any number (this can happen when
	// a number is filled in two locations in a single DoFills iteration)
	for (int i=1; i<10; i++)
		if (nNumbersFound[i]>1)
			return STATE_INVALID;

	return (bComplete ? STATE_SOLVED : STATE_INCOMPLETE);
}

// CCellGroup::BoxHasActionablePairOrTriad
// ---------------------------------------
// For Box cell groups only.
//
// In a box, if a number shows up exclusively in a single row or column,
// then we can clear couldbe flags in the rest of the row or column in
// adjacent boxes.
//
// This function takes a single number to check for and sees if this is
// a box for which we can apply such logic
//
// Returns nonzero index if a group is found:
//   1, 2, 3 -- the row 1, 2, 3 that group is in
//   4, 5, 6 -- the col 1, 2, 3 that group is in
int
CGroup::BoxHasActionablePairOrTriad(int nNumber)
{
	int nTimesSeen = 0;
	int placesFound[3];

	// set observations to 0:
	for (int i=0; i<3; i++) placesFound[i] = 0;

	// first check: does this number show up as a possibility exactly 2 or 3 times?
	for (int cell=0; cell<9; cell++)
	{
		if (m_CellArray[m_CellIndexes[cell]].couldbe[nNumber])
		{
			// we are done if we have already seen it 3 times, this is the fourth:
			if (nTimesSeen >= 3) return 0;

			// save where we saw it
			placesFound[nTimesSeen] = cell;

			// keep track of how many times we've seen it
			nTimesSeen++;
		}
	}
	if (nTimesSeen < 2)
	{
		// no, this is not actionable
		return 0;
	}

	int row1 = 1 + placesFound[0]/3; // (1, 2, 3)
	int row2 = 1 + placesFound[1]/3; // (1, 2, 3)
	int row3 = 1 + placesFound[2]/3; // (1, 2, 3)
	int col1 = 1 + placesFound[0]%3; // (1, 2, 3)
	int col2 = 1 + placesFound[1]%3; // (1, 2, 3)
	int col3 = 1 + placesFound[2]%3; // (1, 2, 3)

	// second check: are these instances in the same row or column?
	if (nTimesSeen == 2)
	{
		// possibility 1: 2 in same row
		if (row1 == row2) return row1;

		// possibility 2: 2 in same col
		if (col1 == col2) return (col1 + 3);
	}
	if (nTimesSeen == 3)
	{
		// possibility 1: 2 in same row
		if ((row1 == row2) && (row1 == row3)) return row1;

		// possibility 2: 2 in same col
		if ((col1 == col2) && (col1 == col3)) return (col1 + 3);
	}

	return 0;
}

// CCellGroup::ProcessPairInRowOrCol
// returns if a change was made
bool
CGroup::ProcessPairInRowOrCol(int pairval, int nRowOrColIndex /* 0, 1, 2 */, bool bRow)
{
	bool bChanged = false;
	for (int cell=0; cell<m_nBoardSize; cell++)
	{
		if ( ( bRow && ((nRowOrColIndex%3) != (cell/3))) ||
			 (!bRow && ((nRowOrColIndex%3) != (cell%3))) )
		{
			// if these cells say they could be pairval, clear that flag
			if (m_CellArray[m_CellIndexes[cell]].couldbe[pairval])
			{
				if (m_CellArray[m_CellIndexes[cell]].ClearCouldBeFlag(pairval))
				{
					if (g_verbosity >= VERBOSE_WORDY)
						printf("Found a set of %ds in %s %d that caused me to clear flag %d in cell R%dC%d\n",
							pairval,
							(bRow ? "row" : "column"),
							nRowOrColIndex + 1,
                            pairval,
							m_CellIndexes[cell] / 10,
							m_CellIndexes[cell] % 10);
					bChanged = true;
				}
			}
		}
	}
	return bChanged;
}

// TODO: This works for rows and cols, but not for columns within boxes
bool
CGroup::DoStackedPairs()
{
	bool bChanged = false;

	// TODO: logic here has to change a lot for different board sizes
	if (9 != m_nBoardSize)
		return bChanged;

	for (int nThird=0; nThird<3; nThird++)
	{
		int nEmpty = 0;		// count unfilled cells in this third
		int couldbe[10];	// number of times each of the numbers can be in the triad
		int totalNumbersAvailableToFillEmpties = 0;   // total numbers (1-m_nBoardSize) that are available

		memset((void *)(couldbe), 0, 10*sizeof(int));
		for (int cell=nThird*3; cell<(nThird+1)*3; cell++) // 0-2, 3-5, 6-8
		{
			if (m_CellArray[m_CellIndexes[cell]].value == 0)
			{
				nEmpty++;
				for (int i=1; i<10; i++)
				{
					if (m_CellArray[m_CellIndexes[cell]].couldbe[i])
					{
						if (0 == couldbe[i])
							totalNumbersAvailableToFillEmpties++;
						couldbe[i]++;
					}
				}
			}
		}

		if ((nEmpty > 1) && (totalNumbersAvailableToFillEmpties == nEmpty))
		{
			// this third is locked down, these numbers are unavailable for all other cells in the group
			for (int i=1; i<10; i++)
			{
				if (couldbe[i])
				{
					for (int cell2=0; cell2<m_nBoardSize; cell2++)
					{
						if ( (cell2<nThird*3) ||  (cell2>=(nThird+1)*3))
						{
                            if (m_CellArray[m_CellIndexes[cell2]].ClearCouldBeFlag(i))
							{
								bChanged = true;
								if (g_verbosity >= VERBOSE_WORDY)
									printf ("Found a stacked %s in %s %d that caused me to clear flag %d in cell R%dC%d\n",
										(nEmpty==2 ? "pair" : "triad"),
										m_szType,
										m_nIndex,
										i,
										m_CellIndexes[cell2]/10,
										m_CellIndexes[cell2]%10);
							}
						}
					}
				}
			}
		}
	}
	return bChanged;
}


bool
CGroup::ClearNumberInRow(int num, int row)
{
	bool retVal = false;

	for (int cell=row*3; cell<(row+1)*3; cell++)
	{
		if (m_CellArray[m_CellIndexes[cell]].ClearCouldBeFlag(num))
		{
			retVal = true;
		}
	}
	return retVal;
}

bool
CGroup::ClearNumberInColumn(int num, int col)
{
	bool retVal = false;
	int cell;

	for (int i=0; i<3; i++)
	{
		cell = col + 3*i;
		if (m_CellArray[m_CellIndexes[cell]].ClearCouldBeFlag(num))
		{
			retVal = true;
		}
	}
	return retVal;
}

CSudokuBoard::CSudokuBoard(const char *desc, int nBoardSize)
{
	int i, j;
	int nRow_i_Cell_Indexes[9];
	int nCol_i_Cell_Indexes[9];
	int nBox_i_Cell_Indexes[9];

	m_nBoardSize = nBoardSize;
	for (i=0;i<100;i++)
		m_cells[i].Init(m_nBoardSize);

	// initialize row, col and box references to the cells
	for (i=0; i<m_nBoardSize; i++)
	{
		for (j=0; j<m_nBoardSize; j++)
		{
			nRow_i_Cell_Indexes[j] = (i+1)*10 + j+1;
			nCol_i_Cell_Indexes[j] = (j+1)*10 + i+1;
			if (m_nBoardSize == 9)
				nBox_i_Cell_Indexes[j] = 10*((j/3)+(3*(i/3))+1) + ((j%3)+(3*(i%3))+1);
			else if (m_nBoardSize == 6)
				nBox_i_Cell_Indexes[j] = 30*(i/3)+10*(1+(j/2))+1+(j%2)+2*(i%3);
			else
				assert(false); // we don't handle this yet
		}
		m_rows[i].Init( m_cells, nRow_i_Cell_Indexes, GROUPTYPE_ROW, i+1, nBoardSize);
		m_cols[i].Init( m_cells, nCol_i_Cell_Indexes, GROUPTYPE_COL, i+1, nBoardSize);
		m_boxes[i].Init(m_cells, nBox_i_Cell_Indexes, GROUPTYPE_BOX, i+1, nBoardSize);
	}
	m_szBoardDescription = (char *)(malloc(strlen(desc)+1));
	strcpy(m_szBoardDescription, desc);

	m_nIterations = 0;
}

// CSudokuBoard::~CSudokuBoard
CSudokuBoard::~CSudokuBoard()
{
	free(m_szBoardDescription);
}

// CSudokuBoard::operator=
void
CSudokuBoard::operator= (CSudokuBoard* source)
{
	// only m_cells need to be copied, as groups are initialized to point within these
	memcpy(&m_cells, source->m_cells, 100*sizeof(CCell));
}

// CSudokuBoard::ReadInFile
bool
CSudokuBoard::ReadInFile(const char* fn)
{
	FILE *stream;
	char  filename[512];
	char  buffer[200];
	int numread, bufferloc;
	char* line = buffer;

	// create a default and convert to wide chars:
	strcpy(filename, (fn ? fn : "puzzle.txt"));

	// open the file
	// TODO: generalize
//	if ( 0 != fopen_s(&stream, filename, "r") )
	stream = fopen(filename, "r");
	if ( stream == NULL )
	{
		printf("\nERROR: Could not open %s (%s)\n", filename, strerror(errno));
		return false;
	}

	// read in the first 200 bytes of the file
	numread = (int) fread(buffer, sizeof(char), 200, stream);
	if ( numread>=(m_nBoardSize * (m_nBoardSize+1)) )
	{
		bufferloc = 0;

		// get a line at a time and have the row object deal with it:
		for(int i=0;i<m_nBoardSize;i++)
		{
			while ((bufferloc < numread) && (buffer[bufferloc] != '\n'))
				bufferloc++;
			buffer[bufferloc++] = '\0';
			m_rows[i].SetValues(line);
			line = buffer + bufferloc;
		}
	}

	fclose(stream);
	return true;
}

// CSudokuBoard::ClearCouldBeFlags
bool
CSudokuBoard::ClearCouldBeFlags()
{
	// standard clearing: looks for numbers in row, col, group)
	bool bChanged = false;
	int group;

	for (group=0; group<m_nBoardSize; group++)
	{
		if(m_rows[group].ClearCouldBeFlags())
			bChanged = true;

		if (m_cols[group].ClearCouldBeFlags())
			bChanged = true;

		if (m_boxes[group].ClearCouldBeFlags())
			bChanged = true;
	}

	// Print out could be flags
	if (g_verbosity >= VERBOSE_WORDY)
		printf("\nBoard %s Iteration %d: Current status of board:\n",
			m_szBoardDescription,
			m_nIterations);
	PrintCouldBeFlags();

/*
	for each box,
		for each number:
			are there only 2 or 3 possibilities that are in a single row, column?
				if so, clear any others of that number in the entire row / column
*/
	// pairwise elimination: looks for pairs in rows, cols that give info about the box
	int location;
	for (int box=0; box<9; box++)
	{
		for (int num=1; num<10; num++)
		{
			location = m_boxes[box].BoxHasActionablePairOrTriad(num);
			if (location)
			{
				bool isColumn = false;
				int otherBox1, otherBox2;

				// TODO: clear the appropriate flags
				if (location>3)
				{
					isColumn = true;
					location -= 3;
				}

				if (isColumn)
				{
					// other boxes in same columns
					otherBox1 = (box+3)%9;
					otherBox2 = (box+6)%9;
					// TODO: print the success here, if these change anything
					m_boxes[otherBox1].ClearNumberInColumn(num, location-1);
					m_boxes[otherBox2].ClearNumberInColumn(num, location-1);
				}
				else
				{	// other boxes in same row
					otherBox1 = (box/3)*3 +((box+1)%3);
					otherBox2 = (box/3)*3 +((box+2)%3);
					// TODO: print the success here, if these change anything
					m_boxes[otherBox1].ClearNumberInRow(num, location-1);
					m_boxes[otherBox2].ClearNumberInRow(num, location-1);
				}
/*
				if (g_verbosity >= VERBOSE_WORDY)
					printf("Found an actionable pair/triad in box %d: %ds in %s %d => clear boxes %d & %d\n",
							box+1,
							num,
							(isColumn ? "column" : "row"),
							location,
							otherBox1 + 1,
							otherBox2 + 1);
*/

			}
		}
	}


	// look for "stacked pairs" in row or column
	for (group=0; group<m_nBoardSize; group++)
	{
		if (m_rows[group].DoStackedPairs())
			bChanged = true;

		if (m_cols[group].DoStackedPairs())
			bChanged = true;

		if (m_boxes[group].DoStackedPairs())
			bChanged = true;
	}

	return bChanged;
}

// CSudokuBoard::UpdateValues
bool
CSudokuBoard::UpdateValues()
{
	bool bChanged = false;
	for (int group=0; group<m_nBoardSize; group++)
	{
		if (m_rows[group].UpdateValues())
			bChanged = true;

		if (m_cols[group].UpdateValues())
			bChanged = true;

		if (m_boxes[group].UpdateValues())
			bChanged = true;
	}
	return bChanged;
}


bool
CSudokuBoard::IterateOnce()
{
	bool bChanged = false;

	if (STATE_INCOMPLETE == CurrentState())
	{
		m_nIterations++;

		// first go through and clear any flags
		bChanged = ClearCouldBeFlags();

		// now go through and Update Values
		if (UpdateValues())
			bChanged = true;
	}

	return bChanged;
}

// CSudokuBoard::Solve
int
CSudokuBoard::Solve()
{
	char desc[1024];

	// do all of the deterministic fills
	bool bChanged = true;

	// loop until we're stuck
	while (bChanged)
	{
		bChanged = IterateOnce();
	}

	// check state (solved, invalid, or stuck) and act accordingly:
	if ( CurrentState() == STATE_SOLVED )
	{
		g_nSolutionsFound++;
		g_nBranchEndpoitsFound++;
		printf("Board %s:  SOLVED after %d iterations\n", m_szBoardDescription, m_nIterations);
		Print();
	}
	else if ( CurrentState() == STATE_INVALID )
	{
		g_nBranchEndpoitsFound++;
		printf("Board %s: INVALID after %d iterations\n", m_szBoardDescription, m_nIterations);
	}
	else
	{
		// if not solved, find an uncertain cell and solve a sub grid (branching)
		int nCellToBranch = FindCellToBranch();
		printf("Board %s: Branching into %d:\n", m_szBoardDescription, m_cells[nCellToBranch].NumPossible());
		Print();
		for (int i=0; i<m_cells[nCellToBranch].NumPossible(); i++)
		{
			sprintf(desc, "%s.%d", m_szBoardDescription, i+1);
			CSudokuBoard childBoard(desc, m_nBoardSize);
			childBoard = this;
			childBoard.m_cells[nCellToBranch].SetValue(m_cells[nCellToBranch].GetNthPossibility(i));
			printf("Board %s: Started by seeding %d in cell R%dC%d\n", desc, childBoard.m_cells[nCellToBranch].value, nCellToBranch/10, nCellToBranch%10);
			m_nIterations += childBoard.Solve();

		}
	}
	return m_nIterations;
}

// CSudokuBoard::CurrentState
// TODO: check for redundant letters in a row, col or box
int
CSudokuBoard::CurrentState()
{
	int nState;
	bool bComplete = true;

	for (int group=0;group<m_nBoardSize;group++)
	{
		nState = m_rows[group].CurrentState();
		if (STATE_INVALID == nState)
			return STATE_INVALID;  // no need to look further
		if (STATE_INCOMPLETE == nState)
			bComplete = false;

		nState = m_cols[group].CurrentState();
		if (STATE_INVALID == nState)
			return STATE_INVALID;  // no need to look further
		if (STATE_INCOMPLETE == nState)
			bComplete = false;

		nState = m_boxes[group].CurrentState();
		if (STATE_INVALID == nState)
			return STATE_INVALID;  // no need to look further
		if (STATE_INCOMPLETE == nState)
			bComplete = false;
	}

	return (bComplete ? STATE_SOLVED : STATE_INCOMPLETE);
}

// CSudokuBoard::FindCellToBranch
// finds the "best" (??) cell to branch
// originally, used the first empty cell, now finds the first with the min number
// of possibilities (probably 2).
int
CSudokuBoard::FindCellToBranch()
{
	int nCell=0;  // the one winning
	int nPossibilities = m_nBoardSize;

	// go through the cells and find one with few possibilities (widest branch)
	for (int row=1; row<m_nBoardSize+1; row++)
		for (int col=1; col<m_nBoardSize+1; col++)
			if (0 == m_cells[row*10 + col].value)
			{
				if (m_cells[row*10 + col].NumPossible() < nPossibilities)
				{
					nCell = row*10 + col;
					nPossibilities = m_cells[row*10 + col].NumPossible();
				}
			}

	return nCell;
}

void
CSudokuBoard::PrintDivider()
{
	if (9 == m_nBoardSize)
	{
		printf("|---|---|---|\n");
	}
	else if (6 == m_nBoardSize)
	{
		printf("|--|--|--|\n");
	}
	else
	{
		assert(false);
	}
}

void
CSudokuBoard::PrintDivider(int which)
{
	if (9 == m_nBoardSize)
	{
		if (which == 2)
			printf("|||===|===|===||===|===|===||===|===|===|||\n");
		else
			printf("|||---|---|---||---|---|---||---|---|---|||\n");
	}
	else if (6 == m_nBoardSize)
	{
		if (which == 2)
			printf("|||===|===||===|===||===|===|||\n");
		else
			printf("|||---|---||---|---||---|---|||\n");
	}
	else
	{
		assert(false);
	}
}

// CSudokuBoard::Print
void
CSudokuBoard::Print()
{
	PrintDivider();
	for (int i=0;i<m_nBoardSize;i++)
	{
		m_rows[i].Print();
		if ((i%3) == 2)
			PrintDivider();
	}
	printf("\n");
}

// CSudokuBoard::PrintCouldBeFlags
void
CSudokuBoard::PrintCouldBeFlags()
{
	if (9 == m_nBoardSize)
	{
		printf("|-----------------COLUMNS-----------------|\n");
		printf("|||-1-|-2-|-3-||-4-|-5-|-6-||-7-|-8-|-9-|||\n");
	}
	else if (6 == m_nBoardSize)
	{
		printf("|-----------COLUMNS-----------|\n");
		printf("|||-1-|-2-||-3-|-4-||-5-|-6-|||\n");
	}
	else
	{
		assert(false);
	}

	for (int i=0;i<m_nBoardSize;i++)
	{
		m_rows[i].PrintCouldBeFlags();
		PrintDivider(i%3);
	}
	printf("\n");
}

