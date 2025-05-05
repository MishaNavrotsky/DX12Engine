#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	using namespace Microsoft::WRL;

	class ModelHeaps : public IID {
	public:
		ModelHeaps() = default;

		void setGeometryUploadHeap(ComPtr<ID3D12Resource>&& resource) { geometryUploadHeap = std::move(resource); }
		void setIndexUploadHeap(ComPtr<ID3D12Resource>&& resource) { indexUploadHeap = std::move(resource); }
		void setGeometryDefaultHeap(ComPtr<ID3D12Resource>&& resource) { geometryDefaultHeap = std::move(resource); }
		void setIndexDefaultHeap(ComPtr<ID3D12Resource>&& resource) { indexDefaultHeap = std::move(resource); }
		void setTexturesUploadHeap(ComPtr<ID3D12Resource>&& resource) { texturesUploadHeap = std::move(resource); }

		ComPtr<ID3D12Resource>& getGeometryUploadHeap() { return geometryUploadHeap; }
		ComPtr<ID3D12Resource>& getIndexUploadHeap() { return indexUploadHeap; }
		ComPtr<ID3D12Resource>& getGeometryDefaultHeap() { return geometryDefaultHeap; }
		ComPtr<ID3D12Resource>& getIndexDefaultHeap() { return indexDefaultHeap; }
		ComPtr<ID3D12Resource>& getTexturesUploadHeap() { return texturesUploadHeap; }

		void releaseUploadHeaps() {
			geometryUploadHeap = nullptr;
			indexUploadHeap = nullptr;
			texturesUploadHeap = nullptr;
		}

	private:
		ComPtr<ID3D12Resource> geometryUploadHeap;
		ComPtr<ID3D12Resource> indexUploadHeap;

		ComPtr<ID3D12Resource> geometryDefaultHeap;
		ComPtr<ID3D12Resource> indexDefaultHeap;

		ComPtr<ID3D12Resource> texturesUploadHeap;
	};
}