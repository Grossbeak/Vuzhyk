#include "SnakeGame.h"

#include <QPainter>
#include <QKeyEvent>
#include <QTimerEvent>
#include <QRandomGenerator>
#include <QFont>
#include <QMessageBox>

SnakeGame::SnakeGame(QWidget *parent)
	: QWidget(parent)
	, m_direction(Right)
	, m_nextDirection(Right)
	, m_timerId(-1)
	, m_score(0)
	, m_gameOver(false)
	, m_paused(false)
	, m_notStarted(true)
	, m_theme("light")
{
	setMinimumSize(GRID_SIZE * CELL_SIZE, GRID_SIZE * CELL_SIZE);
	setFocusPolicy(Qt::StrongFocus);
	// Инициализируем змейку и еду для отображения, но не запускаем таймер
	m_snake.clear();
	m_snake.append(QPoint(GRID_SIZE / 2, GRID_SIZE / 2));
	m_snake.append(QPoint(GRID_SIZE / 2 - 1, GRID_SIZE / 2));
	m_snake.append(QPoint(GRID_SIZE / 2 - 2, GRID_SIZE / 2));
	m_direction = Right;
	m_nextDirection = Right;
	generateFood();
}

void SnakeGame::setTheme(const QString &theme)
{
	m_theme = theme;
	update(); // Перерисовываем игру с новыми цветами
}

SnakeGame::~SnakeGame()
{
	if (m_timerId != -1) {
		killTimer(m_timerId);
	}
}

void SnakeGame::resetGame()
{
	m_snake.clear();
	// Начальная змейка из 3 клеток в центре
	m_snake.append(QPoint(GRID_SIZE / 2, GRID_SIZE / 2));
	m_snake.append(QPoint(GRID_SIZE / 2 - 1, GRID_SIZE / 2));
	m_snake.append(QPoint(GRID_SIZE / 2 - 2, GRID_SIZE / 2));
	m_direction = Right;
	m_nextDirection = Right;
	m_score = 0;
	m_gameOver = false;
	m_paused = false;
	m_notStarted = false;
	generateFood();
	
	if (m_timerId != -1) {
		killTimer(m_timerId);
	}
	m_timerId = startTimer(150); // Обновление каждые 150 мс
}

void SnakeGame::generateFood()
{
	do {
		m_food = QPoint(
			QRandomGenerator::global()->bounded(GRID_SIZE),
			QRandomGenerator::global()->bounded(GRID_SIZE)
		);
	} while (m_snake.contains(m_food));
}

bool SnakeGame::checkCollision()
{
	QPoint head = m_snake.first();
	
	// Проверка границ
	if (head.x() < 0 || head.x() >= GRID_SIZE ||
		head.y() < 0 || head.y() >= GRID_SIZE) {
		return true;
	}
	
	// Проверка столкновения с собой
	for (int i = 1; i < m_snake.size(); ++i) {
		if (m_snake[i] == head) {
			return true;
		}
	}
	
	return false;
}

void SnakeGame::moveSnake()
{
	if (m_gameOver || m_paused) return;
	
	m_direction = m_nextDirection;
	QPoint head = m_snake.first();
	QPoint newHead = head;
	
	switch (m_direction) {
	case Up:
		newHead.ry()--;
		break;
	case Down:
		newHead.ry()++;
		break;
	case Left:
		newHead.rx()--;
		break;
	case Right:
		newHead.rx()++;
		break;
	}
	
	m_snake.prepend(newHead);
	
	// Проверка еды
	if (newHead == m_food) {
		m_score++;
		generateFood();
		// Увеличиваем скорость каждые 5 очков
		if (m_score % 5 == 0 && m_timerId != -1) {
			killTimer(m_timerId);
			int newInterval = qMax(50, 150 - (m_score / 5) * 10);
			m_timerId = startTimer(newInterval);
		}
	} else {
		m_snake.removeLast();
	}
	
	// Проверка столкновения
	if (checkCollision()) {
		m_gameOver = true;
		killTimer(m_timerId);
		m_timerId = -1;
		update();
	}
}

void SnakeGame::keyPressEvent(QKeyEvent *event)
{
	if (m_notStarted) {
		if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
			m_notStarted = false;
			resetGame();
			update();
		}
		return;
	}
	
	if (m_gameOver) {
		if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
			resetGame();
			m_notStarted = false;
			update();
		}
		return;
	}
	
	if (event->key() == Qt::Key_Space) {
		m_paused = !m_paused;
		update();
		return;
	}
	
	Direction newDirection = m_direction;
	switch (event->key()) {
	case Qt::Key_Up:
	case Qt::Key_W:
		newDirection = Up;
		break;
	case Qt::Key_Down:
	case Qt::Key_S:
		newDirection = Down;
		break;
	case Qt::Key_Left:
	case Qt::Key_A:
		newDirection = Left;
		break;
	case Qt::Key_Right:
	case Qt::Key_D:
		newDirection = Right;
		break;
	default:
		QWidget::keyPressEvent(event);
		return;
	}
	
	// Предотвращаем движение в противоположном направлении
	if ((m_direction == Up && newDirection == Down) ||
		(m_direction == Down && newDirection == Up) ||
		(m_direction == Left && newDirection == Right) ||
		(m_direction == Right && newDirection == Left)) {
		return;
	}
	
	m_nextDirection = newDirection;
}

void SnakeGame::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_timerId) {
		moveSnake();
		update();
	} else {
		QWidget::timerEvent(event);
	}
}

void SnakeGame::drawGame(QPainter &painter)
{
	// Определяем цвета в зависимости от темы
	bool isDark = (m_theme == "dark");
	
	// Фон
	if (isDark) {
		painter.fillRect(rect(), QColor(30, 30, 30));
	} else {
		painter.fillRect(rect(), QColor(240, 240, 240));
	}
	
	// Вычисляем размер игрового поля и масштаб для заполнения вкладки
	const int gameWidth = GRID_SIZE * CELL_SIZE;
	const int gameHeight = GRID_SIZE * CELL_SIZE;
	
	// Вычисляем масштаб, чтобы поле занимало максимальную область
	const qreal scaleX = static_cast<qreal>(width()) / gameWidth;
	const qreal scaleY = static_cast<qreal>(height()) / gameHeight;
	const qreal scale = qMin(scaleX, scaleY); // Используем меньший масштаб для сохранения пропорций
	
	// Вычисляем отступы для центрирования после масштабирования
	const int scaledWidth = static_cast<int>(gameWidth * scale);
	const int scaledHeight = static_cast<int>(gameHeight * scale);
	const int offsetX = (width() - scaledWidth) / 2;
	const int offsetY = (height() - scaledHeight) / 2;
	
	// Сохраняем состояние painter для трансформации
	painter.save();
	painter.translate(offsetX, offsetY);
	painter.scale(scale, scale);
	
	// Сетка
	if (isDark) {
		painter.setPen(QPen(QColor(60, 60, 60), 1));
	} else {
		painter.setPen(QPen(QColor(200, 200, 200), 1));
	}
	for (int i = 0; i <= GRID_SIZE; ++i) {
		painter.drawLine(i * CELL_SIZE, 0, i * CELL_SIZE, gameHeight);
		painter.drawLine(0, i * CELL_SIZE, gameWidth, i * CELL_SIZE);
	}
	
	if (m_notStarted) {
		// Рисуем змейку и еду для визуализации (полупрозрачно)
		painter.setOpacity(0.3);
		
		// Еда
		if (isDark) {
			painter.setBrush(QBrush(QColor(255, 80, 80)));
			painter.setPen(QPen(QColor(255, 120, 120), 2));
		} else {
			painter.setBrush(QBrush(QColor(255, 0, 0)));
			painter.setPen(QPen(QColor(200, 0, 0), 2));
		}
		painter.drawEllipse(m_food.x() * CELL_SIZE + 2, m_food.y() * CELL_SIZE + 2,
							CELL_SIZE - 4, CELL_SIZE - 4);
		
		// Змейка
		for (int i = 0; i < m_snake.size(); ++i) {
			QPoint cell = m_snake[i];
			QRect cellRect(cell.x() * CELL_SIZE + 1, cell.y() * CELL_SIZE + 1,
						   CELL_SIZE - 2, CELL_SIZE - 2);
			
			if (i == 0) {
				// Голова
				if (isDark) {
					painter.setBrush(QBrush(QColor(100, 255, 100)));
					painter.setPen(QPen(QColor(80, 255, 80), 2));
				} else {
					painter.setBrush(QBrush(QColor(0, 150, 0)));
					painter.setPen(QPen(QColor(0, 100, 0), 2));
				}
			} else {
				// Тело
				if (isDark) {
					painter.setBrush(QBrush(QColor(120, 255, 120)));
					painter.setPen(QPen(QColor(100, 255, 100), 1));
				} else {
					painter.setBrush(QBrush(QColor(0, 200, 0)));
					painter.setPen(QPen(QColor(0, 150, 0), 1));
				}
			}
			painter.drawRect(cellRect);
		}
		
		// Возвращаем полную непрозрачность для текста
		painter.setOpacity(1.0);
		painter.restore(); // Восстанавливаем трансформацию
		
		// Начальный экран - текст поверх (на весь виджет)
		if (isDark) {
			painter.setPen(QPen(QColor(220, 220, 220), 2));
		} else {
			painter.setPen(QPen(QColor(0, 0, 0), 2));
		}
		QFont font = painter.font();
		font.setPointSize(16);
		font.setBold(true);
		painter.setFont(font);
		QString startText = QString("Нажми пробел чтобы начать.\nУправление на стрелки.");
		QRect textRect = rect();
		painter.drawText(textRect, Qt::AlignCenter, startText);
		return;
	}
	
	if (m_gameOver) {
		painter.restore(); // Восстанавливаем трансформацию
		// Текст "Игра окончена"
		if (isDark) {
			painter.setPen(QPen(QColor(255, 100, 100), 2));
		} else {
			painter.setPen(QPen(QColor(200, 0, 0), 2));
		}
		QFont font = painter.font();
		font.setPointSize(24);
		font.setBold(true);
		painter.setFont(font);
		QString gameOverText = QString("ИГРА ОКОНЧЕНА\n\nСчёт: %1\n\nНажмите ПРОБЕЛ или ENTER\nчтобы начать заново").arg(m_score);
		QRect textRect = rect();
		painter.drawText(textRect, Qt::AlignCenter, gameOverText);
		return;
	}
	
	if (m_paused) {
		painter.restore(); // Восстанавливаем трансформацию
		// Текст "Пауза"
		if (isDark) {
			painter.setPen(QPen(QColor(180, 180, 180), 2));
		} else {
			painter.setPen(QPen(QColor(100, 100, 100), 2));
		}
		QFont font = painter.font();
		font.setPointSize(20);
		font.setBold(true);
		painter.setFont(font);
		painter.drawText(rect(), Qt::AlignCenter, "ПАУЗА\n\nНажмите ПРОБЕЛ чтобы продолжить");
		return;
	}
	
	// Еда
	if (isDark) {
		painter.setBrush(QBrush(QColor(255, 80, 80)));
		painter.setPen(QPen(QColor(255, 120, 120), 2));
	} else {
		painter.setBrush(QBrush(QColor(255, 0, 0)));
		painter.setPen(QPen(QColor(200, 0, 0), 2));
	}
	painter.drawEllipse(m_food.x() * CELL_SIZE + 2, m_food.y() * CELL_SIZE + 2,
						CELL_SIZE - 4, CELL_SIZE - 4);
	
	// Змейка
	for (int i = 0; i < m_snake.size(); ++i) {
		QPoint cell = m_snake[i];
		QRect cellRect(cell.x() * CELL_SIZE + 1, cell.y() * CELL_SIZE + 1,
					   CELL_SIZE - 2, CELL_SIZE - 2);
		
		if (i == 0) {
			// Голова
			if (isDark) {
				painter.setBrush(QBrush(QColor(100, 255, 100)));
				painter.setPen(QPen(QColor(80, 255, 80), 2));
			} else {
				painter.setBrush(QBrush(QColor(0, 150, 0)));
				painter.setPen(QPen(QColor(0, 100, 0), 2));
			}
		} else {
			// Тело
			if (isDark) {
				painter.setBrush(QBrush(QColor(120, 255, 120)));
				painter.setPen(QPen(QColor(100, 255, 100), 1));
			} else {
				painter.setBrush(QBrush(QColor(0, 200, 0)));
				painter.setPen(QPen(QColor(0, 150, 0), 1));
			}
		}
		painter.drawRect(cellRect);
	}
	
	// Восстанавливаем трансформацию перед рисованием счета
	painter.restore();
	
	// Счет (показываем только если игра идет) - используем уже вычисленные отступы и масштаб
	if (isDark) {
		painter.setPen(QPen(QColor(220, 220, 220), 1));
	} else {
		painter.setPen(QPen(QColor(0, 0, 0), 1));
	}
	QFont font = painter.font();
	font.setPointSize(12);
	font.setBold(true);
	painter.setFont(font);
	// Позиционируем счет с учетом масштаба
	painter.drawText(offsetX + static_cast<int>(10 * scale), 
	                 offsetY + static_cast<int>(25 * scale), 
	                 QString("Счёт: %1").arg(m_score));
}

void SnakeGame::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	drawGame(painter);
}

