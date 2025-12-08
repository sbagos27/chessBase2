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
    for(int i=0; i<128; i++) { _bitboardLookup[i] = EMPTY_SQUARES; }

    _bitboardLookup['P'] = WHITE_PAWNS;
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
    if (_currentPlayer == BLACK) {
        updateAI();
    }
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

void Chess::generatePawnMoveList(
    std::vector<BitMove>& moves, 
    BitBoard pawns, 
    BitBoard emptySquares, 
    BitBoard enemyPieces, 
    char color)
{
    if (pawns.getData() == 0) return;

    constexpr uint64_t NotAFile = 0xFEFEFEFEFEFEFEFEULL; 
    constexpr uint64_t NotHFile = 0x7F7F7F7F7F7F7F7FULL;

    constexpr uint64_t Rank3 = 0x0000000000FF0000ULL; 
    constexpr uint64_t Rank6 = 0x0000FF0000000000ULL; 

    uint64_t p = pawns.getData();
    uint64_t empty = emptySquares.getData();
    uint64_t enemy = enemyPieces.getData();

    BitBoard single = (color == WHITE) ? BitBoard((p << 8) & empty)
                                       : BitBoard((p >> 8) & empty);

    BitBoard dbl = (color == WHITE)
        ? BitBoard(((single.getData() & Rank3) << 8) & empty)
        : BitBoard(((single.getData() & Rank6) >> 8) & empty);

    BitBoard capturesL = (color == WHITE)
        ? BitBoard(((p & NotAFile) << 7) & enemy)
        : BitBoard(((p & NotHFile) >> 9) & enemy);

    BitBoard capturesR = (color == WHITE)
        ? BitBoard(((p & NotHFile) << 9) & enemy)
        : BitBoard(((p & NotAFile) >> 7) & enemy);

    int fwdShift     = (color == WHITE) ? 8 : -8;
    int dblShift     = (color == WHITE) ? 16 : -16;
    int leftShift    = (color == WHITE) ? 7 : -9;
    int rightShift   = (color == WHITE) ? 9 : -7;

    addPawnBitboardMovesToList(moves, single,     fwdShift);
    addPawnBitboardMovesToList(moves, dbl,        dblShift);
    addPawnBitboardMovesToList(moves, capturesL,  leftShift);
    addPawnBitboardMovesToList(moves, capturesR,  rightShift);
}

void Chess::addMoveIfValid(
    const char *state,
    std::vector<BitMove>& moves,
    int fromRow, int fromCol,
    int toRow, int toCol,
    ChessPiece piece)
{
    if (toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8) return;

    moves.emplace_back(fromRow*8 + fromCol, toRow*8 + toCol, piece);
}

void Chess::generatePawnMoves(
    const char *state,
    std::vector<BitMove>& moves,
    int row, int col,
    int color)
{
    const int direction = (color == WHITE) ? 1 : -1;
    const int startRow  = (color == WHITE) ? 1 : 6;

    // forward 1
    if (stateNotation(state, row + direction, col) == '0') {
        addMoveIfValid(state, moves, row, col, row + direction, col, Pawn);

        // forward 2 from start
        if (row == startRow &&
            stateNotation(state, row + 2 * direction, col) == '0')
        {
            addMoveIfValid(state, moves,
                row, col,
                row + 2 * direction, col,
                Pawn);
        }
    }

    // captures
    for (int dx : {-1, +1}) {
        int nc = col + dx;
        if (nc < 0 || nc >= 8) continue;

        char target = stateNotation(state, row + direction, nc);
        if (target == '0') continue; // empty

        bool targetIsBlack = (target >= 'a');
        int targetColor = targetIsBlack ? BLACK : WHITE;

        if (targetColor != color)
            addMoveIfValid(state, moves, row, col, row + direction, nc, Pawn);
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

// get all moves from the position the knight(s) is in, and find all combos of the L
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
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitBoard bishopBoard, uint64_t occupancy, uint64_t friendlies) {
    bishopBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(getBishopAttacks(fromSquare, occupancy) & ~friendlies);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
}

void Chess::generateRookMoves(
    std::vector<BitMove>& moves,
    BitBoard rookBoard,
    uint64_t occupancy,
    uint64_t friendlies)
{
    rookBoard.forEachBit([&](int fromSquare) {
        BitBoard moveBitboard = BitBoard(getRookAttacks(fromSquare, occupancy) & ~friendlies);
        moveBitboard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
}

void Chess::generateQueenMoves(
    std::vector<BitMove>& moves,
    BitBoard queenBoard,
    uint64_t occupancy,
    uint64_t friendlies)
{
    queenBoard.forEachBit([&](int fromSquare) {

        uint64_t attacks =
            getRookAttacks(fromSquare, occupancy) |
            getBishopAttacks(fromSquare, occupancy);

        BitBoard moveBitboard = BitBoard(attacks & ~friendlies);

        moveBitboard.forEachBit([&](int toSquare) {
            moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
}

std::vector<BitMove> Chess::generateAllMoves(const std::string& state, int playerColor)
{
    std::vector<BitMove> moves;
    moves.reserve(32);

    // Build bitboards from state string
    for (int i=0; i<e_numBitboards; i++) {
        _bitboards[i] = 0;
    }

    for(int i = 0; i<64; i++) {
        int bitIndex = _bitboardLookup[ static_cast<unsigned char>(state[i]) ];
        _bitboards[bitIndex] |= 1ULL << i;
    }

    bool isWhite = (playerColor == WHITE);

    int knightIndex = isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS;
    int kingIndex   = isWhite ? WHITE_KING    : BLACK_KING;
    int pawnIndex   = isWhite ? WHITE_PAWNS   : BLACK_PAWNS;
    int bishopIndex = isWhite ? WHITE_BISHOPS : BLACK_BISHOPS;
    int enemyIndex  = isWhite ? BLACK_ALL_PIECES : WHITE_ALL_PIECES;
    int rookIndex   = isWhite ? WHITE_ROOKS   : BLACK_ROOKS;
    int queenIndex  = isWhite ? WHITE_QUEENS  : BLACK_QUEENS;

    _bitboards[OCCUPANCY] = 0;
    _bitboards[WHITE_ALL_PIECES] = 0;
    _bitboards[BLACK_ALL_PIECES] = 0;
    for (int sq = 0; sq < 64; ++sq) {
        char c = state[sq];
        if (c != '0') {
            _bitboards[OCCUPANCY] |= 1ULL << sq;
            if (isupper(static_cast<unsigned char>(c)))
                _bitboards[WHITE_ALL_PIECES] |= 1ULL << sq;
            else
                _bitboards[BLACK_ALL_PIECES] |= 1ULL << sq;
        }
    }

    uint64_t empty = ~_bitboards[OCCUPANCY].getData();

    // generate moves
    generateKnightMoves(moves, _bitboards[knightIndex], empty);
    generateKingMoves  (moves, _bitboards[kingIndex],  empty);
    generateBishopMoves(moves, _bitboards[bishopIndex],  _bitboards[OCCUPANCY].getData(), _bitboards[bishopIndex].getData());
    generatePawnMoveList(
        moves,
        _bitboards[pawnIndex],
        BitBoard(empty),
        _bitboards[enemyIndex],
        playerColor
    );
    generateRookMoves(moves, _bitboards[rookIndex], _bitboards[OCCUPANCY].getData(), _bitboards[rookIndex].getData());
    generateQueenMoves(moves, _bitboards[queenIndex], _bitboards[OCCUPANCY].getData(), _bitboards[queenIndex].getData());

    return moves;
}

std::vector<BitMove> Chess::generateAllMoves()
{
    std::vector<BitMove> moves;
    moves.reserve(32);

    // reset bitboards
    for (int i = 0; i < e_numBitboards; i++)
        _bitboards[i] = 0;

    std::string state = stateString();

    // load bitboards
    for (int sq = 0; sq < 64; sq++) {
        char c = state[sq];
        int idx = _bitboardLookup[c];
        _bitboards[idx] |= 1ULL << sq;

        if (c != '0') {
            _bitboards[OCCUPANCY] |= 1ULL << sq;
            if (isupper(c))
                _bitboards[WHITE_ALL_PIECES] |= 1ULL << sq;
            else
                _bitboards[BLACK_ALL_PIECES] |= 1ULL << sq;
        }
    }

    bool isWhite = (_currentPlayer == WHITE);

    int knightIndex = isWhite ? WHITE_KNIGHTS : BLACK_KNIGHTS;
    int kingIndex   = isWhite ? WHITE_KING    : BLACK_KING;
    int pawnIndex   = isWhite ? WHITE_PAWNS   : BLACK_PAWNS;
    int bishopIndex = isWhite ? WHITE_BISHOPS : BLACK_BISHOPS;
    int enemyIndex  = isWhite ? BLACK_ALL_PIECES : WHITE_ALL_PIECES;
    int rookIndex   = isWhite ? WHITE_ROOKS   : BLACK_ROOKS;
    int queenIndex  = isWhite ? WHITE_QUEENS  : BLACK_QUEENS;


    uint64_t empty = ~_bitboards[OCCUPANCY].getData();

    // generate moves
    generateKnightMoves(moves, _bitboards[knightIndex], empty);
    generateKingMoves  (moves, _bitboards[kingIndex],  empty);
    generateBishopMoves(moves, _bitboards[bishopIndex],  _bitboards[OCCUPANCY].getData(), _bitboards[bishopIndex].getData());
    generatePawnMoveList(
        moves, 
        _bitboards[pawnIndex], 
        BitBoard(empty), 
        _bitboards[enemyIndex], 
        _currentPlayer
    );
    generateRookMoves(moves, _bitboards[rookIndex],_bitboards[OCCUPANCY].getData(),_bitboards[rookIndex].getData());

    generateQueenMoves(moves, _bitboards[queenIndex], _bitboards[OCCUPANCY].getData(), _bitboards[queenIndex].getData());

    return moves;
}
static std::map<char, int> evaluateScores = {
    {'P', 100}, {'p', -100},    // Pawns
    {'N', 200}, {'n', -200},    // Knights
    {'B', 230}, {'b', -230},    // Bishops
    {'R', 400}, {'r', -400},    // Rooks
    {'Q', 900}, {'q', -900},    // Queens
    {'K', 2000}, {'k', -2000},  // Kings
    {'0', 0}                     // Empty squares
};


int Chess::evaluateBoard(const std::string& state) {
    int value = 0;
    for(char ch : state) {
        value += evaluateScores[ch];
    }
    return value;
}


int Chess::negamax(std::string& state, int depth, int alpha, int beta, int playerColor)
{
    _countMoves++;

    if (depth == 0) {
        return evaluateBoard(state) * playerColor;
    }

    auto newMoves = generateAllMoves(state, playerColor);

    int bestVal = negInfinite; 

    for(auto move : newMoves) {
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';

        bestVal = std::max(bestVal, -negamax(state, depth - 1, -beta, -alpha, -playerColor));

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;

        // Alpha-beta pruning
        alpha = std::max(alpha, bestVal);
        if (alpha >= beta) {
            break;  // Beta cutoff
        }
    }

    return bestVal;
}


void Chess::updateAI()
{
    int bestVal = negInfinite;
    BitMove bestMove;
    std::string state = stateString();
    _countMoves = 0;

    int playerColor = (_currentPlayer == WHITE) ? WHITE : BLACK;
    const int searchDepth = 5;

    for(auto move : _moves) {
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';

        int moveVal = -negamax(state, searchDepth - 1, -posInfinite, -negInfinite, -playerColor);

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;

        if (moveVal > bestVal) {
            bestMove = move;
            bestVal = moveVal;
        }
    }

    if(bestVal != negInfinite) {
        std::cout << "Moves checked: " << _countMoves << std::endl;
        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare & 7, srcSquare / 8);
        BitHolder& dst = getHolderAt(dstSquare & 7, dstSquare / 8);
        Bit* bit = src.bit();
        if (!bit) {
            std::cerr << "AI selected a move from an empty source square! Aborting AI move." << std::endl;
            return;
        }
        dst.dropBitAtPoint(bit, ImVec2(0, 0));
        src.setBit(nullptr);
        bitMovedFromTo(*bit, src, dst);
    }
}
