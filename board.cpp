#include "board.h"
#include "oalgo.h"

#include <QPainter>

Board::Board(int userC, QWidget *parent) :
    QWidget(parent),
    CELLSIZE(60), GAPSIZE(5), TOTALSIZE(8 * CELLSIZE + 9 * GAPSIZE), BARGIN(30),
    PIECEBARGIN(10),
    //penColor(153, 51, 0), bkgColor(153, 128, 0),
    penColor(0, 0, 0), bkgColor(220, 220, 220),
    cellColorA(170, 170, 170), cellColorB(85, 85, 85),
    pieceColorB(Qt::black), pieceColorW(Qt::white),
    mousePrevCell(-1, -1), mouseCell(-1, -1)
{
    algo = new oAlgo(userC, this);

    if (userC = BLACK)
        CAN_USER = CAN_BLACK;
    else
        CAN_USER = CAN_WHITE;
    setMinimumHeight(TOTALSIZE + BARGIN);
    setMinimumWidth(TOTALSIZE + BARGIN);
    setMouseTracking(true);

    memset(bdState, 0, sizeof(CellState) * 64);
    setPiece(BLACK, 3, 3);
    setPiece(WHITE, 3, 4);
    setPiece(BLACK, 4, 4);
    setPiece(WHITE, 4, 3);

}

Board::~Board()
{
    delete algo;
}

void Board::paintEvent(QPaintEvent *event)
{
    QPainter p(this);

    // down X, right Y
    int leftTopX = x0 = height() / 2 - TOTALSIZE / 2;
    int leftTopY = y0 = width() / 2 - TOTALSIZE / 2;

    // paint bkg
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(bkgColor));
    p.drawRect(leftTopX, leftTopY, TOTALSIZE, TOTALSIZE);

    // paint cells
    leftTopX += GAPSIZE, leftTopY += GAPSIZE;

    p.setPen(QPen(penColor));
    p.setBrush(QBrush(cellColorA));
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if ((i + j) % 2 == 0)
                p.drawRect(leftTopX + i * (GAPSIZE + CELLSIZE),
                           leftTopY + j * (GAPSIZE + CELLSIZE),
                           CELLSIZE, CELLSIZE);
    p.setBrush(QBrush(cellColorB));
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if ((i + j) % 2 == 1)
                p.drawRect(leftTopX + i * (GAPSIZE + CELLSIZE),
                           leftTopY + j * (GAPSIZE + CELLSIZE),
                           CELLSIZE, CELLSIZE);

    // paint pieces
    QPen penS(Qt::black);
    penS.setWidth(3);
    p.setPen(penS);

    for (int k = 0; k < 64; k++)
    {
        if (bdState[k] != IS_BLACK && bdState[k] != IS_WHITE) continue;
        int x = (k / 8) * (GAPSIZE + CELLSIZE), y = (k % 8) * (GAPSIZE + CELLSIZE);

        if (bdState[k] == IS_BLACK)
            p.setBrush(pieceColorB);
        else
            p.setBrush(pieceColorW);
        p.drawEllipse(x + PIECEBARGIN + leftTopX,
                      y + PIECEBARGIN + leftTopY,
                      CELLSIZE - PIECEBARGIN * 2,
                      CELLSIZE - PIECEBARGIN * 2);
    }

    // paint cursor
    int i = mouseCell.x(), j = mouseCell.y();
    if (i >= 0 && j >= 0)
    {
        p.setBrush(Qt::NoBrush);
        QPen penC(Qt::red);
        qDebug("%d", bdState[CELL(i, j)]);
        if ((bdState[CELL(i, j)] & IS_PIECE) == 0 &&
            (bdState[CELL(i, j)] & CAN_USER) != 0)
            penC.setColor(Qt::green);
        //else if (bdState[i * 8 + j] == IS_BLACK)
        //    penC.setColor(Qt::black);
        //else if (bdState[i * 8 + j] == IS_WHITE)
        //    penC.setColor(Qt::white);
        penC.setWidth(3);
        p.setPen(penC);
        p.drawRect(i * (GAPSIZE + CELLSIZE) + leftTopX,
                   j * (GAPSIZE + CELLSIZE) + leftTopY,
                   CELLSIZE, CELLSIZE);
    }
}

void Board::mouseMoveEvent(QMouseEvent *event)
{
    //qDebug("cap!");
    mousePrevCell = mouseCell;
    mouseCell = getMouseCell(event->pos());
    //qDebug("%d %d", mouseCell.x(), mouseCell.y());
    if (mouseCell != mousePrevCell)
        update();
}

void Board::setPiece(int clr, int r, int c)
{
    if (clr == BLACK)
        bdState[CELL(r, c)] = IS_BLACK;
    else
        bdState[CELL(r, c)] = IS_WHITE;
    algo->setPiece(clr, r, c);
    //refreshState();
    //emit decide(r, c);
}

QPoint Board::getMouseCell(QPoint pos)
{
    int x = pos.x(), y = pos.y();

    if (x <= x0 + GAPSIZE || x >= x0 + TOTALSIZE ||
        y <= y0 + GAPSIZE || y >= y0 + TOTALSIZE)
        return QPoint(-1, -1);
    x -= GAPSIZE + x0, y -= GAPSIZE + y0;
    if (x % (GAPSIZE + CELLSIZE) >= CELLSIZE ||
        y % (GAPSIZE + CELLSIZE) >= CELLSIZE)
        return QPoint(-1, -1);
    x = x / (GAPSIZE + CELLSIZE);
    y = y / (GAPSIZE + CELLSIZE);
    return QPoint(x, y);
}
