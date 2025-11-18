#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include <vector>

constexpr int pieceSize = 80;

// enum ChessPiece
// {
//     NoPiece,
//     Pawn,
//     Knight,
//     Bishop,
//     Rook,
//     Queen,
//     King
// };

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    Grid* _grid;

    //part I - Knight, King, Pawn
    BitboardElement _knightBitBoard[64];
    BitboardElement _kingBitBoard[64];
    BitboardElement _pawnBitBoard[64];

    //part II - Rook, Bishop, Queen
    BitboardElement _rookBitBoard[64];
    BitboardElement _bishopBitBoard[64];
    BitboardElement _queenBitBoard[64];

    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares);
    void generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares);
    void generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemyOccupied, bool isWhite);

    //part II
    void generateRookMoves(std::vector<BitMove>& moves, BitboardElement rookBoard, uint64_t emptySquares, uint64_t blackOccupied, uint64_t whiteOccupied);
    void generateBishopMoves(std::vector<BitMove>& moves, BitboardElement bishopBoard, uint64_t emptySquares);
    void generateQueenMoves(std::vector<BitMove>& moves, BitboardElement queenBoard, uint64_t emptySquares);

    void generateKingMovesBitboard(int square);

    void generatePawnMovesBitboard(int square); 

 

    void generateAllMoves();
    
    // Initialize move lookup tables (called once in constructor)
    void initializeKnightBitboards();
    void initializeKingBitboards();
    void initializePawnBitboards();
    // new pieces - Rook, Bishop, Queen
    void initializeRookBitboards();
    void initializeBishopBitboards();
    void initializeQueenBitboards();
    
    
    
    // Scan current board and create bitboards (called every turn)
    void getCurrentBoardState(BitboardElement& whiteKnights, BitboardElement& blackKnights,
                              BitboardElement& whiteKings, BitboardElement& blackKings,
                              BitboardElement& whitePawns, BitboardElement& blackPawns,
                              BitboardElement& whiteRooks, BitboardElement& blackRooks,
                              BitboardElement& whiteBishops, BitboardElement& blackBishops,
                              BitboardElement& whiteQueens, BitboardElement& blackQueens,
                              uint64_t& emptySquares, uint64_t& whiteOccupied, uint64_t& blackOccupied);
    
    std::vector<BitMove> _moves; 

};