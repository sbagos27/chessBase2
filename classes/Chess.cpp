#include "Chess.h"
#include <limits>
#include <cmath>
#include "MagicBitboards.h"

Chess::Chess()
{
    _grid = new Grid(8, 8);
    for(int i=0; i<64; i++) {
        _knightBitboards[i] = generateKnightMoveBitboard(i);
    }
    initMagicBitboards();
    for(int i=0; i<128; i++) { _bitboardLookup[i] = 0; }

    _bitboardLookup['W'] = WHITE_PAWNS;
    _bitboardLookup['N'] = WHITE_KNIGHTS;
    _bitboardLookup['B'] = WHITE_BISHOPS;
    _bitboardLookup['R'] = WHITE_ROOKS;
    _bitboardLookup['Q'] = WHITE_QUEENS;
    _bitboardLookup['K'] = WHITE_KING;
    _bitboardLookup['p'] = BLACK_PAWNS;
    _bitboardLookup['n'] = BLACK_KNIGHTS;
    _bitboardLookup['b'] = BLACK_BISHOPS;
    _bitboardLookup['r'] = BLACK_ROOKS;
    _bitboardLookup['q'] = BLACK_QUEENS;
    _bitboardLookup['k'] = BLACK_KING;
    _bitboardLookup['0'] = EMPTY_SQUARES;
}

Chess::~Chess()
{
    cleanupMagicBitboards();
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    _currentPlayer = WHITE;
    _moves = generateAllMoves();
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
            square->setBit(nullptr);
    });

    std::istringstream fenStream(fen);
    std::string boardPart;
    std::getline(fenStream, boardPart, ' ');

    int row = 7;
    int col = 0;
    for (char ch : fen) {
        if (ch == '/') {
            row--;
            col = 0;
        } else if (isdigit(ch)) {
            col += ch - '0'; // skip empty squares
        } else {
            // convert ch to a piece
            ChessPiece piece = Pawn;
            switch (toupper(ch)) {
            case 'P':
                piece = Pawn;
                break;
            case 'N':
                piece = Knight;
                break;
            case 'B':
                piece = Bishop;
                break;
            case 'R':
                piece = Rook;
                break;
            case 'Q':
                piece = Queen;
                break;
            case 'K':
                piece = King;
                break;
            }
            Bit* bit = PieceForPlayer(isupper(ch) ? 0 : 1, piece);
            ChessSquare *square = _grid->getSquare(col, row);
            bit->setPosition(square->getPosition());
            bit->setParent(square);
            bit->setGameTag(isupper(ch) ? piece : (piece + 128));
            square->setBit(bit);
            col++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    bool ret = false;
    ChessSquare* square = (ChessSquare *)&src;
    if (square) {
        int squareIndex = square->getSquareIndex();
        for(auto move : _moves) {
            if (move.from == squareIndex) {
                ret = true;
                auto dest = _grid->getSquareByIndex(move.to);
                dest->setHighlighted(true);
            }
        }
    }
    return ret;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srdsquare = (ChessSquare *)&src;
    ChessSquare* square = (ChessSquare *)&dst;
    if (square) {
        int squareIndex = square->getSquareIndex();
        for(auto move : _moves) {
            if (move.to == squareIndex && move.from == srdsquare->getSquareIndex()) {
                return true;
            }
        }
    }
    return false;
}

void Chess::clearBoardHighlights() {
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
            square->setHighlighted(false);
    });
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {

    _currentPlayer = (_currentPlayer == WHITE ? BLACK : WHITE);
    _moves = generateAllMoves();
    clearBoardHighlights();
    endTurn();
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

void Chess::addPawnBitboardMovesToList(std::vector<BitMove>& moves, const BitBoard bitboard, const int shift) {
    if (bitboard.getData() == 0)
        return;
    bitboard.forEachBit([&](int toSquare) {
        int fromSquare = toSquare - shift;
        moves.emplace_back(fromSquare, toSquare, Pawn);
    });
}

void Chess::generatePawnMoveList(std::vector<BitMove>& moves, const BitBoard pawns, const BitBoard emptySquares, const BitBoard enemyPieces, char color) {
    if (pawns.getData() == 0)
        return;

    //ranks and files
    constexpr uint64_t NotAFile(0xFEFEFEFEFEFEFEFEULL); // A file mask
    constexpr uint64_t NotHFile(0x7F7F7F7F7F7F7F7FULL); // H file mask
    constexpr uint64_t Rank3(0x0000000000FF0000ULL); // Rank 3 mask
    constexpr uint64_t Rank6(0x0000FF0000000000ULL); // Rank 6 mask

    BitBoard demoRight(NotAFile);
    BitBoard demoLeft(NotHFile);

    BitBoard singleMoves = (color == WHITE) ? (pawns.getData() << 8) & emptySquares.getData() : (pawns.getData() >> 8) & emptySquares.getData();

    BitBoard doubleMoves = (color == WHITE) ? ((singleMoves.getData() & Rank3) << 8) & emptySquares.getData() : ((singleMoves.getData() & Rank6) >> 8) & emptySquares.getData();

    BitBoard capturesLeft = (color == WHITE) ? ((pawns.getData() & NotAFile) << 7) & enemyPieces.getData() : ((pawns.getData() & NotAFile) >> 9) & enemyPieces.getData();
    BitBoard capturesRight = (color == WHITE) ? ((pawns.getData() & NotHFile) << 9) & enemyPieces.getData() : ((pawns.getData() & NotHFile) >> 7) & enemyPieces.getData();

    int shiftForward = (color == WHITE) ? 8 : -8;
    int doubleShift = (color == WHITE) ? 16 : -16;
    int captureLeftShift = (color == WHITE) ? 7 : -9;
    int captureRightShift = (color == WHITE) ? 9 : -7;
    
    addPawnBitboardMovesToList(moves, singleMoves, shiftForward);

    addPawnBitboardMovesToList(moves, doubleMoves, doubleShift);

    addPawnBitboardMovesToList(moves, capturesLeft, captureLeftShift);
    addPawnBitboardMovesToList(moves, capturesRight, captureRightShift);
}

void Chess::addMoveIfValid(const char *state, std::vector<BitMove>& moves, int fromRow, int fromCol, int toRow, int toCol, ChessPiece piece) 
{
    if (toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) {
        moves.emplace_back(fromRow*8+fromCol, toRow*8+toCol, piece);
    }
}

void Chess::generatePawnMoves(const char *state, std::vector<BitMove>& moves, int row, int col, int colorAsInt) 
{
    const int direction = (colorAsInt == WHITE) ? 1 : -1;
    const int startRow = (colorAsInt == WHITE) ? 1 : 6;

    // one square 
    if (stateNotation(state, row + direction, col) == '0') {
        addMoveIfValid(state, moves, row, col, row + direction, col, Pawn);

        // two squares from start
        if (row == startRow && stateNotation( state, row + 2 * direction, col) == '0') {
            addMoveIfValid(state, moves, row, col, row + 2 * direction, col, Pawn);
        }
    }

    // captures
    for (int i = -1; i <= 1; i += 2) { // -1 for left, +1 for right
        if (col + i >= 0 && col + i < 8) {
            int oppositeColor = (colorAsInt == 0) ? 1 : -1;
            int pieceColor = stateNotation( state, row + direction, col + i) >= 'a' ? BLACK : WHITE;
            if (pieceColor == oppositeColor) {
                addMoveIfValid(state, moves, row, col, row + direction, col + i, Pawn);
            }
        }
    }
}

BitBoard Chess::generateKnightMoveBitboard(int square) {
    BitBoard bitboard = 0ULL;
    int rank = square / 8;
    int file = square % 8;
    
    // the L shape combos shown in class
    std::pair<int, int> knightOffsets[] = {
        { 2, 1 }, { 2, -1 }, { -2, 1 }, { -2, -1 },
        { 1, 2 }, { 1, -2 }, { -1, 2 }, { -1, -2 }
    };

    constexpr uint64_t oneBit = 1;
    for (auto [dr, df] : knightOffsets) {
        int r = rank + dr, f = file + df;
        if (r >= 0 && r < 8 && f >= 0 && f < 8) {
            bitboard |= oneBit << (r * 8 + f);
        }
    }
    
    return bitboard;
}

void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitBoard knightBoard, uint64_t occupancy) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(_knightBitboards[fromSquare].getData() & occupancy);
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, BitBoard kingBoard, uint64_t occupancy) {
    kingBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(KingAttacks[fromSquare] & occupancy);
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

std::vector<BitMove> Chess::generateAllMoves()
{
    std::vector<BitMove> moves;
    moves.reserve(32);
    std::string state = stateString();

    for (int i=0; i<e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for(int i = 0; i<64; i++) {
        int bitIndex = _bitboardLookup[state[i]];
        _bitboards[bitIndex] |= 1ULL << i;
        if (state[i] != '0') {
            _bitboards[OCCUPANCY] |= 1ULL << i; 
            _bitboards[isupper(state[i]) ? WHITE_ALL_PIECES : BLACK_ALL_PIECES] |= 1ULL << i;
        }
    }

    int bitIndex = _currentPlayer == WHITE ? WHITE_PAWNS : BLACK_PAWNS;
    int oppBitIndex = _currentPlayer == WHITE ? BLACK_PAWNS : WHITE_PAWNS;

    generateKnightMoves(moves, _bitboards[WHITE_KNIGHTS + bitIndex], ~_bitboards[OCCUPANCY].getData());
    generateKingMoves  (moves, _bitboards[WHITE_KING    + bitIndex], ~_bitboards[OCCUPANCY].getData());
    generatePawnMoveList(moves, _bitboards[WHITE_PAWNS  + bitIndex], ~_bitboards[OCCUPANCY].getData(), _bitboards[WHITE_ALL_PIECES + oppBitIndex].getData(), _currentPlayer);

    return moves;
}
