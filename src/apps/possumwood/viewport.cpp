#include "viewport.h"

#include <cassert>
#include <iostream>
#include <cmath>

#include <QtGui/QMouseEvent>

#include <ImathMatrix.h>

#include <GL/glu.h>

namespace {
std::vector<Viewport*> s_instances;
}

Viewport::Viewport(QWidget* parent)
    : QGLWidget(parent, (s_instances.size() > 0 ? s_instances[0] : NULL)),
      m_sceneDistance(10), m_sceneRotationX(30), m_sceneRotationY(30), m_origin(0, 0, 0),
      m_mouseX(0), m_mouseY(0) {
	s_instances.push_back(this);

	setMouseTracking(true);
}

Viewport::~Viewport() {
	int index = -1;
	for(unsigned a = 0; a < s_instances.size(); a++)
		if(s_instances[a] == this)
			index = a;
	assert(index >= 0);
	s_instances.erase(s_instances.begin() + index);
}

void Viewport::initializeGL() {
	glClearColor(0, 0, 0, 0);
	resizeGL(width(), height());

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	float white[3] = {1, 1, 1};
	float position[4] = {1, 1, 1, 0};

	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, white);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	// glEnable(GL_COLOR_MATERIAL);

	m_timer = boost::posix_time::microsec_clock::universal_time();
}

namespace {

Imath::V3f eyePosition(float sceneRotX, float sceneRotY, float sceneDist) {
	Imath::V3f eye;

	eye.x = sin(-sceneRotX / 180.0f * M_PI) * sceneDist;
	eye.z = cos(-sceneRotX / 180.0f * M_PI) * sceneDist;

	eye.y = sin(sceneRotY / 180.0f * M_PI) * sceneDist;
	eye.x *= cos(sceneRotY / 180.0f * M_PI);
	eye.z *= cos(sceneRotY / 180.0f * M_PI);

	return eye;
}

Imath::M44f lookAt(const Imath::V3f& eyePosition, const Imath::V3f& lookAt,
                   const Imath::V3f& upVector) {
	Imath::V3f forward = lookAt - eyePosition;
	forward.normalize();

	Imath::V3f side = forward.cross(upVector);
	side.normalize();

	Imath::V3f up = side.cross(forward);
	up.normalize();

	Imath::M44f rotmat;
	rotmat.makeIdentity();

	rotmat[0][0] = side.x;
	rotmat[1][0] = side.y;
	rotmat[2][0] = side.z;

	rotmat[0][1] = up.x;
	rotmat[1][1] = up.y;
	rotmat[2][1] = up.z;

	rotmat[0][2] = -forward.x;
	rotmat[1][2] = -forward.y;
	rotmat[2][2] = -forward.z;

	Imath::M44f transmat;
	transmat.setTranslation(Imath::V3f(-eyePosition.x, -eyePosition.y, -eyePosition.z));

	return transmat * rotmat;
}
}

void Viewport::paintGL() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, (float)width() / (float)height(), m_sceneDistance * 0.1f,
	               std::max(m_sceneDistance * 2.0f, 1000.0f));

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	const Imath::V3f eye =
	    eyePosition(m_sceneRotationX, m_sceneRotationY, m_sceneDistance);

	const Imath::M44f m = lookAt(eye + m_origin, m_origin, Imath::V3f(0, 1, 0));
	glMultMatrixf(m.getValue());

	const boost::posix_time::ptime t(boost::posix_time::microsec_clock::universal_time());
	const float dt = (float)(t - m_timer).total_microseconds() / 1e6f;
	m_timer = t;

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	emit render(dt);
	glPopAttrib();
}

void Viewport::resizeGL(int w, int h) {
	QGLWidget::resizeGL(w, h);

	glViewport(0, 0, w, h);
}

void Viewport::mouseMoveEvent(QMouseEvent* event) {
	if(event->buttons() & Qt::LeftButton) {
		m_sceneRotationX += (float)(event->x() - m_mouseX) / (float)width() * 360.0;
		m_sceneRotationY += (float)(event->y() - m_mouseY) / (float)height() * 360.0;

		update();
	}
	m_sceneRotationY = std::max(-89.0f, m_sceneRotationY);
	m_sceneRotationY = std::min(89.0f, m_sceneRotationY);

	if(event->buttons() & Qt::RightButton) {
		m_sceneDistance /= pow(10, (float)(event->y() - m_mouseY) / (float)height());

		update();
	}

	if(event->buttons() & Qt::MiddleButton) {
		const Imath::V3f eye =
		    eyePosition(m_sceneRotationX, m_sceneRotationY, m_sceneDistance);

		const float dx = -(float)(event->x() - m_mouseX) / (float)(width());
		const float dy = (float)(event->y() - m_mouseY) / (float)(height());

		Imath::V3f right = Imath::V3f(0, 1, 0).cross(eye).normalized();
		Imath::V3f up = eye.cross(right).normalized();

		right = right * m_sceneDistance * dx;
		up = up * m_sceneDistance * dy;

		m_origin += right + up;

		update();
	}

	m_mouseX = event->x();
	m_mouseY = event->y();
}
