#include <possumwood_sdk/node_implementation.h>

#include <OpenMesh/Tools/Decimater/ModIndependentSetsT.hh>

#include "datatypes/decimater_module.h"

#include "openmesh.h"
#include "om_log.h"

namespace {

dependency_graph::InAttr<std::vector<DecimaterModule>> a_in;
dependency_graph::OutAttr<std::vector<DecimaterModule>> a_out;

dependency_graph::State compute(dependency_graph::Values& data) {
	OMLog logRedirect;

	std::vector<DecimaterModule> decs = data.get(a_in);

	decs.push_back(DecimaterModule([](OpenMesh::Decimater::DecimaterT<Mesh>& dec) {
		OpenMesh::Decimater::ModIndependentSetsT<Mesh>::Handle mod;
		dec.add(mod);
	}));

	data.set(a_out, decs);

	return dependency_graph::State();
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_in, "in_modules", std::vector<DecimaterModule>());
	meta.addAttribute(a_out, "out_modules", std::vector<DecimaterModule>());

	meta.addInfluence(a_in, a_out);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("openmesh/decimation/module_independent_sets", init);

}