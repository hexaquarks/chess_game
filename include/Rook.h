#pragma once
#include "Piece.h"


// Represents a rook
struct Rook : public Piece {
    Rook(Team, int, int); // Constructor
    moveTypes calcPossibleMoves(Piece*[8][8]) const override;
};