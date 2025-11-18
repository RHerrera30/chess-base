#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <iostream>
#include "MagicBitboards.h"

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
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "b_" : "w_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);
    
    // Set gameTag: piece type for white (player 0), piece type + 128 for black (player 1)
    bit->setGameTag(playerNumber == 0 ? piece + 128 : piece);

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
    generateAllMoves();
    
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)

    int rank = 0;
    int file = 0;

    std::cout << "FEN Length: " << fen.length() << std::endl; 

    for (char c : fen) {
        if (c == '/') {
            rank++;
            file = 0;
            if (rank >= 8 ) break;
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

            
            if(file < 8 && rank < 8) {

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
    generateAllMoves();
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
    

    whiteOccupied = wOcc;
    blackOccupied = bOcc;
    emptySquares = ~(wOcc | bOcc);
}

// Generate all legal moves for the current player - called every turn
void Chess::generateAllMoves() {
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
        //generateBishopMoves(_moves, whiteBishops, whiteTargets);
        generateRookMoves(_moves, whiteRooks, whiteTargets, blackOccupied, whiteOccupied);
        //generateQueenMoves(_moves, whiteQueens, whiteTargets);

    } else {
        // Black can move to empty squares or capture white pieces
        uint64_t blackTargets = emptySquares | whiteOccupied;
        
        generateKnightMoves(_moves, blackKnights, blackTargets);
        generateKingMoves(_moves, blackKings, blackTargets);
        generatePawnMoves(_moves, blackPawns, emptySquares, whiteOccupied, false);
        // TODO: Add other piece types for black
        //generateBishopMoves(_moves, blackBishops, blackTargets);
        generateRookMoves(_moves, blackRooks, blackTargets, blackOccupied, whiteOccupied);
        //generateQueenMoves(_moves, blackQueens, blackTargets);
    }
    
    std::cout << "Generated " << _moves.size() << " moves for player " 
              << getCurrentPlayer()->playerNumber() << std::endl;
}

