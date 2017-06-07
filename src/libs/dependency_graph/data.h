#pragma once

#include <memory>
#include <functional>

#include "io/json.h"

namespace dependency_graph {

class BaseData {
	public:
		virtual ~BaseData() {};

		virtual void assign(const BaseData& src) = 0;
		virtual bool isEqual(const BaseData& src) const = 0;

		virtual void toJson(io::json& j) const = 0;
		virtual void fromJson(const io::json& j) = 0;

		virtual std::string type() const = 0;

		/// creates a new Data<T> instance based on type name
		static std::unique_ptr<BaseData> create(const std::string& type);

	protected:
		template<typename T>
		struct Factory {
			Factory();
		};

	private:
		static std::map<std::string, std::function<std::unique_ptr<BaseData>()>>& factories();
};

template<typename T>
struct Data : public BaseData {
	public:
		Data(const T& v = T());
		virtual ~Data();
		virtual void assign(const BaseData& src) override;
		virtual bool isEqual(const BaseData& src) const;

		virtual void toJson(io::json& j) const override;
		virtual void fromJson(const io::json& j) override;

		virtual std::string type() const override;

		T value;

	private:
		static Factory<T> m_factory;
};

}