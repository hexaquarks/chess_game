#pragma once
#include "../Pieces/Piece.hpp"
#include "../Components/Board.hpp"
#include <vector>

typedef std::pair<int, int> coor2d;
constexpr uint16_t g_FPS = 60;

class PieceTransition
{
public:
    // Getters
    bool getIsTransitioning() const { return m_isTransitioning; }
    bool getHasArrived() const { return m_hasArrived; }
    shared_ptr<Piece> getPiece() const { return m_piece; }
    shared_ptr<Piece> getCapturedPiece() const { return m_captured; }
    coor2d getIncrement() const { return m_increment; }
    coor2d getDestination() const { return m_destination; }
    coor2d getCurrPos() const { return m_currPos; }
    int getCapturedX() const { return capturedXPos; }
    int getCapturedY() const { return capturedYPos; }
    double getPercentageLeft() const { return distance_to_dest / original_dest; }
    bool isUndo() const { return undoMove; }

    // Setters
    void setCurrPos(coor2d&&);
    void setDestination(coor2d&& dest) { m_destination = dest; }
    void resetTransitioningPiece() { m_piece.reset(); }
    void setTransitioningPiece(shared_ptr<Piece>& p) { m_piece = p; }
    void setIsTransitioning(bool b) { m_isTransitioning = b; }
    void setCapturedPiece(shared_ptr<Piece>&, int, int);
    void setIncrement();
    void setHasArrived(Board&);
    void setUndo(bool value_) { undoMove = value_; }

    // Utilities
    void move(Board&);

private:
    shared_ptr<Piece> m_piece;
    shared_ptr<Piece> m_captured;
    coor2d m_currPos = {0, 0};
    coor2d m_destination;
    coor2d m_increment;
    int capturedXPos = 0;
    int capturedYPos = 0;
    bool m_hasArrived = false;
    bool m_isTransitioning = false;
    bool m_xArrived = false;
    bool m_yArrived = false;
    double distance_to_dest = 0.0;
    double original_dest = 1.0;
    bool undoMove = true;

    void updateDistToDest();
    bool pieceIsInBounds();

};
