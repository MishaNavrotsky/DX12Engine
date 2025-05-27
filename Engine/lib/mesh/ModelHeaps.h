#include "stdafx.h"

#pragma once

#include "helpers.h"

namespace Engine {
	class ModelHeaps : public IID {
	public:
		ModelHeaps() = default;

		ComPtr<ID3D12Resource>& getGeometryUploadHeap() { return geometryUploadHeap; }
		ComPtr<ID3D12Resource>& getIndexUploadHeap() { return indexUploadHeap; }
		ComPtr<ID3D12Resource>& getGeometryDefaultHeap() { return geometryDefaultHeap; }
		ComPtr<ID3D12Resource>& getIndexDefaultHeap() { return indexDefaultHeap; }
		ComPtr<ID3D12Resource>& getTexturesUploadHeap() { return texturesUploadHeap; }
		ComPtr<ID3D12Resource>& getCBVsUploadHeap() { return cbvsUploadHeap; }

		void releaseUploadHeaps() {
			geometryUploadHeap = nullptr;
			indexUploadHeap = nullptr;
			texturesUploadHeap = nullptr;
			cbvsUploadHeap = nullptr;
		}
	private:
		ComPtr<ID3D12Resource> geometryUploadHeap;
		ComPtr<ID3D12Resource> indexUploadHeap;

		ComPtr<ID3D12Resource> geometryDefaultHeap;
		ComPtr<ID3D12Resource> indexDefaultHeap;

		ComPtr<ID3D12Resource> texturesUploadHeap;
		ComPtr<ID3D12Resource> cbvsUploadHeap;
	};
}