#include "board.h"
#include "oalgo.h"
#include "choosecolor.h"

#include <QPainter>
#include <QDebug>
#include <QImageReader>
#include <QApplication>
#include <QPushButton>
#include <QMessageBox>

CellLabel::CellLabel(Board *bd, QWidget *parent) :
    QLabel(parent)
{
    father = bd;
    setMouseTracking(true);
}

void CellLabel::mouseMoveEvent(QMouseEvent *event)
{
    QApplication::sendEvent(father, event);
}

void CellLabel::mousePressEvent(QMouseEvent *event)
{
    QApplication::sendEvent(father, event);
}

void CellLabel::mouseReleaseEvent(QMouseEvent *event)
{
    QApplication::sendEvent(father, event);
}

Board::Board(int server, QWidget *parent) :
    QWidget(parent),
    CELLSIZE(55), GAPSIZE(1), x0(183), y0(144), PIECEOFFSET(4), TOTALSIZE(8 * 55 + 9 * 1),
    isServer(server), onConnection(0),
    userColor(-1), currentColor(BLACK),
    needHint(0), needPlay(0),
    penColor(0, 0, 0), bkgColor(220, 220, 220),
    cellColorA(170, 170, 170), cellColorB(85, 85, 85),
    pieceColorB(Qt::black), pieceColorW(Qt::white),
    mousePrevCell(-1, -1), mouseCell(-1, -1), mouseEnable(1),
    scoreAX0(103), scoreAY0(482), scoreBX0(753), scoreBY0(482), scoreH(15), scoreW(50),
    infoAX0(92), infoAY0(305), infoBX0(739), infoBY0(305),
    meReady(0), heReady(0), inGame(0), inHint(0), inMovie(0)
{
    initResources();
    initLabels();
    initInfo();

    algo = new OAlgo(this);

    setMinimumSize(uiWindow.size());
    setMaximumSize(uiWindow.size());
    setMouseTracking(true);

    startButton->setVisible(false);
    surrenderButton->setVisible(false);
    hintButton->setVisible(false);

    connect(startButton, SIGNAL(clicked()), this, SLOT(playerReady()));
    connect(surrenderButton, SIGNAL(clicked()), this, SLOT(surrender()));
    connect(hintButton, SIGNAL(clicked()), this, SLOT(hintPieces()));

    hintTimer = new QTimer(this);
    movieTimer = new QTimer(this);

    connect(hintTimer, SIGNAL(timeout()), this, SLOT(hintStop()));
    connect(movieTimer, SIGNAL(timeout()), this, SLOT(movieStop()));

    qDebug("ready connected");

    memset(bdState, 0, sizeof(CellState) * 64);

    paintPieces(0, 0);
    paintScore();
    update();
}

void Board::gameEstab()
{
    onConnection = 1;
    roleA->setPixmap(QPixmap("://ui/role.jpg"));
    startButton->setVisible(true);
}

void Board::gamePrepare()
{
    qDebug("we'are preparing");
    heReady = 0;
    meReady = 0;
    if (isServer)
    {
        ChooseColor dlg(this);
        userColor = dlg.exec();
        qDebug("%d", userColor);
        if (userColor == BLACK)
            emit decide(GAME_INFO, WHITE);
        else
            emit decide(GAME_INFO, BLACK);
        gameStart();
    }
}

void Board::gameStart()
{
    inGame = 1;
    memset(bdState, 0, sizeof(CellState) * 64);
    paintPieces(0, 0);
    paintScore();
    update();

    restA->restart(0);
    restB->restart(0);
    totalA->restart(3600);
    totalB->restart(3600);

    if (userColor == currentColor)
        totalA->pause();
    else
        totalB->pause();

    qDebug() << "start...";
    algo->reStart();
    algo->setUserC(userColor);
    currentColor = BLACK;

    algo->setPiece(BLACK, 3, 3);
    algo->setPiece(WHITE, 3, 4);
    algo->setPiece(BLACK, 4, 4);
    algo->setPiece(WHITE, 4, 3);

    surrenderButton->setVisible(true);
    hintButton->setVisible(true);
    qDebug() << "set...";
    paintCurrentColor();
    paintPieces(0, 0);
    paintScore();
    update();
}

void Board::surrender()
{
    gameEnd(GAME_SURRENDER);
    emit decide(GAME_INFO, GAME_SURRENDER);
}

void Board::gameEnd(int msg)
{
    switch (msg)
    {
    case GAME_WIN:
    {
        gameMsg("恭喜您获胜了！");
        break;
    }
    case GAME_LOSE:
    {
        gameMsg("别灰心，何妨再来一局");
        break;
    }
    case GAME_TIE:
    {
        gameMsg("棋逢对手，将遇良才，平局");
        break;
    }
    case GAME_SURRENDER:
    {
        gameMsg("您投降了");
        break;
    }
    }

    currentColor = -1;
    inGame = 0;
    startButton->setVisible(true);
    surrenderButton->setVisible(false);
    hintButton->setVisible(false);
    //emit gameEnded(msg);
}

void Board::playerReady()
{
    meReady = 1;
    emit decide(GAME_INFO, GAME_READY);
    qDebug("send ready");
    if (heReady)
        gamePrepare();
    startButton->setVisible(false);
}

Board::~Board()
{
    delete algo;
}

void Board::initInfo()
{
    scoreLabelA = new QLabel(this);
    scoreLabelB = new QLabel(this);

    QPalette pal;
    pal.setColor(QPalette::Background, QColor(0x00, 0xff, 0x00, 0x00));
    pal.setColor(QPalette::WindowText, QColor(216, 156, 48));

    QFont font;
    font.setFamily("arial");
    font.setBold(true);
    font.setPointSize(15);
    scoreLabelA->setGeometry(scoreAX0, scoreAY0, scoreW, scoreH);
    scoreLabelB->setGeometry(scoreBX0, scoreBY0, scoreW, scoreH);
    scoreLabelA->setText("");
    scoreLabelB->setText("");
    scoreLabelA->setPalette(pal);
    scoreLabelB->setPalette(pal);
    scoreLabelA->setFont(font);
    scoreLabelB->setFont(font);

    QPixmap icoPm("://ui/start.png");
    QIcon ico(icoPm);
    startButton = new QPushButton(ico, "", this);
    startButton->setFlat(true);
    startButton->setGeometry(356, 660, 94, 36);
    startButton->setIcon(ico);
    startButton->setIconSize(icoPm.size());

    QPixmap icoP1("://ui/surrender.jpg");
    QIcon ico1(icoP1);
    surrenderButton = new QPushButton(ico1, "", this);
    surrenderButton->setFlat(true);
    surrenderButton->setGeometry(283, 660, 93, 36);
    surrenderButton->setIcon(ico1);
    surrenderButton->setIconSize(icoP1.size());

    QPixmap icoP2("://ui/hint.jpg");
    QIcon ico2(icoP2);
    hintButton = new QPushButton(ico2, "", this);
    hintButton->setFlat(true);
    hintButton->setGeometry(429, 660, 93, 36);
    hintButton->setIcon(ico2);
    hintButton->setIconSize(icoP2.size());

    roleA = new QLabel(this);
    roleB = new QLabel(this);
    roleA->setGeometry(24, 194, 116, 161);
    roleB->setGeometry(672, 194, 116, 161);
    roleB->setPixmap(QPixmap("://ui/role.jpg"));

    pieceInfoA = new QLabel(this);
    pieceInfoB = new QLabel(this);
    pieceInfoA->setGeometry(infoAX0, infoAY0, 46, 46);
    pieceInfoB->setGeometry(infoBX0, infoBY0, 46, 46);

    rTA = new QLabel(this); rTA->setGeometry(78, 427, 60, 17);
    rTA->setPalette(pal); rTA->setFont(font);
    rTB = new QLabel(this); rTB->setGeometry(729, 427, 60, 17);
    rTB->setPalette(pal); rTB->setFont(font);
    tTA = new QLabel(this); tTA->setGeometry(78, 455, 60, 17);
    tTA->setPalette(pal); tTA->setFont(font);
    tTB = new QLabel(this); tTB->setGeometry(729, 455, 60, 17);
    tTB->setPalette(pal); tTB->setFont(font);

    restA = new ExTimer(0, rTA, this);
    restB = new ExTimer(0, rTB, this);
    totalA = new ExTimer(1, tTA, this);
    totalB = new ExTimer(1, tTB, this);

    connect(restB, SIGNAL(bomb()), this, SLOT(userTimeOut()));
}

void Board::initResources()
{
    QImageReader reader;
    reader.setFileName("://ui/baiqi.tga");
    uiPieceW = reader.read();
    reader.setFileName("://ui/heiqi.tga");
    uiPieceB = reader.read();
    reader.setFileName("://ui/window.jpg");
    uiWindow = reader.read();
}

void Board::initLabels()
{
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            //cellArray[CELL(i, j)] = new QLabel(this);
            cellArray[CELL(i, j)] = new CellLabel(this, this);
            cellArray[CELL(i, j)]->resize(CELLSIZE - 2 * PIECEOFFSET,
                                          CELLSIZE - 2 * PIECEOFFSET);
            cellArray[CELL(i, j)]->move(x0 + GAPSIZE + i * (GAPSIZE + CELLSIZE) + PIECEOFFSET,
                                        y0 + GAPSIZE + j * (GAPSIZE + CELLSIZE) + PIECEOFFSET);
            cellArray[CELL(i, j)]->setAlignment(Qt::AlignCenter);
            cellArray[CELL(i, j)]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
}

void Board::paintScore()
{
    int countA = 0, countB = 0; // B for user
    for (int k = 0; k < 64; k++)
        if (othello::sameClr(bdState[k], userColor))
            countB++;
        else if (othello::antiClr(bdState[k], userColor))
            countA++;
    char tmp[10];
    sprintf(tmp, "%d", countA);
    scoreLabelA->setText(tmp);
    sprintf(tmp, "%d", countB);
    scoreLabelB->setText(tmp);
}

void Board::paintPieces(int hint = 0, int play = 0)
{
    //qDebug() << "hello";
    for (int k = 0; k < 64; k++)
        if (othello::sameClr(bdState[k], BLACK))
            cellArray[k]->setPixmap(QPixmap::fromImage(uiPieceB));
        else if (othello::sameClr(bdState[k], WHITE))
            cellArray[k]->setPixmap(QPixmap::fromImage(uiPieceW));
        else
            cellArray[k]->clear();
}

void Board::paintCurrentColor()
{
    qDebug("try paint demo %d %d", userColor, currentColor);
    if (userColor == currentColor)
    {
        if (userColor == BLACK)
            pieceInfoB->setPixmap(QPixmap::fromImage(uiPieceB));
        else
            pieceInfoB->setPixmap(QPixmap::fromImage(uiPieceW));
        pieceInfoA->clear();
    }
    else
    {
        if (userColor == BLACK)
            pieceInfoA->setPixmap(QPixmap::fromImage(uiPieceW));
        else
            pieceInfoA->setPixmap(QPixmap::fromImage(uiPieceB));
        pieceInfoB->clear();
    }
}

void Board::hintPieces()
{
    if (currentColor != userColor) return;
    hintTimer->start(1500);
    hintV.clear();
    for (int i = 0; i < 64; i++)
        if (othello::canPut(bdState[i], userColor))
        {
            //QPixmap pm = QPixmap::fromImage(userColor == BLACK ? uiPieceB : uiPieceW);
            QImage img(userColor == BLACK ? uiPieceB : uiPieceW);
            QImage alphaPic(img.size(), QImage::Format_ARGB32);
            alphaPic.fill(QColor(100, 100, 100, 40));
            img.setAlphaChannel(alphaPic);
            cellArray[i]->setPixmap(QPixmap::fromImage(img));
            hintV.push_back(i);
        }
    inHint = 1;
}

void Board::hintStop()
{
    if (!inHint) return;
    inHint = 0;
    for (int i = 0; i < hintV.size(); i++)
    {
        cellArray[hintV[i]]->clear();
    }
}

void Board::moviePlay()
{

    inMovie = 1;
}

void Board::movieStop()
{
    if (!inMovie) return;
    inMovie = 0;

}

void Board::paintEvent(QPaintEvent *)
{
    //qDebug() << "hi";
    // only paint board & cursor
    QPainter p(this);

    p.drawImage(0, 0, uiWindow);


    int xx = x0 + GAPSIZE, yy = y0 + GAPSIZE;
    int i = mouseCell.x(), j = mouseCell.y();
    if (i >= 0 && j >= 0)
    {
        p.setBrush(Qt::NoBrush);
        QPen penC(Qt::red);
        if ((bdState[CELL(i, j)] & IS_PIECE) == 0 &&
                othello::canPut(bdState[CELL(i, j)], currentColor))
            penC.setColor(Qt::green);
        if (userColor != currentColor)
            penC.setColor(Qt::black);
        penC.setWidth(3);
        p.setPen(penC);
        p.drawRect(i * (GAPSIZE + CELLSIZE) + xx,
                   j * (GAPSIZE + CELLSIZE) + yy,
                   CELLSIZE, CELLSIZE);
    }
}

void Board::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = mapFromGlobal(event->globalPos());
    if (!mouseEnable) return;
    //qDebug("cap!");
    mousePrevCell = mouseCell;
    mouseCell = getMouseCell(pos);
    //qDebug("%d %d", mouseCell.x(), mouseCell.y());
    if (mouseCell != mousePrevCell)
        update();
}

void Board::mousePressEvent(QMouseEvent *event)
{
    if (userColor != currentColor) return;
    QPoint pos = mapFromGlobal(event->globalPos());
    mousePressPos = pos;
    mousePrevCell = mouseCell;
    mouseCell = getMouseCell(pos);
}

void Board::mouseReleaseEvent(QMouseEvent *event)
{
    if (userColor != currentColor) return;
    QPoint pos = mapFromGlobal(event->globalPos());
    if (pos != mousePressPos)
        return;
    trySetPiece(mouseCell.x(), mouseCell.y());
}

void Board::trySetPiece(int r, int c)
{
    if (!inGame) return;
    int x = CELL(r, c);
    if ((bdState[x] & IS_PIECE) != 0) return;
    if (!othello::canPut(bdState[x], currentColor))
    {
        //hintPieces();
        return;
    }

    if (userColor == currentColor)
        emit decide(r, c);

    if (algo->setPiece(currentColor, r, c) != 0)
    {
        noPlace = 0;
        hintStop();
        movieStop();
        currentColor = BLACK + WHITE - currentColor;
    }
    else
    {
        if (algo->refreshCan(currentColor) == 0)
            gameEnd(algo->checkWin());
        else
            if (userColor == currentColor)
                gameMsg("对手无子可落，请继续落子");
            else
                gameMsg("您无子可落，对方继续落子");
    }

    if (userColor == currentColor)
    {
        restB->restart(30);
        totalB->resume();
        totalA->pause();
        restA->restart(0);
    }
    else
    {
        restA->restart(30);
        totalA->resume();
        totalB->pause();
        restB->restart(0);
    }

    paintCurrentColor();
    paintPieces();
    paintScore();
}

void Board::userTimeOut()
{
    if (currentColor != userColor) return;
    std::vector<QPoint> v;
    v.clear();
    for (int k = 0; k < 64; k++)
        if (othello::canPut(bdState[k], userColor))
            v.push_back(QPoint(k / 8, k % 8));
    int chs = rand() % v.size();
    trySetPiece(v[chs].x(), v[chs].y());
}

QPoint Board::getMouseCell(QPoint pos)
{
    int x = pos.x(), y = pos.y();
    //qDebug("%d %d", x, y);
    //qDebug("gap %d cell %d total %d x0 %d y0 %d",
    //       GAPSIZE, CELLSIZE, TOTALSIZE, x0, y0);

    if (x <= x0 + GAPSIZE || x >= x0 + TOTALSIZE ||
        y <= y0 + GAPSIZE || y >= y0 + TOTALSIZE)
        return QPoint(-1, -1);
    x -= GAPSIZE + x0, y -= GAPSIZE + y0;
    if (x % (GAPSIZE + CELLSIZE) >= CELLSIZE ||
        y % (GAPSIZE + CELLSIZE) >= CELLSIZE)
        return QPoint(-1, -1);
    x = x / (GAPSIZE + CELLSIZE);
    y = y / (GAPSIZE + CELLSIZE);
    //qDebug("cell : %d %d", x, y);
    return QPoint(x, y);
}

void Board::react(int r, int c)
{
    qDebug("%d %d", r, c);
    if (r == GAME_INFO)
    {
        switch (c)
        {
        case GAME_READY:
        {
            heReady = 1;
            if (meReady)
                gamePrepare();
            break;
        }
        case BLACK:
        {
            userColor = BLACK;
            gameStart();
            break;
        }
        case WHITE:
        {
            userColor = WHITE;
            gameStart();
            break;
        }
        case GAME_FATALERROR:
        {
            qDebug() << "意外";
            return;
        }
        case GAME_RUNAWAY:
        {
            if (currentColor != userColor)
                gameMsg("对方逃跑了！");
            return;
        }
        case GAME_SURRENDER:
        {
            gameMsg("对方投降了！");
            gameEnd(GAME_INFO);
            return;
        }
        default:
        {
            return;
        }
        }
    }
    else
        if (userColor != currentColor)
            trySetPiece(r, c);
}

void Board::gameMsg(const char *s)
{
    QMessageBox dlg(this->parentWidget());

    /*QPalette pal;

    pal.setColor(QPalette::Window, QColor(32, 16, 0));
    pal.setColor(QPalette::WindowText, QColor(216, 156, 48));
    pal.setColor(QPalette::Button, QColor(32, 16, 0));
    pal.setColor(QPalette::ButtonText, QColor(216, 156, 48));
    dlg.setPalette(pal);*/

    QFont font;
    font.setBold(true);
    font.setPointSize(15);
    dlg.setFont(font);

    dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
    dlg.setWindowFlags(dlg.windowFlags() &~ (
                       Qt::WindowTitleHint |
                       Qt::WindowCloseButtonHint |
                       Qt::WindowMaximizeButtonHint |
                       Qt::WindowMinimizeButtonHint));
    dlg.setText(s);
    dlg.exec();
}
