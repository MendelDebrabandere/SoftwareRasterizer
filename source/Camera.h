#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right);

			invViewMatrix = Matrix
			{
				right,
				up,
				forward,
				origin
			};

			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			//Camera Update Logic
			//...

			const float deltaTime = pTimer->GetElapsed();

			const float movementSpeed = 5.f;
			const float rotationSpeed = 0.003f;

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			if (pKeyboardState[SDL_SCANCODE_W])
			{
				this->origin += movementSpeed * deltaTime * this->forward;
			}
			if (pKeyboardState[SDL_SCANCODE_S])
			{
				this->origin -= movementSpeed * deltaTime * this->forward;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				this->origin -= movementSpeed * deltaTime * this->right;
			}
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				this->origin += movementSpeed * deltaTime * this->right;
			}
			if (pKeyboardState[SDL_SCANCODE_Q])
			{
				this->origin -= movementSpeed * deltaTime * this->up;
			}
			if (pKeyboardState[SDL_SCANCODE_E])
			{
				this->origin += movementSpeed * deltaTime * this->up;
			}

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if (mouseState & SDL_BUTTON_RMASK && mouseState & SDL_BUTTON_LMASK)
			{
				this->origin += movementSpeed * deltaTime * Vector3 { 0, 1, 0 } *mouseY;
			}
			else if (mouseState & SDL_BUTTON_RMASK)
			{
				totalYaw += rotationSpeed * mouseX;
				totalPitch -= rotationSpeed * mouseY;
				if (abs(totalYaw) >= float(M_PI) / 2.f)
				{
					if (totalYaw < 0)
					{
						totalYaw = -float(M_PI) / 2.001f;
					}
					else
					{
						totalYaw = float(M_PI) / 2.001f;
					}
				}
			}
			else if (mouseState & SDL_BUTTON_LMASK)
			{
				totalPitch -= rotationSpeed * mouseX;
				this->origin -= movementSpeed * deltaTime * this->forward * mouseY;
			}

			Matrix finalRotation{ Matrix::CreateRotation(totalPitch, totalYaw, 0) };

			forward = finalRotation.TransformVector(Vector3::UnitZ);
			forward.Normalize();




			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
