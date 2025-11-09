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

    
    BitboardElement _knightBitBoard[64];

    BitboardElement _kingBitBoard[64];

    BitboardElement _pawnBitBoard[64];

    void generateKnightMoves(std::vector<BitMove>& moves, BitboardElement knightBoard, uint64_t emptySquares);

    void generateKingMoves(std::vector<BitMove>& moves, BitboardElement kingBoard, uint64_t emptySquares);

    void generatePawnMoves(std::vector<BitMove>& moves, BitboardElement pawnBoard, uint64_t emptySquares, uint64_t enemyOccupied, bool isWhite);

    void generateKingMovesBitboard(int square);

    void generatePawnMovesBitboard(int square); 

    void generateAllMoves();
    
    // Initialize move lookup tables (called once in constructor)
    void initializeKnightBitboards();
    void initializeKingBitboards();
    void initializePawnBitboards();
    
    // Scan current board and create bitboards (called every turn)
    void getCurrentBoardState(BitboardElement& whiteKnights, BitboardElement& blackKnights,
                              BitboardElement& whiteKings, BitboardElement& blackKings,
                              BitboardElement& whitePawns, BitboardElement& blackPawns,
                              uint64_t& emptySquares, uint64_t& whiteOccupied, uint64_t& blackOccupied);
    
    std::vector<BitMove> _moves; 

};