#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		Texture* text{ new Texture{IMG_Load(path.c_str())} };
		return text;
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv
		const int x = int(uv.x * m_pSurface->w);
		const int y = int(uv.y * m_pSurface->h);

		Uint8 r{ };
		Uint8 g{ };
		Uint8 b{ };

		const int idx{ int(x + (y * m_pSurface->w)) };

		SDL_GetRGB(m_pSurfacePixels[idx], m_pSurface->format, &r, &g, &b);

		ColorRGB kleur{ r/255.f, g / 255.f, b / 255.f };
		return kleur;
	}
}