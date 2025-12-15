#include "grideditor.h"
#include "projectmodel.h"

#include <QMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QPainter>
#include <QPalette>

GridEditor::GridEditor(QWidget *parent)
	: QGraphicsView(parent)
	, m_scene(new QGraphicsScene(this))
	, m_model(nullptr)
	, m_tool(PaintEmpty)
	, m_cellSize(32.0)
	, m_gridPen(QColor(220,220,220))
	, m_wallPen(QColor(120,120,120))
	, m_theme("light")
	, m_lastCi(-1)
	, m_lastCj(-1)
	, m_lastEdge(-1)
{
	setScene(m_scene);
	m_gridPen.setWidthF(1.0);
	m_wallPen.setWidthF(3.0);
	setRenderHint(QPainter::Antialiasing, false);
	setMouseTracking(true);
	setAutoFillBackground(true);
}

GridEditor::~GridEditor() {
	clear();
}

void GridEditor::clear() {
	// Отключаем все связи
	if (m_model) {
		m_model->disconnect(this);
		m_model = nullptr;
	}
	
	// Очищаем сцену от всех элементов
	if (m_scene) {
		// Удаляем все элементы из сцены явно
		QList<QGraphicsItem*> items = m_scene->items();
		for (QGraphicsItem *item : items) {
			if (item) {
				m_scene->removeItem(item);
				delete item;
			}
		}
		// Очищаем сцену
		m_scene->clear();
		// Удаляем сцену из view
		setScene(nullptr);
		// Сцена удалится автоматически при удалении GridEditor, так как this - её родитель
		// Не удаляем её вручную, чтобы избежать двойного удаления
		m_scene = nullptr;
	}
}

void GridEditor::setModel(ProjectModel *model)
{
	// Отключаем старую модель перед установкой новой
	if (m_model) {
		m_model->disconnect(this);
	}
	m_model = model;
	if (m_model) {
		connect(m_model, &ProjectModel::changed, this, &GridEditor::rebuildScene);
		rebuildScene();
	} else {
		// Если модель удалена, очищаем сцену
		if (m_scene) {
			m_scene->clear();
		}
	}
}

void GridEditor::setTool(GridEditor::Tool tool)
{
	m_tool = tool;
}

void GridEditor::setTheme(const QString &theme)
{
	m_theme = theme;
	QPalette palette;
	if (theme == "dark") {
		m_gridPen.setColor(QColor(100, 100, 100));
		m_wallPen.setColor(QColor(180, 180, 180));
		setBackgroundBrush(QBrush(QColor(35, 35, 35)));
		palette.setColor(QPalette::Base, QColor(35, 35, 35));
		palette.setColor(QPalette::Window, QColor(35, 35, 35));
		setPalette(palette);
		setStyleSheet("QGraphicsView { background-color: #232323; border: none; }");
	} else {
		m_gridPen.setColor(QColor(220, 220, 220));
		m_wallPen.setColor(QColor(120, 120, 120));
		setBackgroundBrush(QBrush(QColor(255, 255, 255)));
		palette.setColor(QPalette::Base, QColor(255, 255, 255));
		palette.setColor(QPalette::Window, QColor(255, 255, 255));
		setPalette(palette);
		setStyleSheet("QGraphicsView { background-color: #ffffff; border: none; }");
	}
	rebuildScene();
	update();
}

void GridEditor::resizeEvent(QResizeEvent *event)
{
	QGraphicsView::resizeEvent(event);
	rebuildScene();
}

QPointF GridEditor::cellTopLeft(int i, int j) const
{
	return QPointF(j * m_cellSize, i * m_cellSize);
}

QRectF GridEditor::cellRect(int i, int j) const
{
	QPointF tl = cellTopLeft(i, j);
	return QRectF(tl, QSizeF(m_cellSize, m_cellSize));
}

bool GridEditor::pickCellAndEdge(const QPointF &pos, int &ci, int &cj, int &edge) const
{
	ci = static_cast<int>(pos.y() / m_cellSize);
	cj = static_cast<int>(pos.x() / m_cellSize);
	if (!m_model) return false;
	if (ci < 0 || cj < 0 || ci >= m_model->rows() || cj >= m_model->cols()) return false;
	QRectF r = cellRect(ci, cj);
	qreal leftDist = qAbs(pos.x() - r.left());
	qreal rightDist = qAbs(pos.x() - r.right());
	qreal topDist = qAbs(pos.y() - r.top());
	qreal bottomDist = qAbs(pos.y() - r.bottom());
	qreal mind = qMin(qMin(leftDist, rightDist), qMin(topDist, bottomDist));
	if (mind > 6.0) { edge = -1; } else if (mind == leftDist) { edge = ProjectModel::Left; } else if (mind == rightDist) { edge = ProjectModel::Right; } else if (mind == topDist) { edge = ProjectModel::Top; } else { edge = ProjectModel::Bottom; }
	return true;
}

void GridEditor::rebuildScene()
{
	m_scene->clear();
	if (!m_model) return;
	// fit cell size
	int r = m_model->rows();
	int c = m_model->cols();
	QSizeF avail = viewport()->size();
	m_cellSize = qMax<qreal>(16.0, qMin((avail.width()-2.0)/c, (avail.height()-2.0)/r));
	m_scene->setSceneRect(0, 0, c*m_cellSize, r*m_cellSize);

		// draw cells
	for (int i = 0; i < r; ++i) {
		for (int j = 0; j < c; ++j) {
			QRectF rc = cellRect(i, j);
			QColor fill(255,255,255);
			if (m_theme == "dark") {
				switch (m_model->cellType(i,j)) {
				case ProjectModel::Empty: fill = QColor(50, 50, 50); break;
				case ProjectModel::ToBeFilled: fill = QColor(80, 140, 135); break;
				case ProjectModel::FilledInitial: fill = QColor(140, 130, 100); break;
				}
			} else {
				switch (m_model->cellType(i,j)) {
				case ProjectModel::Empty: fill = QColor(255,255,255); break;
				case ProjectModel::ToBeFilled: fill = QColor(161,220,216); break;
				case ProjectModel::FilledInitial: fill = QColor(255,248,209); break;
				}
			}
			QGraphicsRectItem *cell = m_scene->addRect(rc, Qt::NoPen, QBrush(fill));
			cell->setZValue(-2);
		}
	}

	// draw grid lines
	for (int i = 0; i <= r; ++i) {
		m_scene->addLine(0, i*m_cellSize, c*m_cellSize, i*m_cellSize, m_gridPen)->setZValue(-1);
	}
	for (int j = 0; j <= c; ++j) {
		m_scene->addLine(j*m_cellSize, 0, j*m_cellSize, r*m_cellSize, m_gridPen)->setZValue(-1);
	}

	// outer border stronger
	m_scene->addLine(0, 0, c*m_cellSize, 0, m_wallPen);
	m_scene->addLine(0, r*m_cellSize, c*m_cellSize, r*m_cellSize, m_wallPen);
	m_scene->addLine(0, 0, 0, r*m_cellSize, m_wallPen);
	m_scene->addLine(c*m_cellSize, 0, c*m_cellSize, r*m_cellSize, m_wallPen);

	// draw interior walls (right/bottom)
	for (int i = 0; i < r; ++i) {
		for (int j = 0; j < c; ++j) {
			QRectF rc = cellRect(i, j);
			if (j < c-1 && m_model->wall(i, j, ProjectModel::Right)) {
				m_scene->addLine(rc.right(), rc.top(), rc.right(), rc.bottom(), m_wallPen);
			}
			if (i < r-1 && m_model->wall(i, j, ProjectModel::Bottom)) {
				m_scene->addLine(rc.left(), rc.bottom(), rc.right(), rc.bottom(), m_wallPen);
			}
		}
	}

	// start and parking markers
	if (m_model->hasStart()) {
		QPoint st = m_model->startCell();
		QRectF rc = cellRect(st.x(), st.y());
		QRectF inner(rc.adjusted(6,6,-6,-6));
		QPen pen(m_theme == "dark" ? QColor(100, 150, 255) : Qt::blue); pen.setWidth(2);
		m_scene->addEllipse(inner, pen, Qt::NoBrush);
	}
	if (m_model->hasParking()) {
		QPoint pk = m_model->parkingCell();
		QRectF rc = cellRect(pk.x(), pk.y());
		QRectF inner(rc.adjusted(10,10,-10,-10));
		QColor parkingColor = m_theme == "dark" ? QColor(200, 200, 200) : Qt::black;
		m_scene->addEllipse(inner, Qt::NoPen, QBrush(parkingColor));
	}
}

void GridEditor::mousePressEvent(QMouseEvent *event)
{
	if (!m_model) return;
	QPointF pos = mapToScene(event->pos());
	int ci, cj, edge;
	if (!pickCellAndEdge(pos, ci, cj, edge)) return;

    if (event->button() == Qt::RightButton) {
        if (edge >= 0) {
            m_model->setWall(ci, cj, static_cast<ProjectModel::WallSide>(edge), false);
            m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
            return;
        }
        // Clear cell contents
        m_model->setCellType(ci, cj, ProjectModel::Empty);
        if (m_model->hasStart() && m_model->startCell() == QPoint(ci, cj)) m_model->clearStart();
        if (m_model->hasParking() && m_model->parkingCell() == QPoint(ci, cj)) m_model->clearParking();
        m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
        return;
    }

	if (m_tool == ToggleWall) {
		if (edge >= 0) {
			m_model->toggleWall(ci, cj, static_cast<ProjectModel::WallSide>(edge));
		}
        m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
		return;
	}

	switch (m_tool) {
	case PaintEmpty:
		m_model->setCellType(ci, cj, ProjectModel::Empty);
		break;
	case PaintToBeFilled:
		m_model->setCellType(ci, cj, ProjectModel::ToBeFilled);
		break;
	case PaintFilled:
		m_model->setCellType(ci, cj, ProjectModel::FilledInitial);
		break;
	case SetStart:
		m_model->setStartCell(ci, cj);
		break;
	case SetParking:
		m_model->setParkingCell(ci, cj);
		break;
	case ToggleWall:
		break;
	}
    m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
}

void GridEditor::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_model) return;
    if (!((event->buttons() & Qt::LeftButton) || (event->buttons() & Qt::RightButton))) return;
    QPointF pos = mapToScene(event->pos());
    int ci, cj, edge;
    if (!pickCellAndEdge(pos, ci, cj, edge)) return;

    if (event->buttons() & Qt::RightButton) {
        if (edge >= 0 && (ci != m_lastCi || cj != m_lastCj || edge != m_lastEdge)) {
            m_model->setWall(ci, cj, static_cast<ProjectModel::WallSide>(edge), false);
            m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
        } else if (edge < 0 && (ci != m_lastCi || cj != m_lastCj)) {
            m_model->setCellType(ci, cj, ProjectModel::Empty);
            if (m_model->hasStart() && m_model->startCell() == QPoint(ci, cj)) m_model->clearStart();
            if (m_model->hasParking() && m_model->parkingCell() == QPoint(ci, cj)) m_model->clearParking();
            m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
        }
        return;
    }

    if (m_tool == ToggleWall) {
        if (edge >= 0 && (ci != m_lastCi || cj != m_lastCj || edge != m_lastEdge)) {
            m_model->toggleWall(ci, cj, static_cast<ProjectModel::WallSide>(edge));
            m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
        }
        return;
    }

    if (ci == m_lastCi && cj == m_lastCj) return;
    switch (m_tool) {
    case PaintEmpty:
        m_model->setCellType(ci, cj, ProjectModel::Empty);
        break;
    case PaintToBeFilled:
        m_model->setCellType(ci, cj, ProjectModel::ToBeFilled);
        break;
    case PaintFilled:
        m_model->setCellType(ci, cj, ProjectModel::FilledInitial);
        break;
    case SetStart:
        m_model->setStartCell(ci, cj);
        break;
    case SetParking:
        m_model->setParkingCell(ci, cj);
        break;
    case ToggleWall:
        break;
    }
    m_lastCi = ci; m_lastCj = cj; m_lastEdge = edge;
}







