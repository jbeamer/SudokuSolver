// SudokuSolver.h
// Written by Jonathan Beamer
// Copyright (c) 2005-2008

extern int g_nSolutionsFound;
extern int g_nBranchEndpoitsFound;

// CCell class
// -----------
// - a Cell is a single space that houses a number on the grid
// - it knows what it is, or the numbers it could be
class CCell {
public:
	CCell();

	void
	Init(int boardsize=9);

	bool
	SetValue(int newvalue);

	bool
	ClearCouldBeFlag(int value);

	bool
	UpdateValue();

	int
	NumPossible();

	int
	GetNthPossibility(int n);

	int  value;
	bool couldbe[10];
	int  m_nBoardSize;
};

// CCellGroup class
// ----------------
// - a CellGroup is a group of 9 cells that may be a row, column or nine-box
// - it does not house the cells, is simply hold the coordinates of its nine
//   cells, which are stored elsewhere
class CGroup {
public:
	void
	Init (CCell* cellarray, int* cellIndexes, int type, int index /* 1 based */, int nBoardSize=9);

	void
	SetValues(char *szValues);

	bool
	ClearCouldBeFlags();

	bool
	UpdateValues();

	void
	Print();

	void
	Print(char *buffer, int buffer_size);

	void
	PrintCouldBeFlags();

	int
	CurrentState();

	// CCellGroup::BoxHasActionablePairOrTriad
	// returns the third that it occurs in (1, 2, 3) or 0 for no pair
	int
	BoxHasActionablePairOrTriad(int nNumber);

	// CCellGroup::ProcessPairInRowOrCol
	// returns if a change was made
	bool
	ProcessPairInRowOrCol(int pairval, int nRowOrColIndex /* 0, 1, 2 */, bool bRow);

	bool
	DoStackedPairs();

	int GetValue(int groupindex);

	bool
	ClearNumberInRow(int num, int row);

	bool
	ClearNumberInColumn(int num, int row);

protected:
	int		m_CellIndexes[9];
	CCell*	m_CellArray;
	int		m_nType;
	char*	m_szType;
	int		m_nIndex;
	int		m_nBoardSize;
};

class CSudokuBoard {
public:
	// CSudokuBoard::CSudokuBoard
	CSudokuBoard(const char *desc, int nBoardSize=9);
	~CSudokuBoard();

	void
	operator= (CSudokuBoard* source);

	bool	ReadInFile(const char* filename=NULL);
	bool	ClearCouldBeFlags();
	bool	UpdateValues();
	void	DoFills();
	int		Solve();
	int		CurrentState();
	int		FindCellToBranch();
	void	Print();
	void	PrintCouldBeFlags();
	bool	IterateOnce();
	void    PrintDivider();
	void    PrintDivider(int which);
private:
	CCell	m_cells[100];
	CGroup	m_rows[9];
	CGroup	m_cols[9];
	CGroup	m_boxes[9];
	char*	m_szBoardDescription;
	int		m_nIterations;
	int		m_nBoardSize;  // usually 9, but could be a 6x6
};

