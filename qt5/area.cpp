#include "uipriv_qt5.hpp"

#include "draw.hpp"

#include <QScrollArea>
#include <QPaintEvent>
#include <QGuiApplication>

#include <QDebug> // TODO: remove

struct uiArea : public uiQt5Control {};

static int translateModifiers(Qt::KeyboardModifiers k){
	int result = 0;
	if(k & Qt::ShiftModifier){
		result |= uiModifierShift;
	}
	if(k & Qt::ControlModifier){
		result |= uiModifierCtrl;
	}
	if(k & Qt::AltModifier){
		result |= uiModifierAlt;
	}
	if(k & Qt::MetaModifier){
		result |= uiModifierSuper;
	}
	return result;
}

class Area : public QWidget
{
	Q_DISABLE_COPY(Area)

public:
	Area(uiAreaHandler *ah)
		: ah_(ah)
	{
		setAttribute(Qt::WA_Hover);
	}

private:
	void paintEvent(QPaintEvent *event)
	{
		uiDrawContext painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);

		const auto clip = event->rect();

		// avoid using C99 initializer for portability *cough*MSVC*cough*
		uiAreaDrawParams params = {
			&painter, // Context
			(double)width(), // AreaWidth
			(double)height(), // AreaHeight
			(double)clip.x(), // ClipX;
			(double)clip.y(), // ClipY;
			(double)clip.width(), // ClipWidth;
			(double)clip.height(), // ClipHeight;
		};

		ah_->Draw(ah_,static_cast<uiArea*>(uiFindQt5ControlForQObject(this)),&params);
	}

	bool event(QEvent *event)
	{
		switch(event->type()) {
		//case QEvent::HoverMove: // don't care
		case QEvent::HoverEnter:
		case QEvent::HoverLeave:
			translateHoverEvent(static_cast<QHoverEvent*>(event));
			break;
		}
		return QWidget::event(event);
	}

	void translateHoverEvent(QHoverEvent *hoverEvent)
	{
		auto area = static_cast<uiArea*>(uiFindQt5ControlForQObject(this));
		switch(hoverEvent->type()) {
		case QEvent::HoverEnter:
			ah_->MouseCrossed(ah_,area,0);
			setFocus(Qt::MouseFocusReason); // so we get key events, hacky?
			setMouseTracking(true);
			break;
		case QEvent::HoverLeave:
			ah_->MouseCrossed(ah_,area,1);
			clearFocus();
			setMouseTracking(false);
			break;
		}
	}

	void mousePressEvent(QMouseEvent *mouseEvent) { translateMouseEvent(mouseEvent); }
	void mouseReleaseEvent(QMouseEvent *mouseEvent) { translateMouseEvent(mouseEvent); }
	void mouseDoubleClickEvent(QMouseEvent *mouseEvent) { translateMouseEvent(mouseEvent); }
	void mouseMoveEvent(QMouseEvent *mouseEvent) { translateMouseEvent(mouseEvent); }

	void translateMouseEvent(QMouseEvent *mouseEvent)
	{
		static bool warned = false;
		if (warned) {
			qWarning("TODO: mouseEvent translation is incomplete!");
			warned = true;
		}
		uiAreaMouseEvent me;
		auto pos = mouseEvent->pos();

		me.X = pos.x();
		me.Y = pos.y();

		me.AreaWidth = width();
		me.AreaHeight = height();

		// TODO test
		me.Down = (mouseEvent->type() == QEvent::MouseButtonPress) ? mouseEvent->button() : 0;
		me.Up =  (mouseEvent->type() == QEvent::MouseButtonRelease) ? mouseEvent->button() : 0;
		me.Count = (mouseEvent->type() == QEvent::MouseButtonDblClick) ? 2 : 1;
		me.Modifiers = translateModifiers(QGuiApplication::keyboardModifiers());
		me.Held1To64 = mouseEvent->buttons();

		ah_->MouseEvent(ah_, static_cast<uiArea*>(uiFindQt5ControlForQObject(this)), &me);
	}

	void keyPressEvent(QKeyEvent *keyEvent)  { translateKeyEvent(keyEvent); }
	void keyReleaseEvent(QKeyEvent *keyEvent)  { translateKeyEvent(keyEvent); }

	void translateKeyEvent(QKeyEvent *keyEvent)
	{
		uiAreaKeyEvent ke;

		qWarning() << "TODO: keyEvent translation is incomplete!" << keyEvent;

		ke.Key = keyEvent->key();
		ke.ExtKey = 0;
		ke.Modifier = 0;
		ke.Modifiers = translateModifiers(QGuiApplication::keyboardModifiers());
		ke.Up = (keyEvent->type() == QEvent::KeyRelease);

		ah_->KeyEvent(ah_,static_cast<uiArea*>(uiFindQt5ControlForQObject(this)),&ke);
	}

private:
	uiAreaHandler *ah_ = nullptr;
};

static Area *findArea(uiArea *a)
{
	if (auto widget = uiValidateAndCastObjTo<QWidget>(a)) {
		if (auto area = dynamic_cast<Area*>(widget)) {
			return area;
		}
		return widget->findChild<Area*>();
	}
	return nullptr;
}

void uiAreaSetSize(uiArea *a, intmax_t width, intmax_t height)
{
	qWarning("TODO: %p %d, %d", (void *)a, (int)width, (int)height);
}

void uiAreaQueueRedrawAll(uiArea *a)
{
	if (auto area = findArea(a)) {
		area->update();
	}
}

void uiAreaScrollTo(uiArea *a, double x, double y, double width, double height)
{
	qWarning("TODO: %p %f, %f, %f, %f", (void *)a, x, y, width, height);
}

void uiAreaBeginUserWindowMove(uiArea *a)
{
	qWarning("TODO");
	// if (auto area = findArea(a)) {
	// 	area->window()->move(0, 0);
	// }
}

void uiAreaBeginUserWindowResize(uiArea *a, uiWindowResizeEdge edge)
{
	qWarning("TODO");
}

uiArea *uiNewArea(uiAreaHandler *ah)
{
	auto area = new Area(ah);

	// The widget should get as much space as possible.
	area->setSizePolicy({QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding});

	return uiAllocQt5ControlType(uiArea,area,uiQt5Control::DeleteControlOnQObjectFree);
}

uiArea *uiNewScrollingArea(uiAreaHandler *ah, int width, int height)
{
	auto area = new Area(ah);
	area->setMinimumSize({(int)width,(int)height});

	auto scrollArea = new QScrollArea;
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(area);

	return uiAllocQt5ControlType(uiArea,scrollArea,uiQt5Control::DeleteControlOnQObjectFree);
}
