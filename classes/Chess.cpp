#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <iostream>

Chess::Chess()
{
    _grid = new Grid(8, 8);
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
            std::cout << "Placing piece:" << c << " at file="<< file << " and rank=" << rank << std::endl;

            int playerNumber = std::isupper(static_cast<unsigned char>(c)) ? 0 : 1;
            std::cout << "Player Number: " << playerNumber << std::endl;
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
                //_grid->getSquare(file, rank)->setBit(bit);
                file++;
            }

        }

    }


    // for(int i = 0; i < fen.length(); ++i) {
    //     int x = i % 8;
    //     int y = i / 8;
    //     char c = fen[i];
    //     std::cout << fen[i];
        
    //     if (c >= '1' && c <= '8') {
    //         int emptySquares = c - '0';
    //         for (int j = 0; j < emptySquares; ++j) {
    //             _grid->getSquare(x + j, y)->setBit(nullptr);
    //         }
    //         x += emptySquares - 1; // -1 because of the x++ in the for loop
            
    //     } else if (c == '/') {
    //         // new rank, do nothing
    //         continue;
    //     } 
    //     else {
    //         int playerNumber = std::isupper(static_cast<unsigned char>(c)) ? 0 : 1;
    //         ChessPiece piece;
    //         switch (std::tolower(static_cast<unsigned char>(c))) {
    //             case 'p': piece = Pawn; break;
    //             case 'n': piece = Knight; break;
    //             case 'b': piece = Bishop; break;
    //             case 'r': piece = Rook; break;
    //             case 'q': piece = Queen; break;
    //             case 'k': piece = King; break;
    //             default: piece = Pawn; // should not happen
    //         }
    //         Bit* bit = PieceForPlayer(playerNumber, piece);
    //         _grid->getSquare(x, y)->setBit(bit);
    //     }
    // }
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
    return true;
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
