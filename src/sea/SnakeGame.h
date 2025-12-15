#ifndef SNAKEGAME_H
#define SNAKEGAME_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QPainter>
#include <QVector>
#include <QPoint>

class SnakeGame : public QWidget
{
	Q_OBJECT

public:
	explicit SnakeGame(QWidget *parent = nullptr);
	~SnakeGame();
	
	void setTheme(const QString &theme);

protected:
	void paintEvent(QPaintEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void timerEvent(QTimerEvent *event) override;

private:
	void resetGame();
	void moveSnake();
	void generateFood();
	bool checkCollision();
	void drawGame(QPainter &painter);

	enum Direction {
		Up,
		Down,
		Left,
		Right
	};

	QVector<QPoint> m_snake;
	Direction m_direction;
	Direction m_nextDirection;
	QPoint m_food;
	int m_timerId;
	int m_score;
	bool m_gameOver;
	bool m_paused;
	bool m_notStarted;
	QString m_theme { "light" };
	static const int GRID_SIZE = 20;
	static const int CELL_SIZE = 25;
};

#endif // SNAKEGAME_H

