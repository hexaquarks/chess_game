#include "../../include/Utilities/MoveList.hpp"
#include "../../include/Utilities/PieceTransition.hpp"
#include "../../include/GameThread.hpp"

MoveList::MoveList(Board& board, PieceTransition& p): game(board), m_transitioningPiece(p) {}

void MoveList::highlightLastMove(RenderWindow& window) const {
    if (m_moves.empty()) return;
    Move& move = *m_moveIterator;

    RectangleShape squareBefore = createSquare();
    RectangleShape squareAfter = createSquare();

    Color colorInit = ((move.getInit().first+move.getInit().second) % 2)
                    ? Color(170, 162, 58): Color(205, 210, 106);
    Color colorTarget = ((move.getTarget().first+move.getTarget().second) % 2)
                    ? Color(170, 162, 58): Color(205, 210, 106);
    squareBefore.setFillColor(colorInit);
    squareAfter.setFillColor(colorTarget);

    squareBefore.setPosition(
        GameThread::getWindowXPos(GameThread::boardFlipped()? 7-move.getInit().first: move.getInit().first),
        GameThread::getWindowYPos(GameThread::boardFlipped()? 7-move.getInit().second: move.getInit().second)
    );
    squareAfter.setPosition(
        GameThread::getWindowXPos(GameThread::boardFlipped()? 7-move.getTarget().first: move.getTarget().first), 
        GameThread::getWindowYPos(GameThread::boardFlipped()? 7-move.getTarget().second: move.getTarget().second)
    );

    window.draw(squareBefore);
    window.draw(squareAfter);
}

void MoveList::goToPreviousMove(bool enableTransition, vector<Arrow>& arrowList) {
    if (hasMovesBefore()) {
        undoMove(enableTransition, arrowList);
        ++m_moveIterator; // Go to previous move
        game.switchTurn();
    }
}

void MoveList::goToNextMove(bool enableTransition, vector<Arrow>& arrowList) { 
    if (hasMovesAfter()) {
        --m_moveIterator; // Go to previous move
        applyMove(enableTransition, arrowList);
        game.switchTurn();
    }
}

void MoveList::addMove(Move& move, vector<Arrow>& arrowList) {
    applyMove(move, true, true, arrowList);
    m_moveIterator = m_moves.begin();
}

void MoveList::applyMove(bool enableTransition, vector<Arrow>& arrowList) {
    Move& m = *m_moveIterator;
    applyMove(m, false, enableTransition, arrowList);
}

void MoveList::applyMove(Move& move, bool addToList, bool enableTransition, vector<Arrow>& arrowList) {
    const int castleRow = (game.getTurn() == Team::WHITE)? 7: 0;
    Piece* oldPiece = nullptr;
    Piece* selectedPiece = move.getSelectedPiece();
    Piece* capturedPiece = move.getCapturedPiece();

    coor2d oldCoors;
    int prevX = move.getInit().first;
    int prevY = move.getInit().second;
    int x = move.getTarget().first;
    int y = move.getTarget().second;

    // Set the current tile of the piece null. Necessary for navigating back to current move through goToNextMove()
    game.setBoardTile(prevX, prevY, nullptr); 
    arrowList = move.getMoveArrows();
    // TODO smooth piece transition for castle 
    switch (move.getMoveType()) {
        case MoveType::NORMAL:
            if (addToList) {
                m_moves.emplace_front(move);
                game.setBoardTile(x, y, selectedPiece);
            }
            // soundMove.play();
            break;

        case MoveType::CAPTURE:
            oldPiece = game.getBoardTile(x, y);
            if (addToList) {
                game.setBoardTile(x, y, selectedPiece);
                m_moves.emplace_front(Move(move, oldPiece));
            }
            // soundCapture.play();
            break;

        case MoveType::ENPASSANT:
            oldCoors = {capturedPiece->getY(), capturedPiece->getX()};
            game.setBoardTile(oldCoors.first, oldCoors.second, nullptr);
            if (addToList) {
                game.setBoardTile(x, y, selectedPiece);
                m_moves.emplace_front(Move(move, capturedPiece, oldCoors));
            }
            break;

        case MoveType::CASTLE_KINGSIDE:
            oldPiece = game.getBoardTile(7, castleRow);
            game.setBoardTile(7, castleRow, nullptr);
            game.setBoardTile(5, castleRow, oldPiece);
            game.setBoardTile(6, castleRow, selectedPiece);
            if (addToList) {
                coor2d target = make_pair(6, castleRow);
                move.setTarget(target);
                m_moves.emplace_front(Move(move, oldPiece));
            }
            break;

        case MoveType::CASTLE_QUEENSIDE:
            oldPiece = game.getBoardTile(0, castleRow);
            game.setBoardTile(0, castleRow, nullptr);
            game.setBoardTile(3, castleRow, oldPiece);
            game.setBoardTile(2, castleRow, selectedPiece);
            if (addToList) {
                coor2d target = make_pair(2, castleRow);
                move.setTarget(target);
                m_moves.emplace_front(Move(move, oldPiece));
            }
            break;

        case MoveType::INIT_SPECIAL:
            if (addToList) {
                game.setBoardTile(x, y, selectedPiece);
                m_moves.emplace_front(move);
            }
            break;

        case MoveType::NEWPIECE:
            // Possible leaking memory here actually ? 
            oldPiece = game.getBoardTile(x, y);
            Queen* queen = new Queen(selectedPiece->getTeam(), y, x);
            Piece::setLastMovedPiece(queen);
            game.setBoardTile(x, y, queen);
            if (addToList) {
                m_moves.emplace_front(Move(move, oldPiece));
            }
            break;
    }

    if (!addToList && selectedPiece != nullptr) {
        if (enableTransition) {
            // move the piece from the (-1, -1) hidden location back to the square 
            // where it begins it's transition
            selectedPiece->move(prevY, prevX);

            // enable piece visual transition
            GameThread::setTransitioningPiece(selectedPiece,
                x*CELL_SIZE, y*CELL_SIZE, getTransitioningPiece()
            ); 
        } else {
            game.setBoardTile(x, y, selectedPiece);
        }
    }
}

void MoveList::undoMove(bool enableTransition, vector<Arrow>& arrowList) {
    Move& m = *m_moveIterator;
    Piece* captured = m.getCapturedPiece();
    int x = m.getTarget().first;
    int y = m.getTarget().second;
    int prevX = m.getInit().first;
    int prevY = m.getInit().second;

    int castleRow = (m.getSelectedPiece()->getTeam() == Team::WHITE)? 7: 0;
    arrowList = m.getMoveArrows();
    // TODO smooth transition for castle 
    switch (m.getMoveType()) {
        case MoveType::NORMAL:
            game.setBoardTile(x, y, nullptr);
            break;
        case MoveType::CAPTURE:
            game.setBoardTile(x, y, captured);
            break;
        case MoveType::ENPASSANT:
            game.setBoardTile(x, y, nullptr);
            game.setBoardTile(m.getSpecial().first, m.getSpecial().second, captured);
            break;
        case MoveType::CASTLE_KINGSIDE:
            game.setBoardTile(5, castleRow, nullptr);
            game.setBoardTile(6, castleRow, nullptr);
            game.setBoardTile(7, castleRow, captured);
            break;
        case MoveType::CASTLE_QUEENSIDE:
            game.setBoardTile(3, castleRow, nullptr);
            game.setBoardTile(2, castleRow, nullptr);
            game.setBoardTile(0, castleRow, captured);
            break;
        case MoveType::INIT_SPECIAL:
            game.setBoardTile(x, y, nullptr);
            break;
        case MoveType::NEWPIECE:
            Piece* queen = game.getBoardTile(x, y);
            game.setBoardTile(x, y, captured);
            delete queen;
    }

    if (enableTransition) {
        // move the piece from the (-1,-1) hidden location back to the square 
        // where it begins it's transition
        m.getSelectedPiece()->move(y, x); 

        // Enable transition movement
        GameThread::setTransitioningPiece(
            m.getSelectedPiece(), m.getInit().first * CELL_SIZE, 
            m.getInit().second * CELL_SIZE, getTransitioningPiece()
        );
    } else {
        game.setBoardTile(m.getInit().first, m.getInit().second, m.getSelectedPiece());
    }
}