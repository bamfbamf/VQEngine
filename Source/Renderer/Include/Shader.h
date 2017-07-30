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

#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <queue>
#include <array>
#include <tuple>

#include "Log.h"

using namespace DirectX;

enum LayoutFormat
{
	FLOAT32_2 = DXGI_FORMAT_R32G32_FLOAT,
	FLOAT32_3 = DXGI_FORMAT_R32G32B32_FLOAT,

	LAYOUT_FORMAT_COUNT
};

struct InputLayout
{
	std::string		semanticName;
	LayoutFormat	format;
};

// CONSTANT BUFFER MANAGEMENT
//------------------------------------------------------------------
// Current limitations: 
//  - cbuffers with same names in different shaders (PS/VS/GS/...)
//  - cbuffers with same names in the same shader (not tested)

// used to map **SetShaderConstant(); function in Renderer::Apply()
enum ShaderType
{
	VS = 0,
	GS,
	DS,
	HS,
	CS,
	PS,

	COUNT
};

// information used to create GPU/CPU constant buffers
struct ConstantBufferLayout
{
	D3D11_SHADER_BUFFER_DESC					desc;
	std::vector<D3D11_SHADER_VARIABLE_DESC>		variables;
	std::vector<D3D11_SHADER_TYPE_DESC>			types;
	unsigned									buffSize;
	ShaderType									shdType;
	unsigned									bufSlot;
};



// CPU side constant buffer
// -------------------------------------------------------------------
#define EQUALITY_OPTIMIZED
constexpr size_t MAX_CONSTANT_BUFFERS = 512;
using CPUConstantID = int;
struct CPUConstant
{
	using CPUConstantPool = std::array<CPUConstant, MAX_CONSTANT_BUFFERS>;
private:
	static CPUConstantPool s_constants;
	static size_t s_nextConstIndex;

public:
	inline static CPUConstant& Get(int id) { return s_constants[id]; }
	
	static std::tuple<CPUConstant&, CPUConstantID> GetNextAvailable();
	static void CleanUp();	// call once
	
	CPUConstant() : _name(), _size(0), _data(nullptr){}

	std::string _name;
	size_t		_size;
	void*		_data;

#ifdef EQUALITY_OPTIMIZED
	inline bool operator==(const CPUConstant& c) const { return (((this->_data == c._data) && this->_size == c._size) && this->_name == c._name); }
	inline bool operator!=(const CPUConstant& c) const { return ((this->_data != c._data) || this->_size != c._size || this->_name != c._name); }
#endif
};

// GPU side constant buffer
struct ConstantBuffer
{
	ShaderType shdType;
	unsigned	bufferSlot;
	ID3D11Buffer* data;
	bool dirty;
};
//-------------------------------------------------------------

// SHADER RESOURCE MANAGEMENT
//-------------------------------------------------------------
struct ShaderTexture
{
	std::string name;
	unsigned bufferSlot;
	ShaderType shdType;
};


struct ShaderSampler
{
	std::string name;
	unsigned bufferSlot;
	ShaderType shdType;
};

//-------------------------------------------------------------

enum SHADERS	// good enough for global namespace
{
	FORWARD_PHONG,
	UNLIT,
	TEXTURE_COORDINATES,
	NORMAL,
	TANGENT,
	BINORMAL,
	LINE,
	TBN,
	DEBUG,
	SKYBOX,
	BLOOM,
	BLUR,

	SHADER_COUNT
};


using ShaderID = int;

class Shader
{
public:
	Shader(const std::string& shaderFileName);
	~Shader();

	void ClearConstantBuffers();

	void UpdateConstants(ID3D11DeviceContext* context);


	const std::string&							Name() const;
	ShaderID									ID() const;
	const std::vector<ConstantBufferLayout>&	GetConstantBufferLayouts() const;
	const std::vector<ConstantBuffer>&			GetConstantBuffers() const;

private:
	friend class Renderer;
	void RegisterConstantBufferLayout(ID3D11ShaderReflection * sRefl, ShaderType type);
	void Compile(ID3D11Device* device, const std::string& shaderFileName, const std::vector<InputLayout>& layouts, bool geoShader);
	void SetReflections(ID3D10Blob* vsBlob, ID3D10Blob* psBlob, ID3D10Blob* gsBlob);
	void CheckSignatures();
	void SetConstantBuffers(ID3D11Device* device);

private:
	static std::array<ShaderID, SHADERS::SHADER_COUNT>	s_shaders;
	const std::string									m_name;
	ShaderID											m_id;

	ID3D11VertexShader*									m_vertexShader;
	ID3D11PixelShader*									m_pixelShader;
	ID3D11GeometryShader*								m_geoShader;
	ID3D11HullShader*									m_hullShader;
	ID3D11DomainShader*									m_domainShader;
	ID3D11ComputeShader*								m_computeShader;

	ID3D11ShaderReflection*								m_vsRefl;	// shader reflections, temporary?
	ID3D11ShaderReflection*								m_psRefl;	// shader reflections, temporary?
	ID3D11ShaderReflection*								m_gsRefl;	// shader reflections, temporary?

	ID3D11InputLayout*									m_layout;

	std::vector<ConstantBufferLayout>					m_CBLayouts;
	std::vector<ConstantBuffer>							m_cBuffers;
	std::vector<std::vector<CPUConstantID>>				m_constants;

	std::vector<ShaderTexture>							m_textures;
	std::vector<ShaderSampler>							m_samplers;
};

