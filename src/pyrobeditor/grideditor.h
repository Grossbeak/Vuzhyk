#ifndef GRIDEDITOR_H
#define GRIDEDITOR_H

#include <QGraphicsView>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QPointF>
#include <QRectF>
#include <QPen>

class ProjectModel;

class GridEditor : public QGraphicsView
{
	Q_OBJECT
public:
	enum Tool {
		PaintEmpty = 0,
		PaintToBeFilled = 1,
		PaintFilled = 2,
		SetStart = 3,
		SetParking = 4,
		ToggleWall = 5
	};

	explicit GridEditor(QWidget *parent = nullptr);
	~GridEditor();
	void setModel(ProjectModel *model);
	void setTool(Tool tool);
	void setTheme(const QString &theme);
	void clear(); // Явная очистка всех ресурсов

protected:
	void resizeEvent(QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

private:
	QPointF cellTopLeft(int i, int j) const;
	QRectF cellRect(int i, int j) const;
	bool pickCellAndEdge(const QPointF &pos, int &ci, int &cj, int &edge) const;
	void rebuildScene();

private:
	class QGraphicsScene *m_scene;
	ProjectModel *m_model;
	Tool m_tool;
	qreal m_cellSize;
	QPen m_gridPen;
	QPen m_wallPen;
	QString m_theme { "light" };
	int m_lastCi;
	int m_lastCj;
	int m_lastEdge;
};

#endif // GRIDEDITOR_H

