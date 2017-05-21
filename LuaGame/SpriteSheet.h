#pragma once

#include "pch.h"

#include <fstream>
#include <map>

#include <SpriteBatch.h>

class SpriteSheet
{
public:
	struct SpriteFrame
	{
		RECT                sourceRect;
		DirectX::XMFLOAT2   size;
		DirectX::XMFLOAT2   origin;
		bool                rotated;
	};

	void Load(ID3D11ShaderResourceView* texture, const wchar_t* szFileName)
	{
		mSprites.clear();

		mTexture = texture;

		if (szFileName)
		{
			//
			// This code parses the 'MonoGame' project txt file that is produced
			// by CodeAndWeb's TexturePacker.
			// https://www.codeandweb.com/texturepacker
			//
			// You can modify it to match whatever sprite-sheet tool you are
			// using
			//

			std::wifstream inFile(szFileName);
			if (!inFile)
				throw std::exception("SpriteSheet failed to load .txt data");

			wchar_t strLine[1024];
			for (;;)
			{
				inFile >> strLine;
				if (!inFile)
					break;

				if (0 == wcscmp(strLine, L"#"))
				{
					// Comment
				}
				else
				{
					static const wchar_t* delim = L";\n";

					wchar_t* context = nullptr;
					wchar_t* name = wcstok_s(strLine, delim, &context);
					if (!name || !*name)
						throw std::exception();

					if (mSprites.find(name) != mSprites.cend())
						throw std::exception();

					wchar_t* str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();

					SpriteFrame frame;
					frame.rotated = (_wtoi(str) == 1);

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					frame.sourceRect.left = _wtol(str);

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					frame.sourceRect.top = _wtol(str);

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					LONG dx = _wtol(str);
					frame.sourceRect.right = frame.sourceRect.left + dx;

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					LONG dy = +_wtol(str);
					frame.sourceRect.bottom = frame.sourceRect.top + dy;

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					frame.size.x = static_cast<float>(_wtof(str));

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					frame.size.y = static_cast<float>(_wtof(str));

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					float pivotX = static_cast<float>(_wtof(str));

					str = wcstok_s(nullptr, delim, &context);
					if (!str)
						throw std::exception();
					float pivotY = static_cast<float>(_wtof(str));

					if (frame.rotated)
					{
						frame.origin.x = dx * (1.f - pivotY);
						frame.origin.y = dy * pivotX;
					}
					else
					{
						frame.origin.x = dx * pivotX;
						frame.origin.y = dy * pivotY;
					}

					mSprites.insert(std::pair<std::wstring, SpriteFrame>(
						std::wstring(name), frame));
				}

				inFile.ignore(1000, '\n');
			}
		}
	}

	const SpriteFrame* Find(const wchar_t* name) const
	{
		auto it = mSprites.find(name);
		if (it == mSprites.cend())
			return nullptr;

		return &it->second;
	}

	void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame,
		DirectX::XMFLOAT4 const& rect,
		DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0) const
	{
		assert(batch != 0);
		using namespace DirectX;

		if (frame.rotated)
		{
			rotation -= XM_PIDIV2;
		}

		XMFLOAT2 origin = XMFLOAT2(rect.z / 2.f, rect.w / 2.f);

		RECT dest;
		dest.left = rect.x;
		dest.top = rect.y;

		dest.right = rect.x+rect.z;
		dest.bottom = rect.y+rect.w;


		batch->Draw(mTexture.Get(), dest, &frame.sourceRect, color, rotation);
	}

	/*void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame,
		DirectX::XMFLOAT2 const& position,
		DirectX::FXMVECTOR color, float rotation, DirectX::XMFLOAT2 const& scale,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layerDepth = 0) const
	{

	}

	// Draw overloads specifying position and scale via the first two components of
	// an XMVECTOR.
	void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame,
		DirectX::FXMVECTOR position,
		DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0,
		float scale = 1,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layerDepth = 0) const
	{
		...
	}

	void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame,
		DirectX::FXMVECTOR position,
		DirectX::FXMVECTOR color, float rotation, DirectX::GXMVECTOR scale,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layerDepth = 0) const
	{
		...
	}

	// Draw overloads specifying position as a RECT.
	void XM_CALLCONV Draw(DirectX::SpriteBatch* batch, const SpriteFrame& frame,
		RECT const& destinationRectangle,
		DirectX::FXMVECTOR color = DirectX::Colors::White, float rotation = 0,
		DirectX::SpriteEffects effects = DirectX::SpriteEffects_None,
		float layerDepth = 0) const
	{
		...
	}*/

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mTexture;
	std::map<std::wstring, SpriteFrame>                 mSprites;
};