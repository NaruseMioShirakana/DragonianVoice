#include "MScrollArea.h"

#include <qcoreevent.h>
#include <QWheelEvent>

MScrollArea::MScrollArea(QWidget* parent) :
	QScrollArea(parent)
{
	installEventFilter(this);
}

bool MScrollArea::eventFilter(QObject* watched, QEvent* event)
{
	if(event->type() == QEvent::Wheel)
	{
		const auto e = reinterpret_cast<QWheelEvent*>(event);
		if (e->modifiers() != Qt::NoModifier)
		{
			event->ignore();
			return true;
		}
	}
	return QScrollArea::eventFilter(watched, event);
}
