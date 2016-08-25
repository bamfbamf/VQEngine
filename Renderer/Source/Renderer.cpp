//	DX11Renderer - VDemo | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
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

#include "Renderer.h"
#include "D3DManager.h"
#include "BufferObject.h"
#include "Shader.h"
#include "Mesh.h"
#include "../../Application/Source/SystemDefs.h"
#include "../../Application/Source/utils.h"
#include "../../Application/Source/Camera.h"

#include <cassert>

Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_bufferObjects(std::vector<BufferObject*>(MESH_TYPE::COUNT))
{}


Renderer::~Renderer()
{
}

bool Renderer::Init(int width, int height, HWND hwnd)
{
	m_hWnd = hwnd;

	// Create the Direct3D object.
	m_Direct3D = new D3DManager();
	if (!m_Direct3D)
	{
		assert(false);
		return false;
	}

	// Initialize the Direct3D object.
	bool result = m_Direct3D->Init(width, height, Renderer::VSYNC, m_hWnd, FULL_SCREEN, NEAR_PLANE, FAR_PLANE);
	if (!result)
	{
		MessageBox(m_hWnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}
	m_device		= m_Direct3D->GetDevice();
	m_deviceContext = m_Direct3D->GetDeviceContext();


	GeneratePrimitives();
	return true;
}

void Renderer::Exit()
{
	// Release the Direct3D object.
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
	}

	// cleanup buffer objects
	for (BufferObject* bo : m_bufferObjects)
	{
		delete bo;
		bo = nullptr;
	}

	for (Shader* shd : m_shaders)
	{
		delete shd;
		shd = nullptr;
	}

	return;
}

HWND Renderer::GetWindow() const
{
	return m_hWnd;
}

float Renderer::AspectRatio() const
{
	return m_Direct3D->AspectRatio();
}

unsigned Renderer::WindowHeight() const
{
	return m_Direct3D->WindowHeight();
}

unsigned Renderer::WindowWidth() const
{
	return m_Direct3D->WindowWidth();
}

void Renderer::GeneratePrimitives()
{
	m_geom.SetDevice(m_device);
	m_bufferObjects[TRIANGLE]	= m_geom.Triangle();
	m_bufferObjects[QUAD]		= m_geom.Quad();
	m_bufferObjects[CUBE]		= m_geom.Cube();
}

void Renderer::PollShaderFiles()
{
	// todo: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx
}

ShaderID Renderer::AddShader(const std::string& shdFileName, 
							 const std::string& fileRoot,
							 const std::vector<InputLayout>& layouts)
{
	// example params: "tex", "Shader/"
	std::string path = fileRoot + shdFileName;

	Shader* shd = new Shader(shdFileName);
	shd->Compile(m_device, path, layouts);
	m_shaders.push_back(shd);
	shd->AssignID(static_cast<int>(m_shaders.size()) - 1);
	return shd->ID();
}

void Renderer::SetShader(ShaderID id)
{
	assert(id >= 0 && static_cast<unsigned>(id) < m_shaders.size());
	m_activeShader = id;
}

void Renderer::Reset()
{
	m_activeShader = -1;
	m_activeBuffer = -1;
}


void Renderer::SetViewport(const unsigned width, const unsigned height)
{
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width	= static_cast<float>(width);
	m_viewPort.Height	= static_cast<float>(height);
}

void Renderer::SetBufferObj(int BufferID)
{
	m_activeBuffer = BufferID;
}

void Renderer::SetCamera(Camera* cam)
{
	m_mainCamera = cam;
}

void Renderer::SetConstant4x4f(const char* cName, const XMMATRIX& matrix)
{
	// Here, we write to the CPU version of the constant buffer -- if the contents are updated 
	// ofc, otherwise we don't write -- and set the dirty flag of the GPU CBuffer counterpart.
	// When all the constants are set on the CPU side, right before the draw call,
	// we will use a mapped resource as a block of memory, transfer our CPU
	// version of constants to there, and then send it to GPU CBuffers at one call as a block memory.
	// Otherwise, we would have to make an API call each time we set the constants, which
	// would be slower.
	// Read more here: https://developer.nvidia.com/sites/default/files/akamai/gamedev/files/gdc12/Efficient_Buffer_Management_McDonald.pdf
	//      and  here: https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0

	// prepare data to be copied
	XMFLOAT4X4 m;
	XMStoreFloat4x4(&m, matrix);
	float* data = &m.m[0][0];

	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_activeShader];
	bool found = false;
	for (size_t i = 0; i < shader->m_constants.size() && !found; i++)
	{
		std::vector<CPUConstant>& cVector = shader->m_constants[i];
		for (CPUConstant& c : cVector)
		{
			if (strcmp(cName, c.name.c_str()) == 0)
			{
				found = true;
				if (memcmp(c.data, data, c.size) != 0)	// copy data if its not the same
				{
					memcpy(c.data, data, c.size);
					shader->m_cBuffers[i].dirty = true;
					break;
				}
			}
		}
	}
	if (!found)
	{
		std::string err("Error: Constant not found: "); err += cName;
		OutputDebugString(err.c_str());
	}

	return;
}

void Renderer::Begin(const float clearColor[4])
{
	m_Direct3D->BeginFrame(clearColor);
}

void Renderer::End()
{
	m_Direct3D->EndFrame();
	++m_frameCount;
#ifdef _DEBUG
	if (m_frameCount % 60 == 0)	PollShaderFiles();
#endif
}


void Renderer::Apply()
{
	// Here, we make all the API calls for GPU data

	Shader* shader = m_shaders[m_activeShader];

	// set shaders & data layout
	m_deviceContext->IASetInputLayout(shader->m_layout);
	m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);

	// set vertex & index buffers & topology
	unsigned stride = sizeof(Vertex);	// layout?
	unsigned offset = 0;
	m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[m_activeBuffer]->m_vertexBuffer, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_bufferObjects[m_activeBuffer]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// viewport
	m_deviceContext->RSSetViewports(1, &m_viewPort);

	// set shader constants
	for (unsigned i = 0; i < shader->m_cBuffers.size(); ++i)
	{
		CBuffer& cbuf = shader->m_cBuffers[i];
		if (cbuf.dirty)	// if the CPU-side buffer is updated
		{
			ID3D11Buffer* bufferData = cbuf.data;
			D3D11_MAPPED_SUBRESOURCE mappedResource;

			// Map subresource to GPU - update contends - discard the subresource
			m_deviceContext->Map(bufferData, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			char* bufferPos = static_cast<char*>(mappedResource.pData);	// char* so we can advance the pointer
			std::vector<CPUConstant>& cpuConsts = shader->m_constants[i];
			for (CPUConstant& c : cpuConsts)
			{
				memcpy(bufferPos, c.data, c.size);
				bufferPos += c.size;
			}
			m_deviceContext->Unmap(bufferData, 0);

			m_deviceContext->VSSetConstantBuffers(i, 1, &bufferData);
			cbuf.dirty = false;
		}
	}
}

void Renderer::DrawIndexed()
{
	m_deviceContext->DrawIndexed(m_bufferObjects[m_activeBuffer]->m_indexCount, 0, 0);
}