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

#include "LightingCommon.hlsl"
#include "BRDF.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};


// CBUFFERS
//---------------------------------------------------------
// defines maximum number of dynamic lights
#define LIGHT_COUNT 20  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^

cbuffer SceneVariables	// frame constants
{
	matrix lightSpaceMat; // [arr?]
	//float  pad0;
	//float3 CameraWorldPosition;
	matrix matView;
	matrix matViewToWorld;

	float  lightCount;
	float  spotCount;
	float2 padding;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	
};


// TEXTURES & SAMPLERS
//---------------------------------------------------------
Texture2D texDiffuseRoughnessMap;
Texture2D texSpecularMetalnessMap;
Texture2D texNormals;
Texture2D texPosition;
Texture2D texShadowMap;

SamplerState sShadowSampler;
SamplerState sLinearSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters (View Space Lighting)
	const float3 P = texPosition.Sample(sLinearSampler, In.uv);
	const float3 N = texNormals.Sample(sLinearSampler, In.uv);
	//const float3 T = normalize(In.tangent);
	const float3 V = normalize(- P);
	
	const float3 Pw = mul(matViewToWorld, float4(P, 1)).xyz;
    const float4 lightSpacePos = mul(lightSpaceMat, float4(Pw, 1));

	const float4 diffuseRoughness  = texDiffuseRoughnessMap.Sample(sLinearSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sLinearSampler, In.uv);

    BRDF_Surface s;
    s.N = N;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.roughness = diffuseRoughness.a;
	s.metalness = specularMetalness.a;

#if 1
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular

	// POINT Lights
	// brightness default: 300
	//---------------------------------
	for (int i = 0; i < lightCount; ++i)		
	{
		const float3 Lv       = mul(matView, float4(lights[i].position, 1));
		const float3 Wi       = normalize(Lv - P);
		const float3 radiance = 
			AttenuationBRDF(lights[i].attenuation, length(lights[i].position - Pw))
			* lights[i].color 
			* lights[i].brightness;
		IdIs += BRDF(Wi, s, V, P) * radiance;
	}

#if 1
	// SPOT Lights (shadow)
	//---------------------------------
	for (int j = 0; j < spotCount; ++j)
	{
		const float3 Lv        = mul(matView, float4(spots[j].position, 1));
		const float3 Wi        = normalize(Lv - P);
		const float3 radiance  = Intensity(spots[j], Pw) * spots[j].color * spots[j].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float3 shadowing = ShadowTest(Pw, lightSpacePos, texShadowMap, sShadowSampler);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing;
	}
#endif

	const float3 illumination = IdIs;
	return float4(illumination, 1);
#endif
}