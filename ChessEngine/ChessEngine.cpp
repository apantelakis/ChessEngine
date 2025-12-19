// ChessEngine.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <sstream>
#include <array>
#include <cstdlib>
#include <vector>
#include <iomanip>
#include <cassert>
#include <random>



// Define bitboard
typedef unsigned long long  U64;
// Define Move
typedef uint16_t Move;

struct SearchResult {
	Move move;
	int score;
};

// get/set/pop macros
#define get_bit(bitboard, square) ((bitboard >> square) & 1ULL)
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define pop_bit(bitboard, square) (get_bit(bitboard, square) ? bitboard ^= (1ULL << square) : 0) // turns bit from 1 to 0, only if it's 1

constexpr auto MAX_MOVES = 256;
constexpr auto oneRank = 8;

const auto minScore = -999999;
const auto maxScore = 999999;
const auto checkmateScore = -10000;
const auto depth = 6; // CURRENT DEPTH
const auto MAX_DEPTH = 64;

int plyCounter = 0;

// Enumerate sides / colors
enum Color { white, black };

Color currentSideToMove = white; // This is for UCI


std::array<std::array<Move, MAX_MOVES>, MAX_DEPTH> moveStack; // Max depth 64, one array that stores moves for all depth levels
std::array<int, MAX_DEPTH> moveCountStack; // Number of moves for each depth level


// Forward declarations 
void getPseudoLegalMoves(Color c, std::array<Move, MAX_MOVES>& moveStack, int& moveCount);

void makeMove(Move m, Color c, int ply);
void unmakeMove(Move m, Color c, int ply);

SearchResult negaMax(Color c, int alpha, int beta, int depthLeft, int ply);

enum EFlag {
	quiet_move,
	double_pawn_push,
	king_side_castle,
	queen_side_castle,
	capture,
	en_passant_capture,
	knight_promotion,
	bishop_promotion,
	rook_promotion,
	queen_promotion,
	knight_promo_capture,
	bishop_promo_capture,
	rook_promo_capture,
	queen_promo_capture
};

bool whiteKingSideCastlingRights{ true };
bool whiteQueenSideCastlingRights{ true };

bool blackKingSideCastlingRights{ true };
bool blackQueenSideCastlingRights{ true };

// Enumerate board squares
enum EnumSquare {
	a8, b8, c8, d8, e8, f8, g8, h8,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a1, b1, c1, d1, e1, f1, g1, h1
};


int getFile(int sq)
{
	return sq % 8 + 1;
}

int getRank(int sq)
{
	return 8 - (sq / 8);
}

int stringToSquare(const std::string& str)
{
	if (str.length() != 2) return -1; // Invalid input

	char file = str[0];
	char rank = str[1];

	if (file < 'a' || file > 'h') return -1;
	if (rank < '1' || rank > '8') return -1;

	int fileNum = file - 'a'; // 0-7
	int rankNum = '8' - rank; // 0-7 

	return rankNum * 8 + fileNum; // Convert to 0-63
}

std::string squareToString(int square)
{
	char file = 'a' + (square % 8);
	char rank = '8' - (square / 8);
	return std::string(1, file) + std::string(1, rank);
}

// Enumerate piece type
enum EPieceType {
	ept_pnil = 0, // empty
	ept_wpawn = 1,
	ept_bpawn = 2,
	ept_knight = 3,
	ept_bishop = 4,
	ept_rook = 5,
	ept_queen = 6,
	ept_king = 7,
};

// Enumerate piece code
enum EPieceCode {
	epc_empty = ept_pnil,
	epc_wpawn = ept_wpawn,
	epc_woff = ept_bpawn, // may be used as off the board blocker in mailbox
	epc_wknight = ept_knight,
	epc_wbishop = ept_bishop,
	epc_wrook = ept_rook,
	epc_wqueen = ept_queen,
	epc_wking = ept_king,

	epc_blacky = 8, // color code, may used as off the board blocker in mailbox
	epc_boff = ept_wpawn + epc_blacky, // may be used as off the board blocker in mailbox
	epc_bpawn = ept_bpawn + epc_blacky,
	epc_bknight = ept_knight + epc_blacky,
	epc_bbishop = ept_bishop + epc_blacky,
	epc_brook = ept_rook + epc_blacky,
	epc_bqueen = ept_queen + epc_blacky,
	epc_bking = ept_king + epc_blacky,
};

std::string pieceToUnicode(int piece)
{ // Best solution
	switch (piece)
	{
	case epc_wpawn:   return "♙";
	case epc_wrook:   return "♖";
	case epc_wknight: return "♘";
	case epc_wbishop: return "♗";
	case epc_wqueen:  return "♕";
	case epc_wking:   return "♔";

	case epc_bpawn:   return "♟";
	case epc_brook:   return "♜";
	case epc_bknight: return "♞";
	case epc_bbishop: return "♝";
	case epc_bqueen:  return "♛";
	case epc_bking:   return "♚";

	case epc_empty:   return ".";
	}
	return "?"; // fallback
}

char pieceToChar(int piece)
{ // Temporary
	switch (piece)
	{
	case epc_wpawn:   return 'P';
	case epc_wrook:   return 'R';
	case epc_wknight: return 'N';
	case epc_wbishop: return 'B';
	case epc_wqueen:  return 'Q';
	case epc_wking:   return 'K';
	case epc_bpawn:   return 'p';
	case epc_brook:   return 'r';
	case epc_bknight: return 'n';
	case epc_bbishop: return 'b';
	case epc_bqueen:  return 'q';
	case epc_bking:   return 'k';
	default:          return '.';
	}
}


// Create Board Array / Main Board of type EPieceCode
EPieceCode mainBoard[64];

// Create the 12 bitboards for every piece
std::array<U64, 12> bitboardPieces;

enum Bitboard_index {
	bb_wpawn = 0,
	bb_bpawn,
	bb_wknight,
	bb_bknight,
	bb_wbishop,
	bb_bbishop,
	bb_wrook,
	bb_brook,
	bb_wqueen,
	bb_bqueen,
	bb_wking,
	bb_bking
};

std::array<int, MAX_DEPTH> whichWhitePieceIndexWasThere;
std::array<int, MAX_DEPTH> whichBlackPieceIndexWasThere;

EPieceCode convertPieceIndexToEPC(Color c, int num)
{
	if (c == white)
	{
		if (num == 0) return epc_wpawn;
		if (num == 2) return epc_wknight;
		if (num == 4) return epc_wbishop;
		if (num == 6) return epc_wrook;
		if (num == 8) return epc_wqueen;
		if (num == 10) return epc_wking;
	}
	else if (c == black)
	{
		if (num == 1) return epc_bpawn;
		if (num == 3) return epc_bknight;
		if (num == 5) return epc_bbishop;
		if (num == 7) return epc_brook;
		if (num == 9) return epc_bqueen;
		if (num == 11) return epc_bking;
	}

	return epc_empty;
}

// Occupancy bitboards (white, black and all pieces)
U64 whitePiecesOccupancy;
U64 blackPiecesOccupancy;
U64 allPiecesOccupancy;

// Based on bitboard_index
std::array<int, 12> pieceValue = { 100, -100, 300, -300, 300, -300, 500, -500, 900, -900 };

int getPieceIndex(int sq)
{
	for (int i = bb_wpawn; i <= bb_bking; i++)
	{
		if (get_bit(bitboardPieces[i], sq) == 1) return i;
	}

	return -1;
}


// Initialization of the piece bitboards
void initializeAllBoards()
{
	for (int i = bb_wpawn; i <= bb_bking; i++)
	{
		bitboardPieces[i] = 0ULL;
	}
	whitePiecesOccupancy = 0ULL;
	blackPiecesOccupancy = 0ULL;
	allPiecesOccupancy = 0ULL;

	for (int square = a8; square <= h1; square++)
	{

		if (square == a8 || square == h8)
		{
			set_bit(bitboardPieces[bb_brook], square);
			mainBoard[square] = epc_brook;
		}
		else if (square == b8 || square == g8)
		{
			set_bit(bitboardPieces[bb_bknight], square);
			mainBoard[square] = epc_bknight;
		}
		else if (square == c8 || square == f8)
		{
			set_bit(bitboardPieces[bb_bbishop], square);
			mainBoard[square] = epc_bbishop;
		}
		else if (square == d8)
		{
			set_bit(bitboardPieces[bb_bqueen], square);
			mainBoard[square] = epc_bqueen;
		}
		else if (square == e8)
		{
			set_bit(bitboardPieces[bb_bking], square);
			mainBoard[square] = epc_bking;
		}
		else if (square == a7 || square == b7 || square == c7 || square == d7 || square == e7
			|| square == f7 || square == g7 || square == h7)
		{
			set_bit(bitboardPieces[bb_bpawn], square);
			mainBoard[square] = epc_bpawn;
		}
		else if (square == a1 || square == h1)
		{
			set_bit(bitboardPieces[bb_wrook], square);
			mainBoard[square] = epc_wrook;
		}
		else if (square == b1 || square == g1)
		{
			set_bit(bitboardPieces[bb_wknight], square);
			mainBoard[square] = epc_wknight;
		}
		else if (square == c1 || square == f1)
		{
			set_bit(bitboardPieces[bb_wbishop], square);
			mainBoard[square] = epc_wbishop;
		}
		else if (square == d1)
		{
			set_bit(bitboardPieces[bb_wqueen], square);
			mainBoard[square] = epc_wqueen;
		}
		else if (square == e1)
		{
			set_bit(bitboardPieces[bb_wking], square);
			mainBoard[square] = epc_wking;
		}
		else if (square == a2 || square == b2 || square == c2 || square == d2 || square == e2
			|| square == f2 || square == g2 || square == h2)
		{
			set_bit(bitboardPieces[bb_wpawn], square);
			mainBoard[square] = epc_wpawn;
		}
		else
		{
			mainBoard[square] = epc_empty;
		}
	}

	// Occupancy bitboards initialization
	whitePiecesOccupancy = bitboardPieces[bb_wrook] | bitboardPieces[bb_wpawn] | bitboardPieces[bb_wknight] | bitboardPieces[bb_wbishop] | bitboardPieces[bb_wqueen] | bitboardPieces[bb_wking];
	blackPiecesOccupancy = bitboardPieces[bb_brook] | bitboardPieces[bb_bpawn] | bitboardPieces[bb_bknight] | bitboardPieces[bb_bbishop] | bitboardPieces[bb_bqueen] | bitboardPieces[bb_bking];
	allPiecesOccupancy = whitePiecesOccupancy | blackPiecesOccupancy;
}

/*
--------------------

TRANSPOSITION TABLES
		  
--------------------
*/

U64 zobristPieces[12][64]; // every piece piece and square combo
U64 zobristSideToMove;
U64 positionKey; // current position hash, updated on make/unmake move

struct TTEntry {
	U64 key; // position hash 
	int score; // evaluation score
	int depth; // depth reached
	Move bestMove; // best move found
	uint8_t flag; // type of score (exact, upper, lower)
};

constexpr int TT_SIZE = 1048576; // 2^20
std::vector<TTEntry> transpositionTable(TT_SIZE);

enum TTFlag {
	TT_EXACT,
	TT_ALPHA,
	TT_BETA
};


void initializeZobrist()
{
	std::mt19937_64 rng(123456789);  // fixed seed = same numbers every run

	for (int piece = bb_wpawn; piece <= bb_bking; piece++)
	{
		for (int square = 0; square < 64; square++)
		{
			zobristPieces[piece][square] = rng();
		}
	}

	zobristSideToMove = rng();
}

U64 computePositionKey()
{
	// Called in every board initialization
	U64 key = 0ULL;

	for (int index = bb_wpawn; index <= bb_bking; index++)
	{
		for (int square = 0; square < 64; square++)
		{
			if (get_bit(bitboardPieces[index], square))
			{
				key ^= zobristPieces[index][square];
			}
		}
	}

	if (currentSideToMove == black)
	{
		key ^= zobristSideToMove;
	}

	return key;
}

bool isSquareAttacked(int square, Color byColor)
{
	// Pawn attacks
	int pawnDir = (byColor == white) ? oneRank : -oneRank;
	if (square + pawnDir + 1 >= 0 && square + pawnDir + 1 < 64 && getFile(square) < 8 && get_bit(bitboardPieces[(byColor == white) ? bb_wpawn : bb_bpawn], square + pawnDir + 1)) return true;
	if (square + pawnDir - 1 >= 0 && square + pawnDir - 1 < 64 && getFile(square) > 1 && get_bit(bitboardPieces[(byColor == white) ? bb_wpawn : bb_bpawn], square + pawnDir - 1)) return true;

	// Knight attacks
	int knightOffsets[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };
	for (int offset : knightOffsets)
	{
		if (square + offset < 0 || square + offset > 63) continue;

		int fromFile = square % 8;
		int toFile = (square + offset) % 8;
		int fileDiff = abs(toFile - fromFile);

		int fromRank = square / 8;
		int toRank = (square + offset) / 8;
		int rankDiff = abs(toRank - fromRank);

		if (!((fileDiff == 1 && rankDiff == 2) || (fileDiff == 2 && rankDiff == 1))) continue;

		if (get_bit(bitboardPieces[(byColor == white) ? bb_wknight : bb_bknight], square + offset)) return true;
	}

	// Bishop/Queen
	for (int i = 0; i <= 3; i++)
	{
		for (int j = 1; j <= 7; j++)
		{
			int squareToCheck = 0;
			if (i == 0) squareToCheck = square - j * (oneRank - 1);
			else if (i == 1) squareToCheck = square - j * (oneRank + 1);
			else if (i == 2) squareToCheck = square + j * (oneRank - 1);
			else if (i == 3) squareToCheck = square + j * (oneRank + 1);

			if (squareToCheck > 63 || squareToCheck < 0) break;

			int fromFile = getFile(square);
			int toFile = getFile(squareToCheck);
			if (abs(toFile - fromFile) != j) break;

			int fromRank = getRank(square);
			int toRank = getRank(squareToCheck);
			if (abs(toRank - fromRank) != j) break;

			if (get_bit(bitboardPieces[(byColor == white) ? bb_wbishop : bb_bbishop], squareToCheck)) return true;
			if (get_bit(bitboardPieces[(byColor == white) ? bb_wqueen : bb_bqueen], squareToCheck)) return true;
			if (get_bit(allPiecesOccupancy, squareToCheck) == 1) break;
		}
	}

	// Rook/Queen
	for (int i = 0; i <= 3; i++)
	{
		for (int j = 1; j <= 7; j++)
		{
			int squareToCheck = 0;
			if (i == 0)
			{
				squareToCheck = square - j;
				if (squareToCheck < 0 || getRank(square) != getRank(squareToCheck)) break;
			}
			else if (i == 1)
			{
				squareToCheck = square + j;
				if (squareToCheck > 63 || getRank(square) != getRank(squareToCheck)) break;
			}
			else if (i == 2) squareToCheck = square - j * oneRank;
			else if (i == 3) squareToCheck = square + j * oneRank;

			if (squareToCheck > 63 || squareToCheck < 0) break;

			if (get_bit(bitboardPieces[(byColor == white) ? bb_wrook : bb_brook], squareToCheck)) return true;
			if (get_bit(bitboardPieces[(byColor == white) ? bb_wqueen : bb_bqueen], squareToCheck)) return true;
			if (get_bit(allPiecesOccupancy, squareToCheck) == 1) break;
		}
	}

	// King attacks (for checking if kings are adjacent)
	int kingOffsets[8] = { -9, -8, -7, -1, 1, 7, 8, 9 };
	for (int offset : kingOffsets)
	{
		if (square + offset < 0 || square + offset > 63) continue;
		if (abs(getFile(square) - getFile(square + offset)) > 1) continue;
		if (abs(getRank(square) - getRank(square + offset)) > 1) continue;

		if (get_bit(bitboardPieces[(byColor == white) ? bb_wking : bb_bking], square + offset))
			return true;
	}

	return false;
}

int findKing(Color c)
{
	int kingIndex = (c == white) ? bb_wking : bb_bking;

	for (int square = 0; square < 64; square++)
	{
		if (get_bit(bitboardPieces[kingIndex], square))
		{
			return square;
		}
	}
	return -1;
}

bool isKingInCheck(Color c)
{
	int kingSquare = findKing(c);
	if (kingSquare == -1) return false;
	return isSquareAttacked(kingSquare, (c == white) ? black : white);
}

// Bonus for pawns in center
int pawnSquareTable[64] = {
	 0,  0,  0,  0,  0,  0,  0,  0,
	100, 100, 100, 100, 100, 100, 100, 100,
	10, 10, 20, 80, 80, 20, 10, 10,
	 5,  5, 10, 60, 60, 10,  5,  5,
	 0,  0,  20, 50, 50,  20,  0,  0,
	 5, -5,-10,  10,  10,-10, -5,  5,
	 5, 10, 10,-20,-20, 10, 10,  5,
	 0,  0,  0,  0,  0,  0,  0,  0
};

// Bonus for knights in center
int knightSquareTable[64] = {
   -50,-30,-30,-30,-30,-30,-30,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -40,  0, 10, 15, 15, 10,  0,-40,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -40,  5, 10, 15, 15, 10,  5,-40,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-30,-30,-30,-30,-30,-30,-50
};

// Bonus for king in corners
int kingSquareTable[64] = {
	-80,-70,-70,-70,-70,-70,-70,-80,
	-60,-60,-60,-60,-60,-60,-60,-60,
	-40,-40,-40,-40,-40,-40,-40,-40,
	-20,-20,-20,-20,-20,-20,-20,-20,
	  0,  0,  0,  0,  0,  0,  0,  0,
	 20, 20, 20, 20, 20, 20, 20, 20,
	 40, 40, 40, 40, 40, 40, 40, 40,
	 60, 100, 40, 20, 20, 40, 100, 60
};



int pieceEvaluation()
{
	// Adds when white benefits/black loses, subtracts when white loses/black benefits
	int evaluation{ 0 };

	int heavyPieces{ 0 };


	for (int square = a8; square <= h1; square++)
	{
		// From positioning & piece value

		// White pawn
		if (get_bit(bitboardPieces[bb_wpawn], square) == 1)
		{
			evaluation += pieceValue[bb_wpawn];
			evaluation += pawnSquareTable[square];

			if (get_bit(bitboardPieces[bb_wpawn], square - oneRank) == 1) evaluation -= 25; // Doubled
		}
		// Black pawn
		else if (get_bit(bitboardPieces[bb_bpawn], square) == 1)
		{
			evaluation += pieceValue[bb_bpawn];
			evaluation -= pawnSquareTable[h1 - square]; // Flip board

			if (get_bit(bitboardPieces[bb_bpawn], square - oneRank) == 1) evaluation += 25; // Doubled
		}
		// White knight
		else if (get_bit(bitboardPieces[bb_wknight], square) == 1)
		{
			heavyPieces += 1;
			evaluation += pieceValue[bb_wknight];
			evaluation += knightSquareTable[square];
		}
		// Black knight
		else if (get_bit(bitboardPieces[bb_bknight], square) == 1)
		{
			heavyPieces += 1;
			evaluation += pieceValue[bb_bknight];
			evaluation -= knightSquareTable[square];
		}
		// White bishop/queen
		else if (get_bit(bitboardPieces[bb_wbishop], square) == 1 || get_bit(bitboardPieces[bb_wqueen], square) == 1)
		{
			if (get_bit(bitboardPieces[bb_wbishop], square) == 1)
			{
				evaluation += pieceValue[bb_wbishop];
				heavyPieces += 1;
			}
			if (get_bit(bitboardPieces[bb_wqueen], square) == 1)
			{
				evaluation += pieceValue[bb_wqueen];
				heavyPieces += 1;
			}

			std::array<int, 4> bishopOffsets = { 7, 9, -7 , -9 };
			for (int offset : bishopOffsets)
			{

				int squareToMove = square + offset;

				if (squareToMove < 0 || squareToMove > 63) continue;

				// Wrap around check
				int fromFile = getFile(square);
				int toFile = getFile(squareToMove);
				if (abs(toFile - fromFile) != 1) continue;

				int fromRank = getRank(square);
				int toRank = getRank(squareToMove);
				if (abs(toRank - fromRank) != 1) continue;

				if (get_bit(whitePiecesOccupancy, squareToMove) == 0) evaluation += 20;
				else if (get_bit(blackPiecesOccupancy, squareToMove) == 1)
				{
					evaluation += 5;
					continue;
				}
			}
		}
		// Black bishop/queen
		else if (get_bit(bitboardPieces[bb_bbishop], square) == 1 || get_bit(bitboardPieces[bb_bqueen], square) == 1)
		{
			if (get_bit(bitboardPieces[bb_bbishop], square) == 1)
			{
				evaluation += pieceValue[bb_bbishop];
				heavyPieces += 1;
			}
			if (get_bit(bitboardPieces[bb_bqueen], square) == 1)
			{
				evaluation += pieceValue[bb_bqueen];
				heavyPieces += 1;
			}

			std::array<int, 4> bishopOffsets = { 7, 9, -7 , -9 };
			for (int offset : bishopOffsets)
			{
				int squareToMove = square + offset;

				if (squareToMove < 0 || squareToMove > 63) continue;

				// Wrap around check
				int fromFile = getFile(square);
				int toFile = getFile(squareToMove);
				if (abs(toFile - fromFile) != 1) continue;

				int fromRank = getRank(square);
				int toRank = getRank(squareToMove);
				if (abs(toRank - fromRank) != 1) continue;

				if (get_bit(blackPiecesOccupancy, squareToMove) == 0) evaluation -= 20;
				else if (get_bit(whitePiecesOccupancy, squareToMove) == 1)
				{
					evaluation -= 5;
					continue;
				}
			}
		}
		// White rook/queen
		else if (get_bit(bitboardPieces[bb_wrook], square) == 1 || get_bit(bitboardPieces[bb_wqueen], square) == 1)
		{
			if (get_bit(bitboardPieces[bb_wrook], square) == 1)
			{
				evaluation += pieceValue[bb_wrook];
				heavyPieces += 1;
			}
			if (get_bit(bitboardPieces[bb_wqueen], square) == 1) evaluation += pieceValue[bb_wqueen];

			if (square == a1)
			{
				if (get_bit(whitePiecesOccupancy, b1) == 1) evaluation -= 5;
				if (get_bit(whitePiecesOccupancy, a2) == 1) evaluation -= 5;
			}
			else if (square == h1)
			{
				if (get_bit(whitePiecesOccupancy, g1) == 1) evaluation -= 5;
				if (get_bit(whitePiecesOccupancy, h2) == 1) evaluation -= 5;
			}

			for (int i = 0; i <= 3; i++)
			{
				int squareToCheck = 0;
				if (i == 0)
				{
					squareToCheck = square - 1;
					if (squareToCheck < 0 || getRank(square) != getRank(squareToCheck)) continue;
				}
				else if (i == 1)
				{
					squareToCheck = square + 1;
					if (squareToCheck > 63 || getRank(square) != getRank(squareToCheck)) continue;
				}
				else if (i == 2) squareToCheck = square - oneRank;
				else if (i == 3) squareToCheck = square + oneRank;

				if (squareToCheck > 63 || squareToCheck < 0) continue;

				if (get_bit(whitePiecesOccupancy, squareToCheck) == 0) evaluation += 20;
				else if (get_bit(blackPiecesOccupancy, squareToCheck) == 1)
				{
					evaluation += 5;
					continue;
				}
			}
		}
		// Black rook/queen
		else if (get_bit(bitboardPieces[bb_brook], square) == 1 || get_bit(bitboardPieces[bb_bqueen], square) == 1)
		{
			if (get_bit(bitboardPieces[bb_brook], square) == 1)
			{
				evaluation += pieceValue[bb_brook];
				heavyPieces += 1;
			}
			if (get_bit(bitboardPieces[bb_bqueen], square) == 1) evaluation += pieceValue[bb_bqueen];

			if (square == a8)
			{
				if (get_bit(whitePiecesOccupancy, b8) == 1) evaluation += 5;
				if (get_bit(whitePiecesOccupancy, a7) == 1) evaluation += 5;
			}
			else if (square == h8)
			{
				if (get_bit(whitePiecesOccupancy, g8) == 1) evaluation += 5;
				if (get_bit(whitePiecesOccupancy, h7) == 1) evaluation += 5;
			}

			for (int i = 0; i <= 3; i++)
			{
				int squareToCheck = 0;
				if (i == 0)
				{
					squareToCheck = square - 1;
					if (squareToCheck < 0 || getRank(square) != getRank(squareToCheck)) continue;
				}
				else if (i == 1)
				{
					squareToCheck = square + 1;
					if (squareToCheck > 63 || getRank(square) != getRank(squareToCheck)) continue;
				}
				else if (i == 2) squareToCheck = square - oneRank;
				else if (i == 3) squareToCheck = square + oneRank;

				if (squareToCheck > 63 || squareToCheck < 0) continue;

				if (get_bit(blackPiecesOccupancy, squareToCheck) == 0) evaluation -= 20;
				else if (get_bit(whitePiecesOccupancy, squareToCheck) == 1)
				{
					evaluation -= 5;
					continue;
				}
			}
		}
		// White king
		else if (get_bit(bitboardPieces[bb_wking], square) == 1)
		{
			if (heavyPieces > 4) evaluation += kingSquareTable[square];
			if (get_bit(bitboardPieces[bb_wpawn], square - oneRank) == 1) evaluation += 50;
			if (get_bit(bitboardPieces[bb_wpawn], square - oneRank - 1) == 1) evaluation += 20;
			if (get_bit(bitboardPieces[bb_wpawn], square - oneRank + 1) == 1) evaluation += 20;
		}
		// Black king
		else if (get_bit(bitboardPieces[bb_bking], square) == 1)
		{
			if (heavyPieces > 4) evaluation -= kingSquareTable[63 - square]; // Flip
			if (get_bit(bitboardPieces[bb_bpawn], square + oneRank) == 1) evaluation -= 50;
			if (get_bit(bitboardPieces[bb_bpawn], square + oneRank - 1) == 1) evaluation -= 20;
			if (get_bit(bitboardPieces[bb_bpawn], square + oneRank + 1) == 1) evaluation -= 20;
		}
	}

	// Discourage early queen moves
	if (get_bit(bitboardPieces[bb_wqueen], d1) == 0)
	{
		if (get_bit(bitboardPieces[bb_wknight], b1) == 1) evaluation -= 25;
		if (get_bit(bitboardPieces[bb_wknight], g1) == 1) evaluation -= 25;

		if (get_bit(bitboardPieces[bb_wbishop], c1) == 1) evaluation -= 25;
		if (get_bit(bitboardPieces[bb_wbishop], f1) == 1) evaluation -= 25;
	}
	if (get_bit(bitboardPieces[bb_bqueen], d8) == 0)
	{
		if (get_bit(bitboardPieces[bb_bknight], b8) == 1) evaluation += 25;
		if (get_bit(bitboardPieces[bb_bknight], g8) == 1) evaluation += 25;

		if (get_bit(bitboardPieces[bb_bbishop], c8) == 1) evaluation += 25;
		if (get_bit(bitboardPieces[bb_bbishop], f8) == 1) evaluation += 25;
	}

	// Castling
	if (get_bit(bitboardPieces[bb_wking], g1)) evaluation += 60;
	else if (get_bit(bitboardPieces[bb_wking], c1)) evaluation += 40;

	if (get_bit(bitboardPieces[bb_bking], g8)) evaluation -= 60;
	else if (get_bit(bitboardPieces[bb_bking], c8)) evaluation -= 40;

	return evaluation;
}

int calculateEvaluation()
{
	int evaluation{ 0 };

	evaluation += pieceEvaluation();

	return evaluation;
}


void printBitboard(U64 bitboard)
{
	std::cout << '\n';

	// Loop over ranks
	for (int rank = 0; rank < 8; rank++)
	{

		// Loop over files
		for (int file = 0; file < 8; file++)
		{

			// Create square index
			int square = rank * 8 + file;

			// Print rank info
			if (file == 0)
			{
				std::cout << 8 - rank << "   ";
			}

			// Print bit state (either 1 or 0)
			std::cout << (get_bit(bitboard, square)) << " ";
		}

		// New line every rank
		std::cout << '\n';
	}

	// Print file info
	std::cout << '\n' << "    A B C D E F G H" << '\n';
}

void printMainboard()
{
	std::cout << "\n";

	for (int index = a8; index <= h1; index++)
	{

		// Print rank info
		if (index % 8 == 0)
		{
			int rank = 8 - (index / 8);
			std::cout << rank << "  ";
		}

		// Print square
		std::cout << pieceToChar(mainBoard[index]) << " ";

		// New line every rank
		if (index % 8 == 7)
		{
			std::cout << "\n";
		}
	}

	// Print file letters
	std::cout << "\n   A B C D E F G H\n";
	std::cout << '\n';
}

// Encode a move (Flag | From | To)
inline Move encodeMove(int from, int to, int flag)
{
	return (flag << 12) | (from << 6) | to;
}

inline int getFrom(Move m) { return (m >> 6) & 0x3F; }
inline int getTo(Move m) { return m & 0x3F; }
inline int getFlag(Move m) { return (m >> 12) & 0xF; }

std::vector<Move> whiteMoveLog;
std::vector<Move> blackMoveLog;

// GENERATE ALL PSEUDO LEGAL MOVES (checks legality after)
void getPseudoLegalMoves(Color c, std::array<Move, MAX_MOVES>& moveStack, int& moveCount)
{
	moveCount = 0;

	U64 currBitboard; // temporary bitboard for checking

	// Return opposite color to check for captures
	U64 currOccupancy = (c == white) ? whitePiecesOccupancy : blackPiecesOccupancy;
	U64 oppOccupancy = (c == white) ? blackPiecesOccupancy : whitePiecesOccupancy;

	bool kingSideCastlingRights = (c == white) ? whiteKingSideCastlingRights : blackKingSideCastlingRights;
	bool queenSideCastlingRights = (c == white) ? whiteQueenSideCastlingRights : blackQueenSideCastlingRights;

	// Iterate through every square
	for (int square = a8; square <= h1; square++)
	{
		// Pawn Moves
		if (c == white && moveCount < 256 && moveCount >= 0)
		{
			// White pawn moves
			if (get_bit(bitboardPieces[bb_wpawn], square) == 1)
			{

				// Push
				if (getRank(square) <= 6 && get_bit(allPiecesOccupancy, square - oneRank) == 0)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank, quiet_move);
						moveCount++;
					}
				}

				// Double push
				if (getRank(square) == 2 && get_bit(allPiecesOccupancy, square - oneRank) == 0
					&& get_bit(allPiecesOccupancy, square - oneRank * 2) == 0)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank * 2, double_pawn_push);
						moveCount++;
					}
				}

				// Pawn capture left
				if (getRank(square) <= 6 && getFile(square) > 1 && get_bit(oppOccupancy, square - oneRank - 1) == 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank - 1, capture);
						moveCount++;
					}
				}
				// Pawn capture right
				if (getFile(square) < 8 && getRank(square) <= 6 && get_bit(oppOccupancy, square - oneRank + 1) == 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank + 1, capture);
						moveCount++;
					}
				}

				// En passant capture
				int lastBlackMove = (blackMoveLog.size() != 0) ? blackMoveLog.back() : 0;
				int lastBlackFlag = getFlag(lastBlackMove);
				// Left
				if (lastBlackFlag == double_pawn_push && getFile(square) > 1 && getRank(square) == 5 && getTo(lastBlackMove) == square - 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank - 1, en_passant_capture);
						moveCount++;
					}
				}
				// Right
				if (lastBlackFlag == double_pawn_push && getFile(square) < 8 && getRank(square) == 5 && getTo(lastBlackMove) == square + 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank + 1, en_passant_capture);
						moveCount++;
					}
				}

				// Promotions (no capture)
				if (getRank(square) == 7 && get_bit(allPiecesOccupancy, square - oneRank) == 0)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank, knight_promotion);
						moveCount++;
					}

					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank, bishop_promotion);
						moveCount++;
					}

					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank, rook_promotion);
						moveCount++;
					}

					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank, queen_promotion);
						moveCount++;
					}
				}

				// Promotion capture left
				if (getFile(square) > 1 && getRank(square) == 7 && get_bit(oppOccupancy, square - oneRank - 1) == 1)
				{

					// Knight
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank - 1, knight_promo_capture);
						moveCount++;
					}

					// Bishop
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank - 1, bishop_promo_capture);
						moveCount++;
					}

					// Rook
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank - 1, rook_promo_capture);
						moveCount++;
					}

					// Queen
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank - 1, queen_promo_capture);
						moveCount++;
					}
				}
				// Promotion capture right
				if (getFile(square) < 8 && getRank(square) == 7 && get_bit(oppOccupancy, square - oneRank + 1) == 1)
				{

					// Knight
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank + 1, knight_promo_capture);
						moveCount++;
					}

					// Bishop
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank + 1, bishop_promo_capture);
						moveCount++;
					}

					// Rook
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank + 1, rook_promo_capture);
						moveCount++;
					}

					// Queen
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square - oneRank + 1, queen_promo_capture);
						moveCount++;
					}
				}
			}
		}
		else if (c == black && moveCount < 256 && moveCount >= 0)
		{
			// Black pawn moves
			if (get_bit(bitboardPieces[bb_bpawn], square) == 1)
			{
				// Push
				if (getRank(square) >= 3 && get_bit(allPiecesOccupancy, square + oneRank) == 0)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank, quiet_move);
						moveCount++;
					}
				}

				// Double push
				if (getRank(square) == 7 && get_bit(allPiecesOccupancy, square + oneRank) == 0
					&& get_bit(allPiecesOccupancy, square + oneRank * 2) == 0)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank * 2, double_pawn_push);
						moveCount++;
					}
				}

				// Pawn capture left
				if (getRank(square) >= 3 && getFile(square) > 1 && get_bit(oppOccupancy, square + oneRank - 1) == 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank - 1, capture);
						moveCount++;
					}
				}
				// Pawn capture right
				if (getRank(square) >= 3 && getFile(square) < 8 && get_bit(oppOccupancy, square + oneRank + 1) == 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank + 1, capture);
						moveCount++;
					}
				}

				// En passant capture
				int lastWhiteMove = (whiteMoveLog.size() != 0) ? whiteMoveLog.back() : 0;
				int lastWhiteFlag = getFlag(lastWhiteMove);
				// Left
				if (lastWhiteFlag == double_pawn_push && getFile(square) > 1 && getRank(square) == 4 && getTo(lastWhiteMove) == square - 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank - 1, en_passant_capture);
						moveCount++;
					}
				}
				// Right
				if (lastWhiteFlag == double_pawn_push && getFile(square) < 8 && getRank(square) == 4 && getTo(lastWhiteMove) == square + 1)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank + 1, en_passant_capture);
						moveCount++;
					}
				}

				// Promotions (no capture)
				if (getRank(square) == 2 && get_bit(allPiecesOccupancy, square + oneRank) == 0)
				{
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank, knight_promotion);
						moveCount++;
					}

					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank, bishop_promotion);
						moveCount++;
					}

					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank, rook_promotion);
						moveCount++;
					}

					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank, queen_promotion);
						moveCount++;
					}
				}

				// Promotion capture left
				if (getFile(square) > 1 && getRank(square) == 2 && get_bit(oppOccupancy, square + oneRank - 1) == 1)
				{

					// Knight
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank - 1, knight_promo_capture);
						moveCount++;
					}
					// Bishop
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank - 1, bishop_promo_capture);
						moveCount++;
					}
					// Rook
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank - 1, rook_promo_capture);
						moveCount++;
					}
					// Queen
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank - 1, queen_promo_capture);
						moveCount++;
					}
				}
				// Promotion capture right
				if (getFile(square) < 8 && getRank(square) == 2 && get_bit(oppOccupancy, square + oneRank + 1) == 1)
				{

					// Knight
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank + 1, knight_promo_capture);
						moveCount++;
					}
					// Bishop
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank + 1, bishop_promo_capture);
						moveCount++;
					}
					// Rook
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank + 1, rook_promo_capture);
						moveCount++;
					}
					// Queen
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + oneRank + 1, queen_promo_capture);
						moveCount++;
					}
				}
			}
		}


		// Knight

		currBitboard = (c == white) ? bitboardPieces[bb_wknight] : bitboardPieces[bb_bknight];

		if (get_bit(currBitboard, square) == 1)
		{
			int knightOffsets[8] = { -17, -15, -10, -6, 6, 10, 15, 17 };

			for (int offset : knightOffsets)
			{
				if (square + offset > 63 || square + offset < 0) continue;

				int fromFile = square % 8;
				int toFile = (square + offset) % 8;
				int fileDiff = abs(toFile - fromFile);

				int fromRank = square / 8;
				int toRank = (square + offset) / 8;
				int rankDiff = abs(toRank - fromRank);

				if (!((fileDiff == 1 && rankDiff == 2) || (fileDiff == 2 && rankDiff == 1))) continue;
				if (abs((square + offset) - square) > 17) continue;

				if (get_bit(allPiecesOccupancy, square + offset) == 0)
				{
					// Quiet move
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + offset, quiet_move);
						moveCount++;
					}
				}
				else if (get_bit(oppOccupancy, square + offset) == 1)
				{
					// Capture
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + offset, capture);
						moveCount++;
					}
				}

			}
		}

		// Bishop

		currBitboard = (c == white) ? bitboardPieces[bb_wbishop] : bitboardPieces[bb_bbishop];

		if (get_bit(currBitboard, square) == 1)
		{
			int squareToMove{ 0 };

			// Quiet moves & Captures
			for (int i = 0; i <= 3; i++)
			{
				for (int j = 1; j <= 7; j++)
				{
					if (i == 0) { squareToMove = square - j * (oneRank - 1); } // Top left
					else if (i == 1) { squareToMove = square - j * (oneRank + 1); } // Top right
					else if (i == 2) { squareToMove = square + j * (oneRank - 1); } // Bottom left
					else if (i == 3) { squareToMove = square + j * (oneRank + 1); } // Bottom right

					if (squareToMove > 63 || squareToMove < 0) break;

					// Prevent file wrap around
					int fromFile = square % 8;
					int toFile = squareToMove % 8;
					if (abs(toFile - fromFile) != j) break;

					// Prevent other bugs
					int fromRank = getRank(square);
					int toRank = getRank(squareToMove);
					if (abs(toRank - fromRank) != j) break;

					if (get_bit(allPiecesOccupancy, squareToMove) == 0)
					{
						// Quiet move
						if (moveCount < MAX_MOVES)
						{
							moveStack[moveCount] = encodeMove(square, squareToMove, quiet_move);
							moveCount++;
						}
					}
					else if (get_bit(oppOccupancy, squareToMove) == 1)
					{
						// Capture
						if (moveCount < MAX_MOVES)
						{
							moveStack[moveCount] = encodeMove(square, squareToMove, capture);
							moveCount++;
						}
						break;
					}
					else if (get_bit(currOccupancy, squareToMove) == 1)
					{
						// Same color piece blocks the way, break.
						break;
					}
				}
			}
		}

		// Rook

		currBitboard = (c == white) ? bitboardPieces[bb_wrook] : bitboardPieces[bb_brook];

		if (get_bit(currBitboard, square) == 1)
		{
			int squareToMove{ 0 };

			// Quiet moves & Captures
			for (int i = 0; i <= 3; i++)
			{
				for (int j = 1; j <= 7; j++)
				{
					if (i == 0)
					{
						// Left
						squareToMove = square - j;
						if (getRank(square) != getRank(squareToMove)) { break; }
					}
					else if (i == 1)
					{
						// Right
						squareToMove = square + j;
						if (getRank(square) != getRank(squareToMove)) { break; }
					}
					else if (i == 2) { squareToMove = square - j * (oneRank); } // Up
					else if (i == 3) { squareToMove = square + j * (oneRank); } // Down

					if (squareToMove > 63 || squareToMove < 0 || moveCount >= 256) break;

					if (get_bit(allPiecesOccupancy, squareToMove) == 0)
					{
						// Quiet move
						if (moveCount < MAX_MOVES)
						{
							moveStack[moveCount] = encodeMove(square, squareToMove, quiet_move);
							moveCount++;
						}
					}
					else if (get_bit(oppOccupancy, squareToMove) == 1)
					{
						// Capture
						if (moveCount < MAX_MOVES)
						{
							moveStack[moveCount] = encodeMove(square, squareToMove, capture);
							moveCount++;
						}
						break;
					}
					else if (get_bit(currOccupancy, squareToMove) == 1)
					{
						// Same color piece blocks the way, break.
						break;
					}
				}
			}
		}

		// Queen

		currBitboard = (c == white) ? bitboardPieces[bb_wqueen] : bitboardPieces[bb_bqueen];

		if (get_bit(currBitboard, square) == 1)
		{
			int squareToMove{ 0 };

			// Quiet moves & Captures
			for (int i = 0; i <= 7; i++)
			{
				for (int j = 1; j <= 7; j++)
				{
					if (i == 0)
					{
						// Left
						squareToMove = square - j;
						if (getRank(square) != getRank(squareToMove)) { break; }
					}
					else if (i == 1)
					{
						// Right
						squareToMove = square + j;
						if (getRank(square) != getRank(squareToMove)) { break; }
					}
					else if (i == 2) { squareToMove = square - j * (oneRank); } // Up
					else if (i == 3) { squareToMove = square + j * (oneRank); } // Down
					else if (i == 4) { squareToMove = square - j * (oneRank - 1); } // Top left
					else if (i == 5) { squareToMove = square - j * (oneRank + 1); } // Top right
					else if (i == 6) { squareToMove = square + j * (oneRank - 1); } // Bottom left
					else if (i == 7) { squareToMove = square + j * (oneRank + 1); } // Bottom right

					if (squareToMove > 63 || squareToMove < 0 || moveCount >= 256) break;

					if (i >= 4)
					{
						// Prevent file wrap around
						int fromFile = square % 8;
						int toFile = squareToMove % 8;
						if (abs(toFile - fromFile) != j) break;

						// Prevent other bugs
						int fromRank = getRank(square);
						int toRank = getRank(squareToMove);
						if (abs(toRank - fromRank) != j) break;
					}


					if (get_bit(allPiecesOccupancy, squareToMove) == 0)
					{
						// Quiet move
						if (moveCount < MAX_MOVES)
						{
							moveStack[moveCount] = encodeMove(square, squareToMove, quiet_move);
							moveCount++;
						}
					}
					else if (get_bit(oppOccupancy, squareToMove) == 1)
					{
						// Capture
						if (moveCount < MAX_MOVES)
						{
							moveStack[moveCount] = encodeMove(square, squareToMove, capture);
							moveCount++;
						}
						break;
					}
					else if (get_bit(currOccupancy, squareToMove) == 1)
					{
						// Same piece blocks the way, break.
						break;
					}
				}
			}
		}

		// King

		currBitboard = (c == white) ? bitboardPieces[bb_wking] : bitboardPieces[bb_bking];

		if (get_bit(currBitboard, square) == 1 && moveCount < 256)
		{
			int kingOffsets[8] = { -9, -8, -7, -1, 1, 8 - 1, 8, 9 };
			for (int offset : kingOffsets)
			{
				if (square + offset < 0 || square + offset > 63) continue;

				if (abs(getFile(square) - getFile(square + offset)) > 1 || abs(getRank(square) - getRank(square + offset)) > 1) continue;

				if (isSquareAttacked(square + offset, (c == white) ? black : white)) continue;

				if (get_bit(allPiecesOccupancy, square + offset) == 0)
				{
					/// Quiet move
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + offset, quiet_move);
						moveCount++;
					}
				}
				else if (get_bit(oppOccupancy, square + offset) == 1)
				{
					// Capture
					if (moveCount < MAX_MOVES)
					{
						moveStack[moveCount] = encodeMove(square, square + offset, capture);
						moveCount++;
					}
				}
			}

			int kingSquare = (c == white) ? e1 : e8;

			if (kingSideCastlingRights && get_bit(currBitboard, kingSquare) == 1 && get_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], (c == white) ? h1 : h8) == 1
				&& get_bit(allPiecesOccupancy, square + 1) == 0 && get_bit(allPiecesOccupancy, square + 2) == 0)
			{
				// King side castling
				if (moveCount < MAX_MOVES)
				{
					// Ensure no castling through checks
					Color opponent = (c == white) ? black : white;
					if (!isSquareAttacked(square, opponent) && !isSquareAttacked(square + 1, opponent) && !isSquareAttacked(square + 2, opponent))
					{
						moveStack[moveCount] = encodeMove(square, square + 2, king_side_castle);
						moveCount++;
					}
				}
			}

			if (queenSideCastlingRights && get_bit(currBitboard, kingSquare) == 1 && get_bit(bitboardPieces[((c == white) ? bb_wrook : bb_brook)], ((c == white) ? a1 : a8)) == 1
				&& get_bit(allPiecesOccupancy, square - 1) == 0 && get_bit(allPiecesOccupancy, square - 2) == 0 && get_bit(allPiecesOccupancy, square - 3) == 0)
			{
				// Queen side castling
				if (moveCount < MAX_MOVES)
				{
					// Ensure no castling through checks
					Color opponent = (c == white) ? black : white;
					if (!isSquareAttacked(square, opponent) && !isSquareAttacked(square - 1, opponent) && !isSquareAttacked(square - 2, opponent))
					{
						moveStack[moveCount] = encodeMove(square, square - 2, queen_side_castle);
						moveCount++;
					}
				}
			}
		}
	}

	if (moveCount > MAX_MOVES)
	{
		moveCount = MAX_MOVES;  // Clamp it
	}
}

std::array<bool, MAX_DEPTH> savedWKS, savedWQS, savedBKS, savedBQS;

void makeMove(Move m, Color c, int ply)
{
	savedWKS[ply] = whiteKingSideCastlingRights;
	savedWQS[ply] = whiteQueenSideCastlingRights;
	savedBKS[ply] = blackKingSideCastlingRights;
	savedBQS[ply] = blackQueenSideCastlingRights;

	int from = getFrom(m);
	int to = getTo(m);
	int flag = getFlag(m);

	int currPieceIndex = getPieceIndex(from);

	int pawnbbIndex = (c == white) ? bb_wpawn : bb_bpawn;

	int& whichOppPieceIndex = (c == white) ? whichBlackPieceIndexWasThere[ply] : whichWhitePieceIndexWasThere[ply];

	Color cOpp = (c == white) ? black : white;

	U64& currOccupancy = (c == white) ? whitePiecesOccupancy : blackPiecesOccupancy;
	U64& oppOccupancy = (c == white) ? blackPiecesOccupancy : whitePiecesOccupancy;

	bool& kingSideCastlingRights = (c == white) ? whiteKingSideCastlingRights : blackKingSideCastlingRights;
	bool& queenSideCastlingRights = (c == white) ? whiteQueenSideCastlingRights : blackQueenSideCastlingRights;

	if (flag == capture || flag >= knight_promo_capture)
	{
		whichOppPieceIndex = getPieceIndex(to); // No need to check for ep, a pawn was always there
	}

	// Quiet move / pawn double push
	if (flag == quiet_move || flag == double_pawn_push)
	{
		pop_bit(bitboardPieces[currPieceIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		set_bit(bitboardPieces[currPieceIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, currPieceIndex);

		positionKey ^= zobristPieces[currPieceIndex][from];
		positionKey ^= zobristPieces[currPieceIndex][to];

	}

	// Capture
	else if (flag == capture)
	{
		pop_bit(bitboardPieces[currPieceIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		pop_bit(bitboardPieces[getPieceIndex(to)], to);
		pop_bit(oppOccupancy, to);

		set_bit(bitboardPieces[currPieceIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, currPieceIndex);

		positionKey ^= zobristPieces[currPieceIndex][from];
		positionKey ^= zobristPieces[getPieceIndex(to)][to];
		positionKey ^= zobristPieces[currPieceIndex][to];
	}

	// Promotion (no capture)
	else if (flag >= knight_promotion && flag <= queen_promotion)
	{
		int promoIndex{ 0 };
		if (flag == knight_promotion)
		{
			promoIndex = (c == white) ? bb_wknight : bb_bknight;
		}
		else if (flag == bishop_promotion)
		{
			promoIndex = (c == white) ? bb_wbishop : bb_bbishop;
		}
		else if (flag == rook_promotion)
		{
			promoIndex = (c == white) ? bb_wrook : bb_brook;
		}
		else if (flag == queen_promotion)
		{
			promoIndex = (c == white) ? bb_wqueen : bb_bqueen;
		}

		pop_bit(bitboardPieces[pawnbbIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		set_bit(bitboardPieces[promoIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, promoIndex);

		positionKey ^= zobristPieces[pawnbbIndex][from];
		positionKey ^= zobristPieces[promoIndex][to];
	}

	// Promotion with capture
	else if (flag >= knight_promo_capture)
	{
		int promoIndex{ 0 };
		if (flag == knight_promo_capture)
		{
			promoIndex = (c == white) ? bb_wknight : bb_bknight;
		}
		else if (flag == bishop_promo_capture)
		{
			promoIndex = (c == white) ? bb_wbishop : bb_bbishop;
		}
		else if (flag == rook_promo_capture)
		{
			promoIndex = (c == white) ? bb_wrook : bb_brook;
		}
		else if (flag == queen_promo_capture)
		{
			promoIndex = (c == white) ? bb_wqueen : bb_bqueen;
		}

		pop_bit(bitboardPieces[pawnbbIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		pop_bit(bitboardPieces[getPieceIndex(to)], to);
		pop_bit(oppOccupancy, to);

		set_bit(bitboardPieces[promoIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, promoIndex);

		positionKey ^= zobristPieces[pawnbbIndex][from];
		positionKey ^= zobristPieces[getPieceIndex(to)][to];
		positionKey ^= zobristPieces[promoIndex][to];
	}

	// En passant
	else if (flag == en_passant_capture)
	{
		//printMainboard();
		whichOppPieceIndex = (c == white) ? bb_bpawn : bb_wpawn; // Store captured pawn
		pop_bit(bitboardPieces[pawnbbIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		pop_bit(bitboardPieces[(c == white) ? bb_bpawn : bb_wpawn], to + ((c == white) ? oneRank : -oneRank));
		pop_bit(oppOccupancy, to + ((c == white) ? oneRank : -oneRank));
		pop_bit(allPiecesOccupancy, to + ((c == white) ? oneRank : -oneRank));
		mainBoard[to + ((c == white) ? oneRank : -oneRank)] = epc_empty;

		set_bit(bitboardPieces[pawnbbIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, pawnbbIndex);
		//printMainboard();

		positionKey ^= zobristPieces[pawnbbIndex][from];
		positionKey ^= zobristPieces[(c == white) ? bb_bpawn : bb_wpawn][to + ((c == white) ? oneRank : -oneRank)];
		positionKey ^= zobristPieces[pawnbbIndex][to];
	}

	// King side castling
	else if (flag == king_side_castle && kingSideCastlingRights)
	{
		int kingIndex = (c == white) ? bb_wking : bb_bking;

		pop_bit(bitboardPieces[kingIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		pop_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], from + 3);
		pop_bit(currOccupancy, from + 3);
		pop_bit(allPiecesOccupancy, from + 3);
		mainBoard[from] = epc_empty;
		mainBoard[from + 3] = epc_empty;

		set_bit(bitboardPieces[kingIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		set_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], to - 1);
		set_bit(currOccupancy, to - 1);
		set_bit(allPiecesOccupancy, to - 1);
		mainBoard[to] = convertPieceIndexToEPC(c, kingIndex);
		mainBoard[to - 1] = convertPieceIndexToEPC(c, (c == white) ? bb_wrook : bb_brook);

		positionKey ^= zobristPieces[kingIndex][from];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][from + 3];

		positionKey ^= zobristPieces[kingIndex][to];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][to - 1];
	}

	// Queen side castling
	else if (flag == queen_side_castle && queenSideCastlingRights)
	{
		int kingIndex = (c == white) ? bb_wking : bb_bking;

		pop_bit(bitboardPieces[kingIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		pop_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], from - 4);
		pop_bit(currOccupancy, from - 4);
		pop_bit(allPiecesOccupancy, from - 4);
		mainBoard[from] = epc_empty;
		mainBoard[from - 4] = epc_empty;

		set_bit(bitboardPieces[kingIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		set_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], to + 1);
		set_bit(currOccupancy, to + 1);
		set_bit(allPiecesOccupancy, to + 1);
		mainBoard[to] = convertPieceIndexToEPC(c, kingIndex);
		mainBoard[to + 1] = convertPieceIndexToEPC(c, (c == white) ? bb_wrook : bb_brook);

		positionKey ^= zobristPieces[kingIndex][from];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][from - 4];

		positionKey ^= zobristPieces[kingIndex][to];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][to + 1];
	}

	((c == white) ? whiteMoveLog : blackMoveLog).push_back(m);

	if (from == e1 || to == e1) { whiteKingSideCastlingRights = false; whiteQueenSideCastlingRights = false; }
	if (from == e8 || to == e8) { blackKingSideCastlingRights = false; blackQueenSideCastlingRights = false; }
	if (from == a1 || to == a1) whiteQueenSideCastlingRights = false;
	if (from == h1 || to == h1) whiteKingSideCastlingRights = false;
	if (from == a8 || to == a8) blackQueenSideCastlingRights = false;
	if (from == h8 || to == h8) blackKingSideCastlingRights = false;

	positionKey ^= zobristSideToMove;
}

void unmakeMove(Move m, Color c, int ply)
{
	int from = getTo(m);
	int to = getFrom(m);
	int flag = getFlag(m);

	int currPieceIndex = getPieceIndex(from);

	int pawnbbIndex = (c == white) ? bb_wpawn : bb_bpawn;

	int whichOppPieceIndex = (c == white) ? whichBlackPieceIndexWasThere[ply] : whichWhitePieceIndexWasThere[ply];

	Color cOpp = (c == white) ? black : white;

	U64& currOccupancy = (c == white) ? whitePiecesOccupancy : blackPiecesOccupancy;
	U64& oppOccupancy = (c == white) ? blackPiecesOccupancy : whitePiecesOccupancy;

	bool& kingSideCastlingRights = (c == white) ? whiteKingSideCastlingRights : blackKingSideCastlingRights;
	bool& queenSideCastlingRights = (c == white) ? whiteQueenSideCastlingRights : blackQueenSideCastlingRights;

	// Quiet move / double pawn push
	if (flag == quiet_move || flag == double_pawn_push)
	{
		pop_bit(bitboardPieces[currPieceIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		set_bit(bitboardPieces[currPieceIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, currPieceIndex);

		positionKey ^= zobristPieces[currPieceIndex][from];
		positionKey ^= zobristPieces[currPieceIndex][to];
	}

	// Capture
	else if (flag == capture)
	{
		pop_bit(bitboardPieces[currPieceIndex], from);
		pop_bit(currOccupancy, from);
		mainBoard[from] = convertPieceIndexToEPC(cOpp, whichOppPieceIndex);

		set_bit(bitboardPieces[whichOppPieceIndex], from);
		set_bit(oppOccupancy, from);

		set_bit(bitboardPieces[currPieceIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, currPieceIndex);

		positionKey ^= zobristPieces[currPieceIndex][from];
		positionKey ^= zobristPieces[whichOppPieceIndex][from];
		positionKey ^= zobristPieces[currPieceIndex][to];
	}

	// Promotion without capture
	else if (flag >= knight_promotion && flag <= queen_promotion)
	{
		int promoIndex{ 0 };
		if (flag == knight_promotion)
		{
			promoIndex = (c == white) ? bb_wknight : bb_bknight;
		}
		else if (flag == bishop_promotion)
		{
			promoIndex = (c == white) ? bb_wbishop : bb_bbishop;
		}
		else if (flag == rook_promotion)
		{
			promoIndex = (c == white) ? bb_wrook : bb_brook;
		}
		else if (flag == queen_promotion)
		{
			promoIndex = (c == white) ? bb_wqueen : bb_bqueen;
		}

		pop_bit(bitboardPieces[promoIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		set_bit(bitboardPieces[pawnbbIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, pawnbbIndex);

		positionKey ^= zobristPieces[pawnbbIndex][from];
		positionKey ^= zobristPieces[pawnbbIndex][to];
	}

	// Promotion with capture
	else if (flag >= knight_promo_capture && flag <= queen_promo_capture)
	{
		int promoIndex{ 0 };
		if (flag == knight_promo_capture)
		{
			promoIndex = (c == white) ? bb_wknight : bb_bknight;
		}
		else if (flag == bishop_promo_capture)
		{
			promoIndex = (c == white) ? bb_wbishop : bb_bbishop;
		}
		else if (flag == rook_promo_capture)
		{
			promoIndex = (c == white) ? bb_wrook : bb_brook;
		}
		else if (flag == queen_promo_capture)
		{
			promoIndex = (c == white) ? bb_wqueen : bb_bqueen;
		}

		pop_bit(bitboardPieces[promoIndex], from);
		pop_bit(currOccupancy, from);
		mainBoard[from] = convertPieceIndexToEPC(cOpp, whichOppPieceIndex);

		set_bit(bitboardPieces[whichOppPieceIndex], from);
		set_bit(oppOccupancy, from);

		set_bit(bitboardPieces[pawnbbIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, pawnbbIndex);

		positionKey ^= zobristPieces[promoIndex][from];
		positionKey ^= zobristPieces[whichOppPieceIndex][from];
		positionKey ^= zobristPieces[pawnbbIndex][to];
	}

	// En passant
	else if (flag == en_passant_capture)
	{
		//printMainboard();
		pop_bit(bitboardPieces[pawnbbIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		mainBoard[from] = epc_empty;

		int capturedPawnSquare = getTo(m) + ((c == white) ? oneRank : -oneRank);

		set_bit(bitboardPieces[(c == white) ? bb_bpawn : bb_wpawn], capturedPawnSquare);
		set_bit(oppOccupancy, capturedPawnSquare);
		set_bit(allPiecesOccupancy, capturedPawnSquare);
		mainBoard[capturedPawnSquare] = convertPieceIndexToEPC(cOpp, (c == white) ? bb_bpawn : bb_wpawn);

		set_bit(bitboardPieces[pawnbbIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		mainBoard[to] = convertPieceIndexToEPC(c, pawnbbIndex);
		//printMainboard();

		positionKey ^= zobristPieces[pawnbbIndex][from];
		positionKey ^= zobristPieces[(c == white) ? bb_bpawn : bb_wpawn][capturedPawnSquare];
		positionKey ^= zobristPieces[pawnbbIndex][to];
	}

	// King side castling
	else if (flag == king_side_castle)
	{
		int kingIndex = (c == white) ? bb_wking : bb_bking;

		pop_bit(bitboardPieces[kingIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		pop_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], from - 1);
		pop_bit(currOccupancy, from - 1);
		pop_bit(allPiecesOccupancy, from - 1);
		mainBoard[from] = epc_empty;
		mainBoard[from - 1] = epc_empty;

		set_bit(bitboardPieces[kingIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		set_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], to + 3);
		set_bit(currOccupancy, to + 3);
		set_bit(allPiecesOccupancy, to + 3);
		mainBoard[to] = convertPieceIndexToEPC(c, kingIndex);
		mainBoard[to + 3] = convertPieceIndexToEPC(c, (c == white) ? bb_wrook : bb_brook);

		positionKey ^= zobristPieces[kingIndex][from];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][from - 1];

		positionKey ^= zobristPieces[kingIndex][to];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][to + 3];
	}

	// Queen side castling
	else if (flag == queen_side_castle)
	{
		int kingIndex = (c == white) ? bb_wking : bb_bking;

		pop_bit(bitboardPieces[kingIndex], from);
		pop_bit(currOccupancy, from);
		pop_bit(allPiecesOccupancy, from);
		pop_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], from + 1);
		pop_bit(currOccupancy, from + 1);
		pop_bit(allPiecesOccupancy, from + 1);
		mainBoard[from] = epc_empty;
		mainBoard[from + 1] = epc_empty;

		set_bit(bitboardPieces[kingIndex], to);
		set_bit(currOccupancy, to);
		set_bit(allPiecesOccupancy, to);
		set_bit(bitboardPieces[(c == white) ? bb_wrook : bb_brook], to - 4);
		set_bit(currOccupancy, to - 4);
		set_bit(allPiecesOccupancy, to - 4);
		mainBoard[to] = convertPieceIndexToEPC(c, kingIndex);
		mainBoard[to - 4] = convertPieceIndexToEPC(c, (c == white) ? bb_wrook : bb_brook);

		positionKey ^= zobristPieces[kingIndex][from];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][from + 1];

		positionKey ^= zobristPieces[kingIndex][to];
		positionKey ^= zobristPieces[(c == white) ? bb_wrook : bb_brook][to - 4];
	}

	((c == white) ? whiteMoveLog : blackMoveLog).pop_back();

	whiteKingSideCastlingRights = savedWKS[ply];
	whiteQueenSideCastlingRights = savedWQS[ply];
	blackKingSideCastlingRights = savedBKS[ply];
	blackQueenSideCastlingRights = savedBQS[ply];

	positionKey ^= zobristSideToMove;
}

std::array<int, 12> pieceValueMVV = { 100, 100, 300, 300, 300, 300, 500, 500, 900, 900, 10000, 10000 };

int scoreMove(Color c, Move m)
{
	int from = getFrom(m);
	int to = getTo(m);
	int flag = getFlag(m);

	if (flag == capture || flag >= knight_promo_capture)
	{
		int attackerValue = pieceValueMVV[getPieceIndex(from)];
		int victimValue = pieceValueMVV[getPieceIndex(to)];

		return victimValue * 10 - attackerValue;
	}
	else if (flag == knight_promotion)
	{
		return 3000;
	}
	else if (flag == bishop_promotion)
	{
		return 3000;
	}
	else if (flag == rook_promotion)
	{
		return 5000;
	}
	else if (flag == queen_promotion)
	{
		return 9000;
	}
	else if (flag == king_side_castle)
	{
		return 2000;
	}
	else if (flag == queen_side_castle)
	{
		return 1500;
	}

	return 0;
}

void sortMoves(Color c, std::array<Move, MAX_MOVES>& moves, int moveCount)
{
	for (int i = 1; i < moveCount; i++)
	{
		Move key = moves[i];
		int keyScore = scoreMove(c, key);
		int j = i - 1;

		while (j >= 0 && scoreMove(c, moves[j]) < keyScore)
		{
			moves[j + 1] = moves[j];
			j--;
		}
		if (j < MAX_MOVES) // silence a warning
		{
			moves[j + 1] = key;
		}
	}
}

// Search Algorithms

int quiescence(Color c, int alpha, int beta, int ply)
{
	if (ply >= MAX_DEPTH - 1)
	{
		int eval = calculateEvaluation();
		return (c == white) ? eval : -eval;
	}

	int static_eval = calculateEvaluation();
	if (c == black) static_eval = -static_eval;

	// Stand Pat
	int best_value = static_eval;
	if (best_value >= beta) return best_value;
	if (best_value + 1000 < alpha) return alpha; // Skip hopeless captures
	if (best_value > alpha) alpha = best_value;

	std::array<Move, MAX_MOVES> moves;
	int moveCount = 0;
	getPseudoLegalMoves(c, moves, moveCount);

	for (int i = 0; i < moveCount; i++)
	{
		int flag = getFlag(moves[i]);
		if (flag != capture && flag != en_passant_capture && flag < knight_promo_capture) continue;
		// ignore non captures

		makeMove(moves[i], c, ply);

		if (!isKingInCheck(c))
		{
			int score = -quiescence((c == white) ? black : white, -beta, -alpha, ply + 1);

			unmakeMove(moves[i], c, ply);

			if (score >= beta) return score;
			if (score > best_value) best_value = score;
			if (score > alpha) alpha = score;
		}
		else unmakeMove(moves[i], c, ply);
	}

	return best_value;
}

SearchResult negaMax(Color c, int alpha, int beta, int depthLeft, int ply)
{
	std::vector<Move> moveLog = (c == white) ? whiteMoveLog : blackMoveLog;
	int originalAlpha = alpha;
	int index = positionKey % TT_SIZE;
	TTEntry* entry = &transpositionTable[index];

	if (entry->key == positionKey && entry->depth >= depthLeft)
	{
		if (entry->flag == TT_EXACT)
		{
			return { entry->bestMove, entry->score };
		}
		else if (entry->flag == TT_ALPHA && entry->score <= alpha)
		{
			//position is bad
			return { entry->bestMove, alpha };
		}
		else if (entry->flag == TT_BETA && entry->score >= beta)
		{
			//position is good (cutoff)
			return { entry->bestMove, beta };
		}
	}

	if (depthLeft == 0)
	{
		//int evaluation = calculateEvaluation();
		//return { 0, (c == white) ? evaluation : -evaluation; }
		return { 0, quiescence(c, alpha, beta, ply) };
	}

	int bestValue = minScore;
	Move bestMove = 0;
	bool hasLegalMoves{ false };

	getPseudoLegalMoves(c, moveStack[ply], moveCountStack[ply]); // get pseudo legal moves
	sortMoves(c, moveStack[ply], moveCountStack[ply]);

	// Search stored tt table move from invalid score
	Move ttMove = 0;
	if (entry->key == positionKey) ttMove = entry->bestMove;

	if (ttMove != 0)
	{
		makeMove(ttMove, c, ply);

		if (!isKingInCheck(c))
		{
			hasLegalMoves = true;
			SearchResult result = negaMax((c == white) ? black : white, -beta, -alpha, depthLeft - 1, ply + 1);
			int score = -result.score;

			if (score > bestValue)
			{
				bestValue = score;
				bestMove = ttMove;
				if (score > alpha)
				{
					alpha = score;
				}
			}
			if (score >= beta)
			{
				unmakeMove(ttMove, c, ply);
				transpositionTable[positionKey % TT_SIZE] = { positionKey, bestValue, depthLeft, bestMove, TT_BETA };
				return { bestMove, bestValue };
			}
		}

		unmakeMove(ttMove, c, ply);
	}

	// Search rest of moves
	for (int i = 0; i < moveCountStack[ply]; i++)
	{
		if (moveLog.size() > 4 && (moveStack[ply][i] == moveLog[moveLog.size() - 2] || moveStack[ply][i] == moveLog[moveLog.size() - 4])) continue; // Avoid 3 fold;
		if (moveStack[ply][i] == ttMove) continue;

		makeMove(moveStack[ply][i], c, ply);
		//printMainboard();
		if (!isKingInCheck(c))
		{
			hasLegalMoves = true; // Found legal move

			SearchResult result = negaMax((c == white) ? black : white, -beta, -alpha, depthLeft - 1, ply + 1);
			int score = -result.score;
			if (score > bestValue)
			{
				bestValue = score;
				bestMove = moveStack[ply][i];
				if (score > alpha)
				{
					alpha = score;
				}
			}
			if (score >= beta)
			{
				unmakeMove(moveStack[ply][i], c, ply);
				//printMainboard(); 
				return { bestMove, score };
			}
		}
		unmakeMove(moveStack[ply][i], c, ply);
		//printMainboard();
	}

	uint8_t ttFlag;
	if (bestValue <= originalAlpha)
	{
		ttFlag = TT_ALPHA;
	}
	else if (bestValue >= beta)
	{
		ttFlag = TT_BETA;
	}
	else
	{
		ttFlag = TT_EXACT;
	}
	transpositionTable[positionKey % TT_SIZE] = { positionKey, bestValue, depthLeft, bestMove, ttFlag };

	if (!hasLegalMoves)
	{
		if (isKingInCheck(c)) return { 0, checkmateScore + ply }; // Checkmate
		else return { 0, 0 }; // Stalemate
	}
	return { bestMove, bestValue };
}

bool isGameOver(Color sideToMove)
{
	std::array<Move, MAX_MOVES> moveList;
	int moveCount;
	getPseudoLegalMoves(sideToMove, moveList, moveCount);

	bool hasLegalMove = false;

	for (int i = 0; i < moveCount; i++)
	{
		makeMove(moveList[i], sideToMove, 0);

		if (!isKingInCheck(sideToMove))
		{
			// Found a legal move
			hasLegalMove = true;
			unmakeMove(moveList[i], sideToMove, 0);
			break;  // no need to check more
		}

		unmakeMove(moveList[i], sideToMove, 0);
	}


	if (!hasLegalMove)
	{
		if (isKingInCheck(sideToMove))
		{
			std::cout << "Checkmate. " << ((sideToMove == white) ? "Black" : "White") << " wins." << '\n';
		}
		else
		{
			std::cout << "Stalemate. Game is a draw." << '\n';
		}
		return true;  // Game over
	}

	return false;  // Game continues
}

void gameLoop()
{
	// THIS LOOP DOESNT WORK

	initializeAllBoards();
	positionKey = computePositionKey();
	printMainboard();

	while (true)
	{
		if (isGameOver(white)) break;
		if (isGameOver(black)) break;

		/*int from{ 0 };
		int to{ 0 };
		int flag{ 0 };

		std::string fromString{ "" };
		std::string toString{ "" };
		std::string flagString{ "" };*/

		std::cout << "White's turn" << '\n';
		SearchResult resultWhite = negaMax(white, minScore, maxScore, depth, 0);
		makeMove(resultWhite.move, white, 0);
		printMainboard();
		std::cout << "Evaluation: " << std::fixed << std::setprecision(1) << (resultWhite.score / 100.0) << '\n';


		/*std::cout << "Enter the square of the piece you want to move (lowercase)" << '\n';
		std::cin >> fromString;
		from = stringToSquare(fromString);

		std::cout << "Enter the desired destination (lowercase, must be legal move)" << '\n';
		std::cin >> toString;
		to = stringToSquare(toString);

		std::cout << "Enter the INDEX (1-14) of the move type" << '\n';
		std::cout << "Flag list: " << "quiet_move, double_pawn_push, king_side_castle, queen_side_castle, capture, en_passant_capture, " << '\n' << "knight_promotion, bishop_promotion, rook_promotion, queen_promotion, knight_promo_capture, bishop_promo_capture, rook_promo_capture, queen_promo_capture";
		std::cout << '\n';
		std::cin >> flag;
		flag--;*/

		/*makeMove(encodeMove(from, to, flag), black, 0);
		printMainboard();*/

		std::cout << "Black's turn" << '\n';
		SearchResult resultBlack = negaMax(black, minScore, maxScore, depth, 0);
		makeMove(resultBlack.move, black, 0);
		printMainboard();
		std::cout << "Evaluation: " << std::fixed << std::setprecision(1) << (-resultBlack.score / 100.0) << '\n'; // negate score
	}
}

void uciLoop()
{
	std::string line;

	while (std::getline(std::cin, line))
	{
		std::istringstream iss(line);
		std::string token;
		iss >> token;

		// UCI initialization
		if (token == "uci")
		{
			std::cout << "id name ChessEngineTP" << "\n";
			std::cout << "id author ThanasisPantelakis" << "\n";
			std::cout << "uciok" << "\n";
		}

		// Ready check
		else if (token == "isready")
		{
			std::cout << "readyok" << "\n";
		}

		// New game
		else if (token == "ucinewgame")
		{
			initializeAllBoards();
			positionKey = computePositionKey();
			currentSideToMove = white;
			whiteMoveLog.clear();
			blackMoveLog.clear();
			whiteKingSideCastlingRights = true;
			whiteQueenSideCastlingRights = true;
			blackKingSideCastlingRights = true;
			blackQueenSideCastlingRights = true;
			plyCounter = 0;
		}

		// Set position
		else if (token == "position")
		{
			std::string posType;
			iss >> posType;

			if (posType == "startpos")
			{
				initializeAllBoards();
				positionKey = computePositionKey();
				currentSideToMove = white;
				whiteMoveLog.clear();
				blackMoveLog.clear();
				whiteKingSideCastlingRights = true;
				whiteQueenSideCastlingRights = true;
				blackKingSideCastlingRights = true;
				blackQueenSideCastlingRights = true;
				plyCounter = 0;

				// Check for moves
				std::string movesToken;
				if (iss >> movesToken && movesToken == "moves")
				{
					std::string moveStr;
					while (iss >> moveStr)
					{
						// Parse move
						int from = stringToSquare(moveStr.substr(0, 2));
						int to = stringToSquare(moveStr.substr(2, 2));

						// Generate moves and find the matching one
						std::array<Move, MAX_MOVES> moveList;
						int moveCount;
						getPseudoLegalMoves(currentSideToMove, moveList, moveCount);

						for (int i = 0; i < moveCount; i++)
						{
							if (getFrom(moveList[i]) == from && getTo(moveList[i]) == to)
							{
								// Check for promotion
								if (moveStr.length() == 5)
								{
									char promoChar = moveStr[4];
									int flag = getFlag(moveList[i]);

									// Match promotion type
									if (promoChar == 'q' && (flag != queen_promotion && flag != queen_promo_capture)) continue;
									if (promoChar == 'r' && (flag != rook_promotion && flag != rook_promo_capture)) continue;
									if (promoChar == 'b' && (flag != bishop_promotion && flag != bishop_promo_capture)) continue;
									if (promoChar == 'n' && (flag != knight_promotion && flag != knight_promo_capture)) continue;
								}

								makeMove(moveList[i], currentSideToMove, plyCounter);
								currentSideToMove = (currentSideToMove == white) ? black : white;
								break;
							}
						}
					}
				}
			}
		}

		// Search for best move
		else if (token == "go")
		{
			SearchResult result = negaMax(currentSideToMove, minScore, maxScore, depth, 0);
			if (result.move == 0)
			{
				std::cout << "bestmove (none)" << "\n";
				continue;
			}

			// Format move
			std::string moveStr = squareToString(getFrom(result.move)) + squareToString(getTo(result.move));

			// Add promotion piece if needed
			int flag = getFlag(result.move);
			if (flag >= knight_promotion && flag <= queen_promo_capture)
			{
				if (flag == queen_promotion || flag == queen_promo_capture) moveStr += "q";
				else if (flag == rook_promotion || flag == rook_promo_capture) moveStr += "r";
				else if (flag == bishop_promotion || flag == bishop_promo_capture) moveStr += "b";
				else if (flag == knight_promotion || flag == knight_promo_capture) moveStr += "n";
			}

			std::cout << "bestmove " << moveStr << "\n";
		}

		// Quit
		else if (token == "quit")
		{
			break;
		}
	}
}

int main()
{
	initializeZobrist();

	uciLoop();
	//gameLoop();

	return 0;
}