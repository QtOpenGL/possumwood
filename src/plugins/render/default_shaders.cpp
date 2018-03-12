#include "default_shaders.h"

namespace possumwood {

const std::string& defaultVertexShaderSrc() {
	static std::string s_src =
		"#version 130 \n"
		" \n"
		"in vec3 position;              // position attr from the vbo \n"
		" \n"
		"uniform mat4 iProjection;      // projection matrix \n"
		"uniform mat4 iModelView;       // modelview matrix \n"
		" \n"
		"out vec3 vertexPosition;       // vertex position for the fragment shader \n"
		" \n"
		"void main() {\n"
		"	vec4 pos4 = vec4(position.x, position.y, position.z, 1);\n"
		"\n"
		"	vertexPosition = (iModelView * pos4).xyz;\n"
		"   	gl_Position = iProjection * iModelView * pos4;\n"
		"} \n";

	return s_src;
}

const possumwood::VertexShader& defaultVertexShader() {
	static std::unique_ptr<possumwood::VertexShader> s_shader;

	if(s_shader == nullptr) {
		s_shader = std::unique_ptr<possumwood::VertexShader>(new VertexShader());

		s_shader->compile(defaultVertexShaderSrc());

		assert(!s_shader->state().errored());
	}

	return *s_shader;
}

const std::string& defaultFragmentShaderSrc() {
	static std::string s_src =
		"#version 130 \n"
		" \n"
		"out vec4 color; \n"
		" \n"
		"in vec3 vertexPosition; \n"
		" \n"
		"void main() { \n"
		"	vec3 dx = dFdx(vertexPosition); \n"
		"	vec3 dy = dFdy(vertexPosition); \n"
		" \n"
		"	vec3 norm = normalize(cross(dx, dy)); \n"
		" \n"
		"	color = vec4(norm.z, norm.z, norm.z, 1); \n"
		"} \n";

	return s_src;
}

const possumwood::FragmentShader& defaultFragmentShader() {
	static std::unique_ptr<possumwood::FragmentShader> s_shader;

	if(s_shader == nullptr) {
		s_shader = std::unique_ptr<possumwood::FragmentShader>(new FragmentShader());

		s_shader->compile(defaultFragmentShaderSrc());

		assert(!s_shader->state().errored());
	}

	return *s_shader;
}


}
