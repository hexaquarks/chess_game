#include "../../include/Utilities/MoveList.hpp"
#include "../../include/Utilities/PieceTransition.hpp"
#include "../../include/GameThread.hpp"

MoveList::MoveList(Board& board_, PieceTransition& p_): game(board_), m_transitioningPiece(p_)
{
}

void MoveList::highlightLastMove(RenderWindow& window_) const
{
    if (!hasMovesBefore()) return;
    shared_ptr<Move> move = *m_moveIterator;

    RectangleShape squareBefore = createSquare();
    RectangleShape squareAfter = createSquare();

    Color colorInit = ((move->getInit().first+move->getInit().second) % 2)
                    ? Color(170, 162, 58): Color(205, 210, 106);
    Color colorTarget = ((move->getTarget().first+move->getTarget().second) % 2)
                    ? Color(170, 162, 58): Color(205, 210, 106);
    squareBefore.setFillColor(colorInit);
    squareAfter.setFillColor(colorTarget);

    squareBefore.setPosition(
        GameThread::getWindowXPos(GameThread::boardFlipped()? 7-move->getInit().first: move->getInit().first),
        GameThread::getWindowYPos(GameThread::boardFlipped()? 7-move->getInit().second: move->getInit().second)
    );
    squareAfter.setPosition(
        GameThread::getWindowXPos(GameThread::boardFlipped()? 7-move->getTarget().first: move->getTarget().first),
        GameThread::getWindowYPos(GameThread::boardFlipped()? 7-move->getTarget().second: move->getTarget().second)
    );

    window_.draw(squareBefore);
    window_.draw(squareAfter);
}

void MoveList::goToPreviousMove(bool enableTransition_, vector<Arrow>& arrowList_)
{
    if (hasMovesBefore())
    {
        undoMove(enableTransition_, arrowList_);
        ++m_moveIterator; // Go to previous move
        game.switchTurn();
        GameThread::refreshMoves();
    }
}

void MoveList::goToNextMove(bool enableTransition_, vector<Arrow>& arrowList_)
{
    if (hasMovesAfter())
    {
        --m_moveIterator; // Go to previous move
        applyMove(enableTransition_, arrowList_);
        game.switchTurn();
        GameThread::refreshMoves();
    }
}

void MoveList::addMove(shared_ptr<Move>& move_, vector<Arrow>& arrowList_)
{
    applyMove(move_, true, true, arrowList_);
    m_moveIterator = m_moves.begin();
}

void MoveList::applyMove(bool enableTransition_, vector<Arrow>& arrowList_)
{
    shared_ptr<Move> m = *m_moveIterator;
    applyMove(m, false, enableTransition_, arrowList_);
}

void MoveList::applyMove(shared_ptr<Move>& move_, bool addToList_, bool enableTransition_, vector<Arrow>& arrowList_)
{
    const int castleRow = (game.getTurn() == Team::WHITE)? 7: 0;
    shared_ptr<Piece> pOldPiece;
    shared_ptr<Piece> pSelectedPiece = move_->getSelectedPiece();
    shared_ptr<Piece> pCapturedPiece;
    int capturedX = -1, capturedY = -1;

    coor2d oldCoors;
    int prevX = move_->getInit().first;
    int prevY = move_->getInit().second;
    int x = move_->getTarget().first;
    int y = move_->getTarget().second;

    // Set the current tile of the piece null. Necessary for navigating back to current move through goToNextMove()
    game.resetBoardTile(prevX, prevY);
    arrowList_ = move_->getMoveArrows();
    // TODO smooth piece transition for castle
    switch (move_->getMoveType())
    {
        case MoveType::NORMAL:
            game.setBoardTile(x, y, pSelectedPiece);
            if (addToList_)
            {
                m_moves.emplace_front(move_);
            }
            // soundMove.play();
            break;

        case MoveType::CAPTURE:
            pCapturedPiece = move_->getCapturedPiece();
            capturedX = x;
            capturedY = y;
            pOldPiece = game.getBoardTile(x, y);
            game.setBoardTile(x, y, pSelectedPiece);
            if (addToList_)
            {
                m_moves.emplace_front(make_shared<Move>(*move_, pOldPiece));
            }
            // soundCapture.play();
            break;

        case MoveType::ENPASSANT:
            pCapturedPiece = move_->getCapturedPiece();
            capturedX = pCapturedPiece->getY();
            capturedY = pCapturedPiece->getX();
            oldCoors = {capturedX, capturedY};
            game.resetBoardTile(oldCoors.first, oldCoors.second);
            game.setBoardTile(x, y, pSelectedPiece);
            if (addToList_)
            {
                m_moves.emplace_front(make_shared<Move>(*move_, pCapturedPiece, oldCoors));
            }
            break;

        case MoveType::CASTLE_KINGSIDE:
            pOldPiece = game.getBoardTile(7, castleRow);
            game.resetBoardTile(7, castleRow);
            game.setBoardTile(5, castleRow, pOldPiece);
            game.setBoardTile(6, castleRow, pSelectedPiece);
            game.setBoardTile(x, y, pSelectedPiece);
            if (addToList_)
            {
                coor2d target = {6, castleRow};
                move_->setTarget(target);
                m_moves.emplace_front(make_shared<Move>(*move_, pOldPiece));
            }
            break;

        case MoveType::CASTLE_QUEENSIDE:
            pOldPiece = game.getBoardTile(0, castleRow);
            game.resetBoardTile(0, castleRow);
            game.setBoardTile(3, castleRow, pOldPiece);
            game.setBoardTile(2, castleRow, pSelectedPiece);
            game.setBoardTile(x, y, pSelectedPiece);
            if (addToList_)
            {
                coor2d target = {2, castleRow};
                move_->setTarget(target);
                m_moves.emplace_front(make_shared<Move>(*move_, pOldPiece));
            }
            break;

        case MoveType::INIT_SPECIAL:
            game.setBoardTile(x, y, pSelectedPiece);
            if (addToList_)
            {
                m_moves.emplace_front(move_);
            }
            break;

        case MoveType::NEWPIECE:
            pOldPiece = game.getBoardTile(x, y);
            shared_ptr<Piece> queen = make_shared<Queen>(pSelectedPiece->getTeam(), y, x);
            Piece::setLastMovedPiece(queen);
            game.setBoardTile(x, y, queen);
            game.addPiece(queen);
            if (addToList_)
            {
                m_moves.emplace_front(make_shared<Move>(*move_, pOldPiece));
            }
            break;
    }

    if (!addToList_ && pSelectedPiece)
    {
        if (enableTransition_)
        {
            // Enable piece visual transition
            GameThread::setTransitioningPiece(
                false, pSelectedPiece, prevX, prevY, x, y, pCapturedPiece,
                capturedX, capturedY, getTransitioningPiece()
            );
        }
    }
}

void MoveList::undoMove(bool enableTransition_, vector<Arrow>& arrowList_)
{
    shared_ptr<Move> m = *m_moveIterator;
    shared_ptr<Piece> pCaptured = m->getCapturedPiece();
    shared_ptr<Piece> pUndoPiece;
    int x = m->getTarget().first;
    int y = m->getTarget().second;
    int prevX = m->getInit().first;
    int prevY = m->getInit().second;
    int capturedX = -1, capturedY = -1;

    int castleRow = (m->getSelectedPiece()->getTeam() == Team::WHITE)? 7: 0;
    arrowList_ = m->getMoveArrows();
    // TODO smooth transition for castle
    switch (m->getMoveType())
    {
        case MoveType::NORMAL:
            game.resetBoardTile(x, y);
            break;
        case MoveType::CAPTURE:
            pUndoPiece = pCaptured;
            capturedX = x;
            capturedY = y;
            game.setBoardTile(x, y, pCaptured);
            break;
        case MoveType::ENPASSANT:
            pUndoPiece = pCaptured;
            capturedX = m->getSpecial().first;
            capturedY = m->getSpecial().second;
            game.resetBoardTile(x, y);
            game.setBoardTile(capturedX, capturedY, pCaptured);
            break;
        case MoveType::CASTLE_KINGSIDE:
            game.getKing()->setAsFirstMovement();
            game.resetBoardTile(5, castleRow);
            game.resetBoardTile(6, castleRow);
            game.setBoardTile(7, castleRow, pCaptured);
            break;
        case MoveType::CASTLE_QUEENSIDE:
            game.getKing()->setAsFirstMovement();
            game.resetBoardTile(3, castleRow);
            game.resetBoardTile(2, castleRow);
            game.setBoardTile(0, castleRow, pCaptured);
            break;
        case MoveType::INIT_SPECIAL:
            game.resetBoardTile(x, y);
            break;
        case MoveType::NEWPIECE:
            shared_ptr<Piece> pawn = make_shared<Pawn>(m->getSelectedPiece()->getTeam(), prevY, prevX);
            game.setBoardTile(x, y, pCaptured);
            game.setBoardTile(prevX, prevY, pawn);
    }

    shared_ptr<Piece> selected = m->getSelectedPiece();
    game.setBoardTile(prevX, prevY, selected);

    if (enableTransition_)
    {
        // Enable transition movement
        GameThread::setTransitioningPiece(
            true, selected, x, y, prevX, prevY, pUndoPiece,
            capturedX, capturedY, getTransitioningPiece()
        );
    }
}
