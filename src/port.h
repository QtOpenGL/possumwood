#pragma once

#include <QGraphicsTextItem>
#include <QGraphicsTextItem>
#include <QSet>

class Node;
class Edge;

class Port : public QGraphicsRectItem {
	public:
		enum Type { kInput = 1, kOutput = 2, kInputOutput = 3 };

		Port(const QString& name, Type t, QColor color, QGraphicsItem* parent);

		const QString name() const;
		const Type portType() const;
		const QColor color() const;

	private:
		void setWidth(unsigned w);
		unsigned minWidth() const;

		void adjustEdges();

		QGraphicsTextItem* m_name;

		QColor m_color;

		QGraphicsRectItem* m_in;
		QGraphicsRectItem* m_out;

		QSet<Edge*> m_edges;

		friend class Node;
		friend class Edge;
};
