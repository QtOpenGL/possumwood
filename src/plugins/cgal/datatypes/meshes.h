#pragma once

#include <possumwood_sdk/traits.h>

#include "../meshes.h"

namespace possumwood {

template<>
struct Traits<Meshes> {
	// static IO<std::shared_ptr<const Mesh>> io;

	static constexpr std::array<float, 3> colour() {
		return std::array<float, 3>{{1, 0, 0}};
	}
};

}
