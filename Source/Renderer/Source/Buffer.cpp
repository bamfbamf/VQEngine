//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

//#include "BufferObject.h"
#include "Renderer.h"

#include "Utilities/Log.h"

Buffer::Buffer(const BufferDesc& desc)
	: mDesc(desc)
	, mDirty(true)
	, mpCPUData(nullptr)
	, mpGPUData(nullptr)
{}

void Buffer::Initialize(ID3D11Device* device, const void* pData /*=nullptr*/)
{
	// GPU BUFFER
	D3D11_BUFFER_DESC bufDesc;
	bufDesc.Usage = static_cast<D3D11_USAGE>(mDesc.mUsage);
	bufDesc.BindFlags = static_cast<D3D11_BIND_FLAG>(mDesc.mType);
	bufDesc.ByteWidth = mDesc.mStride * mDesc.mElementCount;
	bufDesc.CPUAccessFlags = mDesc.mUsage == EBufferUsage::GPU_READ_CPU_WRITE ? D3D11_CPU_ACCESS_WRITE : 0;	// dynamic r/w?
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = mDesc.mStructureByteStride;

	D3D11_SUBRESOURCE_DATA* pBufData = nullptr;
	D3D11_SUBRESOURCE_DATA bufData = {};
	if (pData)
	{
		bufData.pSysMem = pData;
		bufData.SysMemPitch = 0;		// irrelevant for non-texture sub-resources
		bufData.SysMemSlicePitch = 0;	// irrelevant for non-texture sub-resources
		pBufData = &bufData;

		mpCPUData = malloc(bufDesc.ByteWidth);
		memcpy(mpCPUData, pData, bufDesc.ByteWidth);
	}
	else
	{
		int a = 5;
	}

	int hr = device->CreateBuffer(&bufDesc, pBufData, &this->mpGPUData);
	if (FAILED(hr))
	{
		Log::Error("Failed to create TODO buffer!");
	}

#if defined(_DEBUG) || defined(PROFILE)
	// no identified for buffers
	//if (!this->mpCPUData .empty())
	//{
	//	this->mpGPUData->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(this->_name.length()), this->_name.c_str());
	//}
#endif

	// CPU Buffer
	// we'll use mCPUData only when the created buffer is dynamic or staging
	if (mDesc.mUsage == GPU_READ_CPU_WRITE || mDesc.mUsage == GPU_READ_CPU_READ)
	{
		//assert(false);
		//const size_t AllocSize = mDesc.mStride * mDesc.mElementCount;
		//mCPUDataCache = mAllocator.allocate(AllocSize);
	}
}

void Buffer::CleanUp()
{
	if (mpGPUData)
	{
		mpGPUData->Release();
		mpGPUData = nullptr;
	}

	if (mpCPUData)
	{
		free(mpCPUData);
		//const size_t AllocSize = mDesc.mStride * mDesc.mElementCount;
		//mAllocator.deallocate(static_cast<char*>(mCPUDataCache), AllocSize);
	}
}

void Buffer::Update(Renderer* pRenderer, const void* pData)
{
	auto* ctx = pRenderer->m_deviceContext;

	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	constexpr UINT Subresource = 0;
	constexpr UINT MapFlags = 0;
	const UINT Size = mDesc.mStride * mDesc.mElementCount;

	ctx->Map(mpGPUData, Subresource, D3D11_MAP_WRITE_DISCARD, MapFlags, &mappedResource);
	memcpy(mappedResource.pData, pData, Size);
	ctx->Unmap(mpGPUData, Subresource);
}