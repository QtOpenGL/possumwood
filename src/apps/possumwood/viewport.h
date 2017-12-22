#pragma once

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include <QtOpenGL/QGLWidget>

#include <ImathVec.h>

class Viewport : public QGLWidget, public boost::noncopyable {
	Q_OBJECT

  signals:
	void render(float dt);

  public:
	Viewport(QWidget* parent = NULL);
	virtual ~Viewport();

  protected:
	virtual void initializeGL();
	virtual void paintGL();
	virtual void resizeGL(int w, int h);
	virtual void mouseMoveEvent(QMouseEvent* event);

  private:
	float m_sceneDistance, m_sceneRotationX, m_sceneRotationY;
	Imath::V3f m_origin;
	int m_mouseX, m_mouseY;

	boost::posix_time::ptime m_timer;
};
