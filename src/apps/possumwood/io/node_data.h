#pragma once

#include "../node_data.h"

#include <dependency_graph/io/json.h>

void to_json(::dependency_graph::io::json& j, const NodeData& g);
void from_json(const ::dependency_graph::io::json& j, NodeData& g);