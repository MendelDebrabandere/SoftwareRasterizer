//External includes
#include "SDL.h"
#include "SDL_surface.h"

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

	//Define triangle - Vertices in NDC space
	std::vector<Vertex> vertices_world
	{
		//Triangle 0
		{{0.f, 2.f, 0.f}, {1,0,0}},
		{{1.5f, -1.f, 0.f}, {1,0,0}},
		{{-1.5f, -1.f, 0.f}, {1,0,0}},
		

		//Triangle 1
		{{0.f, 4.f, 2.f}, {1,0,0}},
		{{3.f, -2.f, 2.f}, {0,1,0}},
		{{-3.f, -2.f, 2.f}, {0,0,1}}
	};

	std::vector<Vertex> vertices_screen{};

	VertexTransformationFunction(vertices_world, vertices_screen);

	for (int vertexIdx{}; vertexIdx < vertices_screen.size(); vertexIdx += 3)
	{
		const Vector2 v0{ vertices_screen[vertexIdx].position.x ,vertices_screen[vertexIdx].position.y };
		const Vector2 v1{ vertices_screen[vertexIdx + 1].position.x ,vertices_screen[vertexIdx + 1].position.y };
		const Vector2 v2{ vertices_screen[vertexIdx + 2].position.x ,vertices_screen[vertexIdx + 2].position.y };

		const Vector2 edge01 = v1 - v0;
		const Vector2 edge12 = v2 - v1;
		const Vector2 edge20 = v0 - v2;

		const float areaTriangle{ Vector2::Cross(v1 - v0, v2 - v0) };

		const ColorRGB colorV0{ vertices_screen[vertexIdx].color };
		const ColorRGB colorV1{ vertices_screen[vertexIdx + 1].color };
		const ColorRGB colorV2{ vertices_screen[vertexIdx + 2].color };


		//RENDER LOGIC
		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				if (!IsPixelInBoundingBoxOfTriangle(px, py, v0, v1, v2))
					continue;


				ColorRGB finalColor = colors::Black;

				Vector2 pixel = { (float)px,(float)py };


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
					weightV0 * vertices_screen[vertexIdx].position.z +
					weightV1 * vertices_screen[vertexIdx + 1].position.z +
					weightV2 * vertices_screen[vertexIdx + 2].position.z
				};

				if (depthWeight > m_pDepthBufferPixels[px * m_Height + py])
					continue;

				m_pDepthBufferPixels[px* m_Height + py] = depthWeight;

				finalColor = colorV0 * weightV0 + colorV1 * weightV1 + colorV2 * weightV2;

				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
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
	vertices_out.reserve(vertices_in.size());

	for (Vertex vertex : vertices_in)
	{
		// to view space
		vertex.position = m_Camera.viewMatrix.TransformPoint(vertex.position);

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
