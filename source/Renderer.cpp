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

	m_AspectRatio = float(m_Width) / m_Height;

	//Initialize Camera
	m_Camera.Initialize(60.f, { 0.f, 0.f, -10.f });

	m_pTexture = m_pTexture->LoadFromFile("resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	// Fill the array with max float value
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	ClearBackground();

	// Define Mesh
	std::vector<Mesh> meshes_world
	{
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
		PrimitiveTopology::TriangleStrip
		}
	};
	//std::vector<Mesh> meshes_world
	//{
	//	Mesh{
	//		{
	//		Vertex{Vector3{-3,3,-2},	ColorRGB{1,1,1},	Vector2{0,0}},
	//		Vertex{Vector3{0,3,-2},		ColorRGB{1,1,1},	Vector2{0.5f, 0}},
	//		Vertex{Vector3{3,3,-2},		ColorRGB{1,1,1},	Vector2{1,0}},
	//		Vertex{Vector3{-3,0,-2},	ColorRGB{1,1,1},	Vector2{0,0.5f}},
	//		Vertex{Vector3{0,0,-2},		ColorRGB{1,1,1},	Vector2{0.5f,0.5f}},
	//		Vertex{Vector3{3,0,-2},		ColorRGB{1,1,1},	Vector2{1,0.5f}},
	//		Vertex{Vector3{-3,-3,-2},	ColorRGB{1,1,1},	Vector2{0,1}},
	//		Vertex{Vector3{0,-3,-2},	ColorRGB{1,1,1},	Vector2{0.5f,1}},
	//		Vertex{Vector3{3,-3,-2},	ColorRGB{1,1,1},	Vector2{1,1}}
	//		},
	//	{
	//		3,0,1,	1,4,3,	4,1,2,
	//		2,5,4,	6,3,4,	4,7,6,
	//		7,4,5,	5,8,7
	//	},
	//	PrimitiveTopology::TriangleList
	//	}
	//};
	std::vector<Mesh> meshes_camera = meshes_world;

	VertexTransformationFunction(meshes_world, meshes_camera);

	for (const Mesh& mesh : meshes_camera)
	{
		for (int currIdx{}; currIdx < mesh.indices.size(); ++currIdx)
		{
			int vertexIdx0{};
			int vertexIdx1{};
			int vertexIdx2{};


			if (meshes_world[0].primitiveTopology == PrimitiveTopology::TriangleList)
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
			else
			{
				assert(false, "non existing PrimitiveTopology");
			}

			const Vector2 v0 = mesh.vertices[vertexIdx0].position.GetXY();
			const Vector2 v1 = mesh.vertices[vertexIdx1].position.GetXY();
			const Vector2 v2 = mesh.vertices[vertexIdx2].position.GetXY();

			//const ColorRGB colorV0{ mesh.vertices[vertexIdx0].color };
			//const ColorRGB colorV1{ mesh.vertices[vertexIdx1].color };
			//const ColorRGB colorV2{ mesh.vertices[vertexIdx2].color };

			const Vector2 v0uv = mesh.vertices[vertexIdx0].uv;
			const Vector2 v1uv = mesh.vertices[vertexIdx1].uv;
			const Vector2 v2uv = mesh.vertices[vertexIdx2].uv;

			const Vector2 edge01 = v1 - v0;
			const Vector2 edge12 = v2 - v1;
			const Vector2 edge20 = v0 - v2;

			const float areaTriangle{ Vector2::Cross(v1 - v0, v2 - v0) };

			if (areaTriangle <= 0.0001f)
				continue;


			//RENDER LOGIC
			for (int px{}; px < m_Width; ++px)
			{
				for (int py{}; py < m_Height; ++py)
				{
					if (!IsPixelInBoundingBoxOfTriangle(px, py, v0, v1, v2))
						continue;


					ColorRGB finalColor = colors::Black;

					Vector2 pixel = { float(px), float(py) };


					const Vector2 directionV0 = pixel - v0;
					const Vector2 directionV1 = pixel - v1;
					const Vector2 directionV2 = pixel - v2;

					float weightV2 = Vector2::Cross(edge01, directionV0);
					if (weightV2 < 0)
						continue;

					float weightV0 = Vector2::Cross(edge12, directionV1);
					if (weightV0 < 0)
						continue;

					float weightV1 = Vector2::Cross(edge20, directionV2);
					if (weightV1 < 0)
						continue;

					weightV0 /= areaTriangle;
					weightV1 /= areaTriangle;
					weightV2 /= areaTriangle;

					const float depthWeight =
					{
						weightV0 * meshes_world[0].vertices[vertexIdx0].position.z +
						weightV1 * meshes_world[0].vertices[vertexIdx1].position.z +
						weightV2 * meshes_world[0].vertices[vertexIdx2].position.z
					};

					if (depthWeight > m_pDepthBufferPixels[px * m_Height + py])
						continue;

					m_pDepthBufferPixels[px * m_Height + py] = depthWeight;

					//finalColor = colorV0 * weightV0 + colorV1 * weightV1 + colorV2 * weightV2;

					const float Z0{ mesh.vertices[vertexIdx0].position.z };
					const float Z1{ mesh.vertices[vertexIdx1].position.z };
					const float Z2{ mesh.vertices[vertexIdx2].position.z };
					const float Zinterpolated{ 1 / (1 / Z0 * weightV0 + 1 / Z1 * weightV1 + 1 / Z2 * weightV2) };
					//Vector2 currUV{ v0uv * weightV0 + v1uv * weightV1 + v2uv * weightV2 };
					Vector2 currUV{(v0uv/ Z0 * weightV0 + v1uv / Z1 * weightV1 + v2uv / Z2 * weightV2) * Zinterpolated };
					finalColor = m_pTexture->Sample(currUV);

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

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	vertices_out.clear();
	vertices_out.reserve(vertices_in.size());

	for (Vertex vertex : vertices_in)
	{
		// to view space
		vertex.position = Vector4{ m_Camera.viewMatrix.TransformPoint(vertex.position), 1 };

		// to projection space
		vertex.position.x /= vertex.position.z;
		vertex.position.y /= vertex.position.z;

		vertex.position.x /= (m_Camera.fov * m_AspectRatio);
		vertex.position.y /= m_Camera.fov;

		// to screen/raster space
		vertex.position.x = (vertex.position.x + 1) / 2.f * m_Width;
		vertex.position.y = (1 - vertex.position.y) / 2.f * m_Height;

		vertices_out.push_back(vertex);
	}

}

void Renderer::VertexTransformationFunction(const std::vector<Mesh>& mesh_in, std::vector<Mesh>& mesh_out) const
{
	//mesh_out.clear();
	//mesh_out.reserve(mesh_in.size());

	for (int i{}; i < mesh_in.size(); ++i)
	{
		VertexTransformationFunction(mesh_in[i].vertices, mesh_out[i].vertices);
	}
}


bool dae::Renderer::IsPixelInBoundingBoxOfTriangle(int px, int py, const Vector2& v0, const Vector2& v1, const Vector2& v2) const
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

void dae::Renderer::ClearBackground() const
{
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
