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

using namespace dae;

#define tuktuk
//#define UV_grid

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
	m_Camera.Initialize(float(m_Width) / m_Height, 60.f, { 0.f, 5.f, -30.f });

	//Spin mesh?
	m_IsSpinning = true;

	// DEFINE MESH
#if defined(tuktuk)
	m_MeshesWorld.clear();
	m_MeshesWorld.reserve(1);
	m_MeshesWorld.emplace_back(Mesh{});

	Utils::ParseOBJ("Resources/tuktuk.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);

	const Vector3	position{ /*m_Camera.origin +*/ Vector3{0, 0, 0} /* + Vector3{0, -3, 15} */};
	const Vector3	scale{ 0.5f, 0.5f, 0.5f };

	m_MeshesWorld[0].worldMatrix = Matrix::CreateTranslation(Vector3{ 0,0,0 });

	m_pTexture = m_pTexture->LoadFromFile("resources/tuktuk.png");

#elif defined(UV_grid)
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
#endif
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_IsSpinning)
	{
		RotateMesh(pTimer->GetElapsed());
	}
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

	for (Mesh& mesh : m_MeshesWorld)
	{
		for (int currIdx{}; currIdx < mesh.indices.size(); ++currIdx)
		{
 			int vertexIdx0{};
			int vertexIdx1{};
			int vertexIdx2{};

			// Updating vertexindeces depending on the certain primitiveTopology
			UpdateVerticesUsingPrimTop(mesh, currIdx, vertexIdx0, vertexIdx1, vertexIdx2);
			//vertexIdx0 = mesh.indices[currIdx];
			//vertexIdx1 = mesh.indices[currIdx + 1];
			//vertexIdx2 = mesh.indices[currIdx + 2];
			//currIdx += 2;


			if (vertexIdx0 == vertexIdx1 || vertexIdx1 == vertexIdx2 || vertexIdx0 == vertexIdx2)
				continue;

			if (!IsInFrustum(mesh.vertices_out[vertexIdx0])
				|| !IsInFrustum(mesh.vertices_out[vertexIdx1])
				|| !IsInFrustum(mesh.vertices_out[vertexIdx2]))
				continue;

			Vertex_Out Vertex1{ NDCToScreen(mesh.vertices_out[vertexIdx0]) };
			Vertex_Out Vertex2{ NDCToScreen(mesh.vertices_out[vertexIdx1]) };
			Vertex_Out Vertex3{ NDCToScreen(mesh.vertices_out[vertexIdx2]) };

			//Setting up some variables
			const Vector2 v0{ Vertex1.position.GetXY() };
			const Vector2 v1{ Vertex2.position.GetXY() };
			const Vector2 v2{ Vertex3.position.GetXY() };

			const float depthV0{ Vertex1.position.z };
			const float depthV1{ Vertex2.position.z };
			const float depthV2{ Vertex3.position.z };

			const float wV0{ Vertex1.position.w };
			const float wV1{ Vertex2.position.w };
			const float wV2{ Vertex3.position.w };

			const Vector2 v0uv{ Vertex1.uv };
			const Vector2 v1uv{ Vertex2.uv };
			const Vector2 v2uv{ Vertex3.uv };

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

					//// This continue should never get called but it is just a safety check
					//if (weightV0 + weightV1 + weightV2 < 1 - 4 * FLT_EPSILON	// 4 Times epsilon because 3 floats get counted up 
					//	|| weightV0 + weightV1 + weightV2 > 1 + 4 * FLT_EPSILON)// so worst case scenario the error is 3 times epsilon size
					//	continue;												// (added 1 for actual epsilon on top of error size)

					const float ZBufferVal{
						1.f /
						((1 / depthV0) * weightV0 +
						(1 / depthV1) * weightV1 +
						(1 / depthV2) * weightV2)
					};

					//// safety check
					//if (ZBufferVal < 0 || ZBufferVal > 1)
					//	continue;

					// Check if there is no triangle in front of this triangle
					if (ZBufferVal > m_pDepthBufferPixels[px * m_Height + py])
						continue;

					m_pDepthBufferPixels[px * m_Height + py] = ZBufferVal;


					switch (m_Visualize)
					{
					case Visualize::FinalColor:
					{
						// Maths for sampling the UV coordinates and color
						const float interpolatedWDepthWeight = {
							1.f /
							((1 / wV0) * weightV0 +
							(1 / wV1) * weightV1 +
							(1 / wV2) * weightV2)
						};

						const Vector2 currUV = {
							((v0uv / wV0) * weightV0 +
							(v1uv / wV1) * weightV1 +
							(v2uv / wV2) * weightV2) * interpolatedWDepthWeight
						};

						finalColor = m_pTexture->Sample(currUV);
						break;
					}
					case Visualize::DepthBuffer:
					{
						const float depthRemapSize{ 0.005f };

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

			vertexOut.position.x /= vertexOut.position.w;
			vertexOut.position.y /= vertexOut.position.w;
			vertexOut.position.z /= vertexOut.position.w;

			vertexOut.color = vtx.color;
			vertexOut.normal = vtx.normal;
			vertexOut.uv = vtx.uv;
			vertexOut.tangent = vtx.tangent;

			mesh.vertices_out.emplace_back(vertexOut);
		
		}
	}
}


bool Renderer::IsPixelInBoundingBoxOfTriangle(int px, int py, const Vector2& v0, const Vector2& v1, const Vector2& v2) const
{
	Vector2 bottomLeft{ std::min(std::min(v0.x, v1.x), v2.x),  std::min(std::min(v0.y, v1.y), v2.y) };

	Vector2 topRight{ std::max(std::max(v0.x, v1.x), v2.x),  std::max(std::max(v0.y, v1.y), v2.y) };

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

bool Renderer::IsInFrustum(const Vertex_Out & vtx) const
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

void Renderer::DepthRemap(float& depth, float topPercentile)
{
	depth = (depth - (1.f - topPercentile)) / topPercentile;

	depth = std::max(0.f, depth);
	depth = std::min(1.f, depth);
}

void Renderer::RotateMesh(float elapsedSec)
{
	const float rotationSpeed{ 0.5f };
	m_MeshRotationAngle += rotationSpeed * elapsedSec;
	Matrix rotationMatrix{ Matrix::CreateRotationY(m_MeshRotationAngle) };
	m_MeshesWorld[0].worldMatrix = rotationMatrix;
}

void dae::Renderer::ClearBackground() const
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0));
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