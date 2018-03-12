#include <possumwood_sdk/node_implementation.h>

#include "possumwood_sdk/datatypes/enum.h"

#include "datatypes/meshes.h"

#include <CGAL/Polygon_mesh_processing/compute_normal.h>

namespace {

using possumwood::Meshes;
using possumwood::CGALPolyhedron;

dependency_graph::InAttr<possumwood::Enum> a_mode;
dependency_graph::InAttr<Meshes> a_inMeshes;
dependency_graph::OutAttr<Meshes> a_outMesh;

namespace {
	struct FakeKernel {
		typedef float FT;

		typedef possumwood::CGALKernel::Point_3 Point_3;

		struct Vector_3 {
			Vector_3(float x, float y, float z) : data{{x,y,z}} {
			}

			Vector_3(const std::array<float, 3>& arr) : data(arr) {
			}

			Vector_3(CGAL::Null_vector) : data{{0,0,0}} {
			}

			float& operator[](std::size_t index) {
				return data[index];
			}

			const float& operator[](std::size_t index) const {
				return data[index];
			}

			operator const std::array<float, 3>& () const {
				return data;
			}

			std::array<float, 3> data;
		};

		struct Equal_3 {
			bool operator()(const Vector_3& p, CGAL::Null_vector) {
				return p[0] == 0 && p[1] == 0 && p[2] == 0;
			}
		};

		auto construct_vector_3_object() const {
			struct Result {
				Vector_3 operator() (CGAL::Null_vector) {
					return Vector_3(CGAL::Null_vector());
				}

				Vector_3 operator() (const Vector_3& val) {
					return val;
				}

				Vector_3 operator() (const Point_3& p1, const Point_3& p2) {
					return Vector_3(p1[0]-p2[0], p1[1]-p2[1], p1[2]-p2[2]);
				}
			};

			return Result();
		}

		auto construct_sum_of_vectors_3_object() const {
			return [](const Vector_3& p1, const Vector_3& p2) {
				return std::array<float, 3>{{p1[0]+p2[0], p1[1]+p2[1], p1[2]+p2[2]}};
			};
		}

		auto construct_divided_vector_3_object() const {
			return [](const Vector_3& p1, float v) {
				return std::array<float, 3>{{p1[0]/v, p1[1]/v, p1[2]/v}};
			};
		}

		auto construct_scaled_vector_3_object() const {
			return [](const Vector_3& p1, float s) {
				return std::array<float, 3>{{p1[0]*s, p1[1]*s, p1[2]*s}};
			};
		}

		auto compute_squared_length_3_object() const {
			return [](const Vector_3& v1) {
				return v1[0]*v1[0] + v1[1]*v1[1] + v1[2]*v1[2];
			};
		}

		auto construct_cross_product_vector_3_object() const {
			return [](const Vector_3& p1, const Vector_3& p2) {
				return std::array<float, 3>{{
					p1[1]*p2[2] - p1[2]*p2[1],
					p1[2]*p2[0] - p1[0]*p2[2],
					p1[0]*p2[1] - p1[1]*p2[0]
				}};
			};
		}
	};

	template<typename PROPERTY, typename ITERATOR>
	void put(PROPERTY& prop, const ITERATOR& target, const FakeKernel::Vector_3& norm) {
		prop.set(target->property_key(), norm);
	}
}

dependency_graph::State compute(dependency_graph::Values& data) {
	const possumwood::Enum mode = data.get(a_mode);

	Meshes result = data.get(a_inMeshes);
	for(auto& mesh : result) {
		// request for vertex normals
		if(mode.value() == "Per-vertex normals") {
			// remove face normals, if they exist
			if(mesh.faceProperties().hasProperty("vec3:normals"))
				mesh.faceProperties().removeProperty("vec3:normals");

			auto& normals = mesh.vertexProperties().addProperty("vec3:normals", std::array<float, 3>{{0,0,0}});

			CGAL::Polygon_mesh_processing::compute_vertex_normals(mesh.polyhedron(), normals,
				CGAL::Polygon_mesh_processing::parameters::geom_traits(FakeKernel()));
		}

		// request for face normals
		else if(mode.value() == "Per-face normals") {
			// remove vertex normals, if they exist
			if(mesh.vertexProperties().hasProperty("vec3:normals"))
				mesh.vertexProperties().removeProperty("vec3:normals");

			auto& normals = mesh.faceProperties().addProperty("vec3:normals", std::array<float, 3>{{0,0,0}});

			CGAL::Polygon_mesh_processing::compute_face_normals(mesh.polyhedron(), normals,
				CGAL::Polygon_mesh_processing::parameters::geom_traits(FakeKernel()));
		}
	}

	data.set(a_outMesh, result);

	return dependency_graph::State();
}

void init(possumwood::Metadata& meta) {
	meta.addAttribute(a_mode, "mode",
	                  possumwood::Enum({"Per-face normals", "Per-vertex normals"}));
	meta.addAttribute(a_inMeshes, "input");
	meta.addAttribute(a_outMesh, "output");

	meta.addInfluence(a_mode, a_outMesh);
	meta.addInfluence(a_inMeshes, a_outMesh);

	meta.setCompute(compute);
}

possumwood::NodeImplementation s_impl("cgal/normals_generator", init);
}
