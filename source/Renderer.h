#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void CycleVisualization();
	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		std::vector<Mesh> m_MeshesWorld;

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};
		Texture* m_pTexture;

		int m_Width{};
		int m_Height{};

		float m_MeshRotationAngle{};
		bool m_IsSpinning{};

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(std::vector<Mesh>& mesh_in) const;

		bool IsPixelInBoundingBoxOfTriangle(int px, int py, const Vector2& v0, const Vector2& v1, const Vector2& v2) const;
		Vertex_Out NDCToScreen(const Vertex_Out& vtx) const;
		bool IsInFrustum(const Vertex_Out& vtx) const;
		void UpdateVerticesUsingPrimTop(const Mesh& mesh, int& currIdx, int& vertexIdx0, int& vertexIdx1, int& vertexIdx2);
		void DepthRemap(float& depth, float topPercentile);

		void RotateMesh(float elapsedSec);

		void ClearBackground() const;


		enum class Visualize {
			FinalColor = 0,
			DepthBuffer = 1
		};

		Visualize m_Visualize{ Visualize::FinalColor };

	};
}
