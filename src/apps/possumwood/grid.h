#pragma once

#include <boost/noncopyable.hpp>

#include <ImathMatrix.h>

#include <possumwood_sdk/gl_renderable.h>

namespace possumwood {

/// A simple drawable grid, compatible with Core GL profile. To be created during a draw
/// call, when a GL context is active.
class Grid : public possumwood::GLRenderable {
  public:
	Grid();
};
}
