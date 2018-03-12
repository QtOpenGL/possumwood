#include <memory>

#include <possumwood_sdk/node_implementation.h>

#include "datatypes/shader.h"
#include "ui/shader_editor.h"
#include "default_shaders.h"

namespace {

dependency_graph::InAttr<std::string> a_src;
dependency_graph::OutAttr<std::shared_ptr<const possumwood::VertexShader>> a_shader;

class Editor : public possumwood::ShaderEditor {
	public:
		Editor() : ShaderEditor(a_src) {
		}

};

dependency_graph::State compute(dependency_graph::Values& data) {
	dependency_graph::State result;

	const std::string& src = data.get(a_src);

	std::unique_ptr<possumwood::VertexShader> shader(new possumwood::VertexShader());

	shader->compile(src);
	result = shader->state();

	if(!shader->state().errored())
		data.set(a_shader, std::shared_ptr<const possumwood::VertexShader>(shader.release()));
	else
		data.set(a_shader, std::shared_ptr<const possumwood::VertexShader>());

	return result;
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_src, "source", possumwood::defaultVertexShaderSrc());
	meta.addAttribute(a_shader, "shader");

	meta.addInfluence(a_src, a_shader);

	meta.setCompute(&compute);
	meta.setEditor<Editor>();
}

possumwood::NodeImplementation s_impl("render/vertex_shader", init);

}
