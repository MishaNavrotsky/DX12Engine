#include "stdafx.h"

#pragma once

#include "MeshTextures.h"


namespace Engine {
	using namespace Microsoft::WRL;

	class Material {
	public:
		Material(std::string id) : m_id(id) {};
		std::string getId() {
			return m_id;
		}
		MeshTextures textures;
	private:
		std::string m_id;
	};
}