#include "enum.h"

#include <algorithm>

namespace possumwood {

Enum::Enum(std::initializer_list<std::string> options) {
	int counter = 0;
	for(auto& o : options)
		m_options.push_back(std::make_pair(o, counter++));

	if(!m_options.empty())
		m_value = m_options[0];
}

Enum::Enum(std::initializer_list<std::pair<std::string, int>> options) : m_options(options.begin(), options.end()) {
	if(!m_options.empty())
		m_value = m_options[0];
}

const std::string& Enum::value() const {
	return m_value.first;
}

int Enum::intValue() const {
	return m_value.second;
}

void Enum::setValue(const std::string& value) {
	auto it = std::find_if(m_options.begin(), m_options.end(), [&value](const std::pair<std::string, int>& val) {return val.first == value;});
	assert(it != m_options.end());
	m_value = *it;
}

const std::vector<std::pair<std::string, int>>& Enum::options() const {
	return m_options;
}

Enum& Enum::operator=(const Enum& fn) {
	if(m_options.empty())
		m_options = fn.m_options;
	m_value = fn.m_value;

	return *this;
}

bool Enum::operator==(const Enum& fn) const {
	return m_value == fn.m_value && m_options == fn.m_options;
}

bool Enum::operator!=(const Enum& fn) const {
	return m_value != fn.m_value || m_options != fn.m_options;
}

void Enum::fromJson(const ::dependency_graph::io::json& json) {
	setValue(json.get<std::string>());
}

void Enum::toJson(::dependency_graph::io::json& json) const {
	json = value();
}

///////

namespace {

void toJson(::dependency_graph::io::json& json, const Enum& value) {
	json = value.value();
}

void fromJson(const ::dependency_graph::io::json& json, Enum& value) {
	value.setValue(json.get<std::string>());
}
}

IO<Enum> Traits<Enum>::io(&toJson, &fromJson);
}
