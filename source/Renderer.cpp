//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include <cassert>

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

#include <iostream>

using namespace dae;

//#define UV_grid
//#define tuktuk
#define vehicle

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];


	//Initialize Camera
	m_Camera.Initialize(float(m_Width) / m_Height, 45.f, { 0.f, 0.f, 0.f });

	// DEFINE MESH
#if defined(UV_grid)
	m_MeshesWorld.clear();
	m_MeshesWorld.reserve(1);
	m_MeshesWorld.emplace_back(
		Mesh{
			{
			Vertex{Vector3{-3,3,-2},	ColorRGB{1,1,1},	Vector2{0,0}},
			Vertex{Vector3{0,3,-2},		ColorRGB{1,1,1},	Vector2{0.5f, 0}},
			Vertex{Vector3{3,3,-2},		ColorRGB{1,1,1},	Vector2{1,0}},
			Vertex{Vector3{-3,0,-2},	ColorRGB{1,1,1},	Vector2{0,0.5f}},
			Vertex{Vector3{0,0,-2},		ColorRGB{1,1,1},	Vector2{0.5f,0.5f}},
			Vertex{Vector3{3,0,-2},		ColorRGB{1,1,1},	Vector2{1,0.5f}},
			Vertex{Vector3{-3,-3,-2},	ColorRGB{1,1,1},	Vector2{0,1}},
			Vertex{Vector3{0,-3,-2},	ColorRGB{1,1,1},	Vector2{0.5f,1}},
			Vertex{Vector3{3,-3,-2},	ColorRGB{1,1,1},	Vector2{1,1}}
			},
		{
			3,0,4,1,5,2,
			2,6,
			6,3,7,4,8,5
		},
		PrimitiveTopology::TriangleStrip,
		{},
		Matrix::CreateTranslation(Vector3{0,0,0})
		}
	);


	//Mesh{
	//{
	//Vertex{Vector3{-3,3,-2},	ColorRGB{1,1,1},	Vector2{0,0}},
	//Vertex{Vector3{0,3,-2},		ColorRGB{1,1,1},	Vector2{0.5f, 0}},
	//Vertex{Vector3{3,3,-2},		ColorRGB{1,1,1},	Vector2{1,0}},
	//Vertex{Vector3{-3,0,-2},	ColorRGB{1,1,1},	Vector2{0,0.5f}},
	//Vertex{Vector3{0,0,-2},		ColorRGB{1,1,1},	Vector2{0.5f,0.5f}},
	//Vertex{Vector3{3,0,-2},		ColorRGB{1,1,1},	Vector2{1,0.5f}},
	//Vertex{Vector3{-3,-3,-2},	ColorRGB{1,1,1},	Vector2{0,1}},
	//Vertex{Vector3{0,-3,-2},	ColorRGB{1,1,1},	Vector2{0.5f,1}},
	//Vertex{Vector3{3,-3,-2},	ColorRGB{1,1,1},	Vector2{1,1}}
	//},
	//{
	//	3,0,1,	1,4,3,	4,1,2,
	//	2,5,4,	6,3,4,	4,7,6,
	//	7,4,5,	5,8,7
	//},
	//PrimitiveTopology::TriangleList,
	//{},
	//Matrix::CreateTranslation(Vector3{0,0,0})
	//}

	m_pTexture = m_pTexture->LoadFromFile("resources/UV_grid_2.png");

#elif defined(tuktuk)
	m_MeshesWorld.clear();
	m_MeshesWorld.reserve(1);
	m_MeshesWorld.emplace_back(Mesh{});

	Utils::ParseOBJ("Resources/tuktuk.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);

	const Vector3	position{ /*m_Camera.origin +*/ Vector3{0, 0, 0} /* + Vector3{0, -3, 15} */ };
	const Vector3	scale{ 0.5f, 0.5f, 0.5f };

	m_MeshesWorld[0].worldMatrix = Matrix::CreateTranslation(Vector3{ 0,0,0 });

	m_pTexture = m_pTexture->LoadFromFile("resources/tuktuk.png");

#elif defined(vehicle)
	m_MeshesWorld.clear();
	m_MeshesWorld.reserve(1);
	m_MeshesWorld.emplace_back(Mesh{});

	Utils::ParseOBJ("Resources/vehicle.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);

	const Vector3 position{ 0, 0, 50 };
	//const Vector3 position{ 12.4f, -0.7f, 7.5f };
	constexpr float YRotation{ -PI_DIV_2 };

	m_MeshesWorld[0].worldMatrix = Matrix::CreateRotationY(YRotation) * Matrix::CreateTranslation(position);
	m_MeshOriginalWorldMatrix = m_MeshesWorld[0].worldMatrix;


	m_pTexture = Texture::LoadFromFile("resources/vehicle_diffuse.png");
	m_pNormalMap = Texture::LoadFromFile("resources/vehicle_normal.png");
	m_pSpecularMap = Texture::LoadFromFile("resources/vehicle_specular.png");
	m_pGlossinessMap = Texture::LoadFromFile("resources/vehicle_gloss.png");

#endif
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
	delete m_pNormalMap;
	delete m_pSpecularMap;
	delete m_pGlossinessMap;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_IsSpinning)
	{
		RotateMesh(pTimer->GetElapsed());
	}
	Matrix rotationMatrix{ Matrix::CreateRotationY(m_MeshRotationAngle) };
	m_MeshesWorld[0].worldMatrix = rotationMatrix * m_MeshOriginalWorldMatrix;

}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Fill the array with max float value
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	ClearBackground();


	VertexTransformationFunction(m_MeshesWorld);

	// For every mesh
	for (Mesh& mesh : m_MeshesWorld)
	{
		// For every triangle
		for (int currIdx{}; currIdx < mesh.indices.size(); ++currIdx)
		{
 			int vertexIdx0{};
			int vertexIdx1{};
			int vertexIdx2{};

			// Updating vertexindeces depending on the certain primitiveTopology
			UpdateVerticesUsingPrimTop(mesh, currIdx, vertexIdx0, vertexIdx1, vertexIdx2);


			if (vertexIdx0 == vertexIdx1 || vertexIdx1 == vertexIdx2 || vertexIdx0 == vertexIdx2)
				continue;

			if (!IsInFrustum(mesh.vertices_out[vertexIdx0])
				|| !IsInFrustum(mesh.vertices_out[vertexIdx1])
				|| !IsInFrustum(mesh.vertices_out[vertexIdx2]))
				continue;

			Vertex_Out Vertex0{ NDCToScreen(mesh.vertices_out[vertexIdx0]) };
			Vertex_Out Vertex1{ NDCToScreen(mesh.vertices_out[vertexIdx1]) };
			Vertex_Out Vertex2{ NDCToScreen(mesh.vertices_out[vertexIdx2]) };

			//Setting up some variables
			const Vector2 v0{ Vertex0.position.GetXY() };
			const Vector2 v1{ Vertex1.position.GetXY() };
			const Vector2 v2{ Vertex2.position.GetXY() };

			const float depthV0{ Vertex0.position.z };
			const float depthV1{ Vertex1.position.z };
			const float depthV2{ Vertex2.position.z };

			const float wV0{ Vertex0.position.w };
			const float wV1{ Vertex1.position.w };
			const float wV2{ Vertex2.position.w };

			const Vector2 v0uv{ Vertex0.uv };
			const Vector2 v1uv{ Vertex1.uv };
			const Vector2 v2uv{ Vertex2.uv };

			const Vector2 edge01{ v1 - v0 };
			const Vector2 edge12{ v2 - v1 };
			const Vector2 edge20{ v0 - v2 };

			const float areaTriangle{ fabs(Vector2::Cross(v1 - v0, v2 - v0)) };

			if (areaTriangle <= 0.01f)
			{
				continue;
			}

			// create bounding box for triangle
			const int bottom = std::min(int(std::min(v0.y, v1.y)), int(v2.y));
			const int top = std::max(int(std::max(v0.y, v1.y)), int(v2.y)) + 1;

			const int left = std::min(int(std::min(v0.x, v1.x)), int(v2.x));
			const int right = std::max(int(std::max(v0.x, v1.x)), int(v2.x)) + 1;

			// check if bounding box is in screen
			if (left <= 0 || right >= m_Width - 1)
				continue;

			if (bottom <= 0 || top >= m_Height - 1)
				continue;

			const int offSet{ 1 };

			////RENDER LOGIC
			//for (int px{}; px < m_Width; ++px)
			//{
			//	for (int py{}; py < m_Height; ++py)
			//	{

			for (int px = left - offSet; px < right + offSet; ++px)
			{
				for (int py = bottom - offSet; py < top + offSet; ++py)
				{


					ColorRGB finalColor = colors::Black;

					Vector2 pixel = { float(px), float(py) };

					// Maths to check if the pixel is in the triangle
					const Vector2 directionV0 = pixel - v0;
					const Vector2 directionV1 = pixel - v1;
					const Vector2 directionV2 = pixel - v2;

					// If the code survives all the if and continues, its inside the triangle!
					float weightV2 = Vector2::Cross(edge01, directionV0);
					if (weightV2 < 0)
						continue;

					float weightV0 = Vector2::Cross(edge12, directionV1);
					if (weightV0 < 0)
						continue;

					float weightV1 = Vector2::Cross(edge20, directionV2);
					if (weightV1 < 0)
						continue;

					// Setting up the weights for the UV coordinates
					weightV0 /= areaTriangle;
					weightV1 /= areaTriangle;
					weightV2 /= areaTriangle;

					// Create BufferValue
					const float ZBufferVal{
						1.f /
						((1 / depthV0) * weightV0 +
						(1 / depthV1) * weightV1 +
						(1 / depthV2) * weightV2)
					};

					// Check if there is no triangle in front of this triangle
					if (ZBufferVal > m_pDepthBufferPixels[px * m_Height + py])
						continue;

					// Add BufferValue to the array
					m_pDepthBufferPixels[px * m_Height + py] = ZBufferVal;


					// Visualize what is requested by user
					switch (m_Visualize)
					{
					case Visualize::FinalColor:
					{
						// Interpolating all atributes
						// for shading we use world coordinates

						const Vertex_Out v0_world{ mesh.vertices_out[vertexIdx0] };
						const Vertex_Out v1_world{ mesh.vertices_out[vertexIdx1] };
						const Vertex_Out v2_world{ mesh.vertices_out[vertexIdx2] };

						const float interpolatedWDepth = {
							1.f /
							((1 / v0_world.position.w) * weightV0 +
							(1 / v1_world.position.w) * weightV1 +
							(1 / v2_world.position.w) * weightV2)
						};

						const Vector2 interpolatedUV = {
							((v0_world.uv / v0_world.position.w) * weightV0 +
							(v1_world.uv / v1_world.position.w) * weightV1 +
							(v2_world.uv / v2_world.position.w) * weightV2) * interpolatedWDepth
						};

						Vector3 interpolatedNormal = {
							((v0_world.normal / v0_world.position.w) * weightV0 +
							(v1_world.normal / v1_world.position.w) * weightV1 +
							(v2_world.normal / v2_world.position.w) * weightV2) * interpolatedWDepth
						};
						interpolatedNormal.Normalize();

						Vector3 interpolatedTangent = {
							((v0_world.tangent / v0_world.position.w) * weightV0 +
							(v1_world.tangent / v1_world.position.w) * weightV1 +
							(v2_world.tangent / v2_world.position.w) * weightV2) * interpolatedWDepth
						};
						interpolatedTangent.Normalize();

						Vector3 interpolatedViewDirection = {
							((v0_world.viewDirection / v0_world.position.w) * weightV0 +
							(v1_world.viewDirection / v1_world.position.w) * weightV1 +
							(v2_world.viewDirection / v2_world.position.w) * weightV2) * interpolatedWDepth
						};
						interpolatedViewDirection.Normalize();

						//Interpolated Vertex Attributes for Pixel
						Vertex_Out pixelVertex;
						pixelVertex.position = Vector4{ pixel.x, pixel.y, ZBufferVal, interpolatedWDepth };
						pixelVertex.color = ColorRGB{0,0,0};
						pixelVertex.uv = interpolatedUV;
						pixelVertex.normal = interpolatedNormal;
						pixelVertex.tangent = interpolatedTangent;
						pixelVertex.viewDirection = interpolatedViewDirection;

						finalColor = PixelShading(pixelVertex);

						break;
					}
					case Visualize::DepthBuffer:
					{
						constexpr float depthRemapSize{ 0.005f };

						float remapedBufferVal{ ZBufferVal };
						DepthRemap(remapedBufferVal, depthRemapSize);
						finalColor = ColorRGB{ remapedBufferVal, remapedBufferVal , remapedBufferVal };
						break;
					}
					}

					//Update Color in Buffer
					finalColor.MaxToOne();

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
				}
			}
		}
	}


	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::ToggleSpinning()
{
	m_IsSpinning = !m_IsSpinning;
}

void dae::Renderer::ToggleNormalMap()
{
	m_UseNormalMap = !m_UseNormalMap;
}

void dae::Renderer::CycleShadingMode()
{
	switch (m_ShadingMode)
	{
	case ShadingMode::ObservedArea:
		m_ShadingMode = ShadingMode::Diffuse;
		break;
	case ShadingMode::Diffuse:
		m_ShadingMode = ShadingMode::Specular;
		break;
	case ShadingMode::Specular:
		m_ShadingMode = ShadingMode::Combined;
		break;
	case ShadingMode::Combined:
		m_ShadingMode = ShadingMode::ObservedArea;
		break;
	}
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& mesh_in) const
{
	for (Mesh& mesh : mesh_in)
	{
		//Todo > W1 Projection Stage COMPLETED
		mesh.vertices_out.clear();
		mesh.vertices_out.reserve(mesh.vertices.size());

		Matrix worldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

		for (const Vertex& vtx : mesh.vertices)
		{

			Vertex_Out vertexOut{};
			
			// to NDC-Space
			vertexOut.position = worldViewProjectionMatrix.TransformPoint({ vtx.position, 1.0f });

			// The viewdirection is just the coordinates of the vertex after being transformed to viewspace
			vertexOut.viewDirection = vertexOut.position.GetXYZ();
			vertexOut.viewDirection.Normalize();

			vertexOut.position.x /= vertexOut.position.w;
			vertexOut.position.y /= vertexOut.position.w;
			vertexOut.position.z /= vertexOut.position.w;

			vertexOut.color = vtx.color;
			vertexOut.uv = vtx.uv;
			vertexOut.normal = mesh.worldMatrix.TransformVector(vtx.normal);
			vertexOut.normal.Normalize();
			vertexOut.tangent = mesh.worldMatrix.TransformVector(vtx.tangent);
			vertexOut.tangent.Normalize();

			mesh.vertices_out.emplace_back(vertexOut);
		
		}
	}
}


bool Renderer::IsPixelInBoundingBoxOfTriangle(int px, int py, const Vector2& v0, const Vector2& v1, const Vector2& v2) const
{
	const Vector2 bottomLeft{ std::min(std::min(v0.x, v1.x), v2.x),  std::min(std::min(v0.y, v1.y), v2.y) };

	const Vector2 topRight{ std::max(std::max(v0.x, v1.x), v2.x),  std::max(std::max(v0.y, v1.y), v2.y) };

	if (px <= topRight.x && px >= bottomLeft.x)
	{
		if (py <= topRight.y && py >= bottomLeft.y)
		{
			return true;
		}
	}

	return false;
}

Vertex_Out Renderer::NDCToScreen(const Vertex_Out& vtx) const
{
	Vertex_Out vertex{ vtx };
	vertex.position.x = (vtx.position.x + 1.f) * 0.5f * float(m_Width);
	vertex.position.y = (1.f - vtx.position.y) * 0.5f * float(m_Height);
	return vertex;
}

bool Renderer::IsInFrustum(const Vertex_Out & vtx)
{
	if (vtx.position.x < -1 || vtx.position.x > 1)
		return false;

	if (vtx.position.y < -1 || vtx.position.y > 1)
		return  false;

	if (vtx.position.z < 0 || vtx.position.z > 1)
		return  false;

	return true;
}

void dae::Renderer::UpdateVerticesUsingPrimTop(const Mesh& mesh, int& currIdx, int& vertexIdx0, int& vertexIdx1, int& vertexIdx2)
{
	if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
	{
		vertexIdx0 = mesh.indices[currIdx];
		vertexIdx1 = mesh.indices[currIdx + 1];
		vertexIdx2 = mesh.indices[currIdx + 2];
		currIdx += 2;
	}
	else if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
	{
		vertexIdx0 = mesh.indices[currIdx];

		if (currIdx % 2 == 0)
		{
			vertexIdx1 = mesh.indices[currIdx + 1];
			vertexIdx2 = mesh.indices[currIdx + 2];
		}
		else
		{
			vertexIdx1 = mesh.indices[currIdx + 2];
			vertexIdx2 = mesh.indices[currIdx + 1];
		}

		if (currIdx + 3 >= mesh.indices.size())
			currIdx += 2;
	}
}

void Renderer::DepthRemap(float& depth, float topPercentile) const
{
	depth = (depth - (1.f - topPercentile)) / topPercentile;

	depth = std::max(0.f, depth);
	depth = std::min(1.f, depth);
}

ColorRGB Renderer::PixelShading(Vertex_Out v) const
{
	// Light settings
	Vector3 lightDirection{ 0.577f, -0.577f, 0.577f };
	lightDirection.Normalize();
	constexpr float lightIntensity{ 7.f };
	constexpr float specularShininess{ 25.f };


	if (m_UseNormalMap)
	{	
		const Vector3 biNormal = Vector3::Cross(v.normal, v.tangent);
		const Matrix tangentSpaceAxis = { v.tangent, biNormal, v.normal, Vector3::Zero };

		const ColorRGB normalColor = m_pNormalMap->Sample(v.uv);
		Vector3 sampledNormal = { normalColor.r, normalColor.g, normalColor.b };
		sampledNormal = 2.f * sampledNormal - Vector3{ 1.f, 1.f, 1.f };

		sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal);

		v.normal = sampledNormal.Normalized();
	}


	// OBSERVED AREA
	float ObservedArea{ Vector3::Dot(v.normal,  -lightDirection)};
	ObservedArea = std::max(ObservedArea, 0.f);
	
	const ColorRGB observedAreaRGB{ ObservedArea ,ObservedArea ,ObservedArea };

	// DIFFUSE
	const ColorRGB TextureColor{ m_pTexture->Sample(v.uv) };

	// SPECULAR
	const Vector3 reflect{ Vector3::Reflect(-lightDirection, v.normal) };
	float cosAlpha{ Vector3::Dot(reflect, v.viewDirection) };
	cosAlpha = std::max(0.f, cosAlpha);
	

	const float specularExp{ specularShininess * m_pGlossinessMap->Sample(v.uv).r };

	const ColorRGB specular{ m_pSpecularMap->Sample(v.uv) * powf(cosAlpha, specularExp) };

	ColorRGB finalColor{ 0,0,0 };

	switch(m_ShadingMode)
	{
	case ShadingMode::ObservedArea:
	{
		finalColor += observedAreaRGB;
		break;
	}
	case ShadingMode::Diffuse:
	{
		finalColor +=  lightIntensity * observedAreaRGB * TextureColor / PI;
		break;
	}
	case ShadingMode::Specular:
	{

		finalColor += specular;// *observedAreaRGB;
		break;
	}
	case ShadingMode::Combined:
	{
		finalColor += lightIntensity * observedAreaRGB * TextureColor / PI;
		finalColor += specular;
		break;
	}
	}

	constexpr ColorRGB ambient{ 0.025f, 0.025f, 0.025f };

	//finalColor += ambient;

	finalColor.MaxToOne();

	return finalColor;
}

void Renderer::RotateMesh(float elapsedSec)
{
	constexpr float rotationSpeed{ 1.f };
	m_MeshRotationAngle += rotationSpeed * elapsedSec;
}

void dae::Renderer::ClearBackground() const
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::CycleVisualization()
{
	switch (m_Visualize)
	{
	case Visualize::FinalColor:
		m_Visualize = Visualize::DepthBuffer;
		break;
	case Visualize::DepthBuffer:
		m_Visualize = Visualize::FinalColor;
		break;
	}
}