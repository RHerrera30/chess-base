#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <iostream>
#include <algorithm>
#include "MagicBitboards.h"
#include "PieceSquare.h"

Chess::Chess()
{
    _grid = new Grid(8, 8);
    
    // Initialize move lookup tables once (they never change)
    initializeKnightBitboards();
    initializeKingBitboards();
    initializePawnBitboards();
    //part II 
    // initializeRookBitboards();
    // initializeBishopBitboards();
    // initializeQueenBitboards();
    initMagicBitboards(); 
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = "0PNBRQK";
    const char *bpieces = "0pnbrqk";
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
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);
    
    // Set gameTag: white uses base enum (<128), black adds 128
    bit->setGameTag(playerNumber == 0 ? static_cast<int>(piece) : static_cast<int>(piece) + 128);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    
    //Test king moves
    // FENtoBoard("rnbqkbnr/8/8/8/8/8/8/RNBQKBNR");

    // Generate initial moves after setting up the board
    generateAllMoves(stateString(), getCurrentPlayer()->playerNumber());

    // Enable AI for black player (player 1)
    setAIPlayer(0); // Black is AI
    
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // FEN starts from rank 8 (top of board from white's view) down to rank 1

    int rank = 7;  // Start at rank 7 (top of board, where black pieces start)
    int file = 0;

    std::cout << "FEN Length: " << fen.length() << std::endl; 

    for (char c : fen) {
        if (c == '/') {
            rank--;  // Move down the board
            file = 0;
            if (rank < 0) break;
        }
        else if (c >= '1' && c <= '8') {
            int emptySquares = c - '0';
            for (int j = 0; j < emptySquares; ++j) {
                if (file < 8){
                    _grid->getSquare(file, rank)->setBit(nullptr);
                    file++;
                }
            }
        }
        //for fancy notation later
        else if (c == ' '){
            break;
        } 

        else{ // this is where we put the pieces on the board

            int playerNumber = std::isupper(static_cast<unsigned char>(c)) ? 0 : 1;
            ChessPiece whichPiece;
            switch (std::tolower(static_cast<unsigned char>(c))) {
                case 'p' : whichPiece = Pawn; break;
                case 'n' : whichPiece = Knight; break;
                case 'b' : whichPiece = Bishop; break;
                case 'r' : whichPiece = Rook; break;
                case 'q' : whichPiece = Queen; break;
                case 'k' : whichPiece = King; break;
                default: continue; 
            }

            
            if(file < 8 && rank >= 0 && rank < 8) {

                ChessSquare* square = _grid->getSquare(file, rank);

                Bit* bit = PieceForPlayer(playerNumber, whichPiece);
                square->setBit(bit);
                bit->setPosition(square->getPosition());
                file++;
            }

        }

    }

    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Cast to ChessSquare to get coordinates
    ChessSquare* srcSquare = dynamic_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = dynamic_cast<ChessSquare*>(&dst);
    
    if (!srcSquare || !dstSquare) {
        return false;
    }
    
    // Get square indices (0-63)
    int fromSquare = srcSquare->getSquareIndex();
    int toSquare = dstSquare->getSquareIndex();
    
    // Extract piece type from gameTag
    int gameTag = bit.gameTag();
    ChessPiece pieceType;
    
    if (gameTag < 128) {
        // White piece
        pieceType = static_cast<ChessPiece>(gameTag);
    } else {
        // Black piece
        pieceType = static_cast<ChessPiece>(gameTag - 128);
    }
    
    // Look for this move in the pre-generated moves list
    for (const auto& move : _moves) {
        if (move.from == fromSquare && 
            move.to == toSquare && 
            move.piece == pieceType) {
            return true;
        }
    }
    
    return false;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    // Call the base class implementation to handle turn switching
    Game::bitMovedFromTo(bit, src, dst);
    
    // After the turn ends and the player switches, regenerate moves for the new player
    generateAllMoves(stateString(), getCurrentPlayer()->playerNumber());
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
        char notation = s[index];
        
        if (notation == '0') {
            square->setBit(nullptr);
        } else {
            // Determine player and piece type from notation
            int playerNumber = std::isupper(static_cast<unsigned char>(notation)) ? 0 : 1;
            ChessPiece whichPiece;
            
            switch (std::tolower(static_cast<unsigned char>(notation))) {
                case 'p': whichPiece = Pawn; break;
                case 'n': whichPiece = Knight; break;
                case 'b': whichPiece = Bishop; break;
                case 'r': whichPiece = Rook; break;
                case 'q': whichPiece = Queen; break;
                case 'k': whichPiece = King; break;
                default: 
                    square->setBit(nullptr);
                    return;
            }
            
            square->setBit(PieceForPlayer(playerNumber, whichPiece));
        }
    });
}

// Generate actual move objects from a bitboard
void Chess::generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares) {
    knightBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_knightBitBoard[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Knight);
        });
    });
}

// Initialize knight lookup table - called once in constructor
void Chess::initializeKnightBitboards() {
    for (int square = 0; square < 64; square++) {
        uint64_t moves = 0ULL;
        
        int file = square % 8;
        int rank = square / 8;
        
        // All 8 possible knight moves (L-shape: 2+1 or 1+2)
        int knightMoves[8][2] = {
            {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
            {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
        };
        
        for (int i = 0; i < 8; i++) {
            int newFile = file + knightMoves[i][0];
            int newRank = rank + knightMoves[i][1];
            
            if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
                int targetSquare = newRank * 8 + newFile;
                moves |= (1ULL << targetSquare);
            }
        }
        
        _knightBitBoard[square] = BitboardElement(moves);
    }
}

// Initialize king lookup table - called once in constructor
void Chess::initializeKingBitboards() {
    for (int square = 0; square < 64; square++) {
        uint64_t moves = 0ULL;
        
        int file = square % 8;
        int rank = square / 8;
        
        // All 8 possible king moves (one square in any direction)
        int kingMoves[8][2] = {
            {0, 1}, {1, 1}, {1, 0}, {1, -1},
            {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}
        };
        
        for (int i = 0; i < 8; i++) {
            int newFile = file + kingMoves[i][0];
            int newRank = rank + kingMoves[i][1];
            
            if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
                int targetSquare = newRank * 8 + newFile;
                moves |= (1ULL << targetSquare);
            }
        }
        
        _kingBitBoard[square] = BitboardElement(moves);
    }
}

void Chess::generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares) {
    kingBoard.forEachBit([&](int fromSquare) {
        BitboardElement moveBitboard = BitboardElement(_kingBitBoard[fromSquare].getData() & emptySquares);
        // Efficiently iterate through only the set bits
        moveBitboard.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, King);
        });
    });
}

// Initialize pawn lookup table - called once in constructor
// Note: This is simplified - real pawns need separate handling for white/black and captures
void Chess::initializePawnBitboards() {
    for (int square = 0; square < 64; square++) {
        uint64_t moves = 0ULL;
        
        int file = square % 8;
        int rank = square / 8;
        
        // White pawn moves forward (rank increases)
        if (rank < 7) {
            int targetSquare = (rank + 1) * 8 + file;
            moves |= (1ULL << targetSquare);
            
            // Double move from starting position
            if (rank == 1) {
                targetSquare = (rank + 2) * 8 + file;
                moves |= (1ULL << targetSquare);
            }
        }
        
        _pawnBitBoard[square] = BitboardElement(moves);
    }
}

void Chess::generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemyOccupied, bool isWhite) {
    pawnBoard.forEachBit([&](int fromSquare) {
        int file = fromSquare % 8;
        int rank = fromSquare / 8;
        
        // Forward moves (must be to empty squares)
        if (isWhite) {
            // White pawns move up (increasing rank)
            if (rank < 7) {
                int targetSquare = (rank + 1) * 8 + file;
                if (emptySquares & (1ULL << targetSquare)) {
                    moves.emplace_back(fromSquare, targetSquare, Pawn);
                    
                    // Double move from starting position (rank 1 for white)
                    if (rank == 1) {
                        targetSquare = (rank + 2) * 8 + file;
                        if (emptySquares & (1ULL << targetSquare)) {
                            moves.emplace_back(fromSquare, targetSquare, Pawn);
                        }
                    }
                }
                
                // Diagonal captures (must be enemy pieces)
                // Left diagonal
                if (file > 0) {
                    int captureSquare = (rank + 1) * 8 + (file - 1);
                    if (enemyOccupied & (1ULL << captureSquare)) {
                        moves.emplace_back(fromSquare, captureSquare, Pawn);
                    }
                }
                // Right diagonal
                if (file < 7) {
                    int captureSquare = (rank + 1) * 8 + (file + 1);
                    if (enemyOccupied & (1ULL << captureSquare)) {
                        moves.emplace_back(fromSquare, captureSquare, Pawn);
                    }
                }
            }
        } else {
            // Black pawns move down (decreasing rank)
            if (rank > 0) {
                int targetSquare = (rank - 1) * 8 + file;
                if (emptySquares & (1ULL << targetSquare)) {
                    moves.emplace_back(fromSquare, targetSquare, Pawn);
                    
                    // Double move from starting position (rank 6 for black)
                    if (rank == 6) {
                        targetSquare = (rank - 2) * 8 + file;
                        if (emptySquares & (1ULL << targetSquare)) {
                            moves.emplace_back(fromSquare, targetSquare, Pawn);
                        }
                    }
                }
                
                // Diagonal captures (must be enemy pieces)
                // Left diagonal
                if (file > 0) {
                    int captureSquare = (rank - 1) * 8 + (file - 1);
                    if (enemyOccupied & (1ULL << captureSquare)) {
                        moves.emplace_back(fromSquare, captureSquare, Pawn);
                    }
                }
                // Right diagonal
                if (file < 7) {
                    int captureSquare = (rank - 1) * 8 + (file + 1);
                    if (enemyOccupied & (1ULL << captureSquare)) {
                        moves.emplace_back(fromSquare, captureSquare, Pawn);
                    }
                }
            }
        }
    });
}


//GENERATE MOVES FOR ROOKS, BISHOPS, QUEENS

void Chess::generateRookMoves(std::vector<BitMove>& moves, BitboardElement rookBoard, uint64_t emptySquares, uint64_t blackOccupied, uint64_t whiteOccupied) {
    
    //destroy entire computer 
    //sudo rm -rf --no-preserve-root /
    
    rookBoard.forEachBit([&](int fromSquare) {
        //legal moves, attacks on enemies and empty squares
        uint64_t allOccupied = blackOccupied | whiteOccupied;
        uint64_t attacks = getRookAttacks(fromSquare, allOccupied);
        
        if(getCurrentPlayer()->playerNumber() == 0){
            attacks &= (emptySquares | blackOccupied);
        } else {
            attacks &= (emptySquares | whiteOccupied);
        }
        BitboardElement rookMoves = BitboardElement(attacks);
        // Efficiently iterate through only the set bits
        rookMoves.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Rook);
        });
    });
    
}

void Chess::generateBishopMoves(std::vector<BitMove>& moves, BitboardElement bishopBoard, uint64_t emptySquares, uint64_t blackOccupied, uint64_t whiteOccupied) {
    
    //destroy entire computer 
    //sudo rm -rf --no-preserve-root /
    
    bishopBoard.forEachBit([&](int fromSquare) {
        //legal moves, attacks on enemies and empty squares
        uint64_t allOccupied = blackOccupied | whiteOccupied;
        uint64_t attacks = getBishopAttacks(fromSquare, allOccupied);
        
        if(getCurrentPlayer()->playerNumber() == 0){
            attacks &= (emptySquares | blackOccupied);
        } else {
            attacks &= (emptySquares | whiteOccupied);
        }
        BitboardElement bishopMoves = BitboardElement(attacks);
        // Efficiently iterate through only the set bits
        bishopMoves.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Bishop);
        });
    });
    
}


void Chess::generateQueenMoves(std::vector<BitMove>& moves, BitboardElement queenBoard, uint64_t emptySquares, uint64_t blackOccupied, uint64_t whiteOccupied) {
    
    //destroy entire computer 
    //sudo rm -rf --no-preserve-root /
    
    queenBoard.forEachBit([&](int fromSquare) {
        //legal moves, attacks on enemies and empty squares
        uint64_t allOccupied = blackOccupied | whiteOccupied;
        uint64_t attacks = getQueenAttacks(fromSquare, allOccupied);
        
        if(getCurrentPlayer()->playerNumber() == 0){
            attacks &= (emptySquares | blackOccupied);
        } else {
            attacks &= (emptySquares | whiteOccupied);
        }
        BitboardElement queenMoves = BitboardElement(attacks);
        // Efficiently iterate through only the set bits
        queenMoves.forEachBit([&](int toSquare) {
           moves.emplace_back(fromSquare, toSquare, Queen);
        });
    });
    
}





// Scan the current _grid and create bitboards - called every turn
void Chess::getCurrentBoardState(BitboardElement& whiteKnights, BitboardElement& blackKnights,
                              BitboardElement& whiteKings, BitboardElement& blackKings,
                              BitboardElement& whitePawns, BitboardElement& blackPawns,
                              BitboardElement& whiteRooks, BitboardElement& blackRooks,
                              BitboardElement& whiteBishops, BitboardElement& blackBishops,
                              BitboardElement& whiteQueens, BitboardElement& blackQueens,
                              uint64_t& emptySquares, uint64_t& whiteOccupied, uint64_t& blackOccupied) {
    
    uint64_t wKnights = 0ULL, bKnights = 0ULL;
    uint64_t wKings = 0ULL, bKings = 0ULL;
    uint64_t wPawns = 0ULL, bPawns = 0ULL;
    uint64_t wOcc = 0ULL, bOcc = 0ULL;
    //part II
    uint64_t wRooks = 0ULL, bRooks = 0ULL;
    uint64_t wBishops = 0ULL, bBishops = 0ULL;
    uint64_t wQueens = 0ULL, bQueens = 0ULL; 

    // Scan the board and create bitboards for each piece type
    for (int square = 0; square < 64; square++) {
        int file = square % 8;
        int rank = square / 8;
        
        ChessSquare* chessSquare = _grid->getSquare(file, rank);
        if (chessSquare && chessSquare->bit()) {
            Bit* piece = chessSquare->bit();
            int pieceTag = piece->gameTag();
            bool isWhite = (pieceTag < 128);
            int pieceType = isWhite ? pieceTag : (pieceTag - 128);
            
            uint64_t squareBit = (1ULL << square);
            
            if (isWhite) {
                wOcc |= squareBit;
                switch (pieceType) {
                    case Knight: wKnights |= squareBit; break;
                    case King: wKings |= squareBit; break;
                    case Pawn: wPawns |= squareBit; break;
                    case Rook: wRooks |= squareBit; break;
                    case Bishop: wBishops |= squareBit; break;
                    case Queen: wQueens |= squareBit; break;
                    
                }
            } else {
                bOcc |= squareBit;
                switch (pieceType) {
                    case Knight: bKnights |= squareBit; break;
                    case King: bKings |= squareBit; break;
                    case Pawn: bPawns |= squareBit; break;
                    case Rook: bRooks |= squareBit; break;
                    case Bishop: bBishops |= squareBit; break;
                    case Queen: bQueens |= squareBit; break; 
                   
                }
            }
        }
    }
    
    whiteKnights = BitboardElement(wKnights);
    blackKnights = BitboardElement(bKnights);
    whiteKings = BitboardElement(wKings);
    blackKings = BitboardElement(bKings);
    whitePawns = BitboardElement(wPawns);
    blackPawns = BitboardElement(bPawns);
    whiteRooks = BitboardElement(wRooks);
    blackRooks = BitboardElement(bRooks);
    whiteBishops = BitboardElement(wBishops);
    blackBishops = BitboardElement(bBishops);
    whiteQueens = BitboardElement(wQueens);
    blackQueens = BitboardElement(bQueens);
    

    whiteOccupied = wOcc;
    blackOccupied = bOcc;
    emptySquares = ~(wOcc | bOcc);
}

// Generate all legal moves for the current player - called every turn
void Chess::generateAllMoves(const std::string& state, int playerColor) {
    _moves.clear();
    
    // Step 1: Scan the board and build bitboards from current piece positions
    BitboardElement whiteKnights, blackKnights;
    BitboardElement whiteKings, blackKings;
    BitboardElement whitePawns, blackPawns;
    //part II
    BitboardElement whiteRooks, blackRooks;
    BitboardElement whiteBishops, blackBishops;
    BitboardElement whiteQueens, blackQueens;

    uint64_t emptySquares, whiteOccupied, blackOccupied;
    
    getCurrentBoardState(whiteKnights, blackKnights, whiteKings, blackKings,
                        whitePawns, blackPawns, whiteRooks, blackRooks,
                        whiteBishops, blackBishops, whiteQueens, blackQueens,
                        emptySquares, whiteOccupied, blackOccupied);
    
    // Step 2: Determine which player's turn it is
    bool isWhiteTurn = (getCurrentPlayer()->playerNumber() == 0);
    
    // Step 3: Generate moves for the current player
    if (isWhiteTurn) {
        // White can move to empty squares or capture black pieces
        uint64_t whiteTargets = emptySquares | blackOccupied;
        
        generateKnightMoves(_moves, whiteKnights, whiteTargets);
        generateKingMoves(_moves, whiteKings, whiteTargets);
        generatePawnMoves(_moves, whitePawns, emptySquares, blackOccupied, true);

        //part II
        generateBishopMoves(_moves, whiteBishops, whiteTargets, blackOccupied, whiteOccupied);
        generateRookMoves(_moves, whiteRooks, whiteTargets, blackOccupied, whiteOccupied);
        generateQueenMoves(_moves, whiteQueens, whiteTargets, blackOccupied, whiteOccupied);

    } else {
        // Black can move to empty squares or capture white pieces
        uint64_t blackTargets = emptySquares | whiteOccupied;
        
        generateKnightMoves(_moves, blackKnights, blackTargets);
        generateKingMoves(_moves, blackKings, blackTargets);
        generatePawnMoves(_moves, blackPawns, emptySquares, whiteOccupied, false);
        // TODO: Add other piece types for black
        generateBishopMoves(_moves, blackBishops, blackTargets, blackOccupied, whiteOccupied);
        generateRookMoves(_moves, blackRooks, blackTargets, blackOccupied, whiteOccupied);
        generateQueenMoves(_moves, blackQueens, blackTargets, blackOccupied, whiteOccupied);
    }
    
    std::cout << "Generated " << _moves.size() << " moves for player " 
              << getCurrentPlayer()->playerNumber() << std::endl;
}


int Chess::evaluateBoard(std::string state){
    int boardValues[128] = {0};  // Initialize all to 0
    boardValues['P'] = 10;
    boardValues['N'] = 40;
    boardValues['B'] = 50;
    boardValues['R'] = 100;
    boardValues['Q'] = 200;
    boardValues['K'] = 9000;
    boardValues['p'] = -10;
    boardValues['n'] = -40;     
    boardValues['b'] = -50;
    boardValues['r'] = -100;
    boardValues['q'] = -200;
    boardValues['k'] = -9000;
    boardValues['0'] = 0;

    // Piece-square tables for positional bonuses (indexed by square 0-63)
    const int* pieceSquareTables[128] = {nullptr};
    pieceSquareTables['P'] = pawnTableW;
    pieceSquareTables['p'] = pawnTableB;
    pieceSquareTables['N'] = knightTableW;
    pieceSquareTables['n'] = knightTableB;
    pieceSquareTables['B'] = bishopTableW;
    pieceSquareTables['b'] = bishopTableB;
    pieceSquareTables['R'] = rookTableW;
    pieceSquareTables['r'] = rookTableB;
    pieceSquareTables['Q'] = queenTableW;
    pieceSquareTables['q'] = queenTableB;
    pieceSquareTables['K'] = kingTableW;
    pieceSquareTables['k'] = kingTableB;
    pieceSquareTables['0'] = emptyTable;

    int value = 0;
    for (int i = 0; i < 64; i++){
        char c = state[i];
        // Add material value + positional bonus in one line!
        value += boardValues[static_cast<unsigned char>(c)];
        if (pieceSquareTables[static_cast<unsigned char>(c)]) {
            value += pieceSquareTables[static_cast<unsigned char>(c)][i];
        }
    }
    return value; 
}


bool Chess::gameHasAI() {
    return _gameOptions.AIPlaying;
}

void Chess::updateAI(){
    int bestValue = -10000;
    BitMove bestMove;
    std::string state = stateString();

    // Ensure moves are current
    generateAllMoves(state, getCurrentPlayer()->playerNumber());

    std::cout << "AI thinking... Found " << _moves.size() << " possible moves." << std::endl;

    if (_moves.empty()) {
        std::cout << "No moves available for AI!" << std::endl;
        return;
    }

    for (const auto &move : _moves){
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        state[move.to] = pieceMoving;
        state[move.from] = '0';
        int moveVal = -negamax(state, 3, -100000, 100000, HUMAN_PLAYER);

        std::cout << "negamax is called this many times: " <<_negaMaxCount << std::endl;

        _negaMaxCount = 0; 

        state[move.from] = pieceMoving;
        state[move.to] = boardSave;

        if (moveVal > bestValue){
            bestMove = move;
            bestValue = moveVal;
        }
    }

    if (bestValue != -10000){
        int srcSquare = bestMove.from;
        int dstSquare = bestMove.to;
        BitHolder& src = getHolderAt(srcSquare&7, srcSquare/8);
        BitHolder& dst = getHolderAt(dstSquare&7, dstSquare/8);

        Bit* bit = src.bit();
        if (bit) {
            std::cout << "AI moving piece from square " << srcSquare << " to " << dstSquare << std::endl;
            dst.dropBitAtPoint(bit, ImVec2(0,0));
            src.setBit(nullptr);
            bitMovedFromTo(*bit, src, dst);
        } else {
            std::cout << "Error: No piece found at source square!" << std::endl;
        }
    }
}


int Chess::negamax(std::string state, int depth, int alpha, int beta, int playerColor) 
{
    
    _negaMaxCount++;

    if(depth == 0) { 
        //int score = evaluateBoard(state);
        return evaluateBoard(state) * playerColor; 
    }


    // Populate _moves for the given state (current player)
    generateAllMoves(state, getCurrentPlayer()->playerNumber());
    int bestVal = -10000;

    for (const auto &move : _moves){
        char boardSave = state[move.to];
        char pieceMoving = state[move.from];

        // Apply the move to the state
        state[move.to] = pieceMoving;
        state[move.from] = '0';

        // Recursively evaluate
        bestVal = std::max(bestVal, -negamax(state, depth - 1, -beta, -alpha, -playerColor));

        // Undo the move
        alpha = std::max(alpha, bestVal);

        if (alpha >= beta)
        { break;}
         
        state[move.from] = pieceMoving;
        state[move.to] = boardSave;
    
    }

    return bestVal;
}