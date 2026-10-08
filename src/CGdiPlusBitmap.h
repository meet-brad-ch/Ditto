#pragma once

#include <memory>

class CGdiPlusBitmap
{
public:
	/** @brief The loaded bitmap (owned); empty when nothing is loaded. */
	std::unique_ptr<Gdiplus::Bitmap> m_pBitmap{};

public:
	CGdiPlusBitmap()							{ }
	virtual ~CGdiPlusBitmap()					{ Empty(); }

	/** @brief Frees the loaded bitmap. */
	void Empty()								{ m_pBitmap.reset(); }

	/** @brief The loaded bitmap (non-owning), NULL when nothing is loaded. */
	operator Gdiplus::Bitmap*() const			{ return m_pBitmap.get(); }
};


class CGdiPlusBitmapResource : public CGdiPlusBitmap
{
protected:
	HGLOBAL m_hBuffer;

	/**
	 * @brief Finds a resource and locks its data.
	 * @param pName the resource name.
	 * @param pType the resource type.
	 * @param hInst the module holding the resource.
	 * @param imageSize receives the size of the data in bytes.
	 * @return the locked data, or NULL when the resource is missing, empty or cannot be loaded.
	 */
	static const void* LockResourceData(LPCTSTR pName, LPCTSTR pType, HMODULE hInst, DWORD& imageSize);

public:
	CGdiPlusBitmapResource()					{ m_hBuffer = NULL; }
	CGdiPlusBitmapResource(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(pName, pType, hInst); }
	CGdiPlusBitmapResource(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(id, pType, hInst); }
	CGdiPlusBitmapResource(UINT id, UINT type, HMODULE hInst = NULL)
												{ m_hBuffer = NULL; Load(id, type, hInst); }
	virtual ~CGdiPlusBitmapResource()			{ Empty(); }

	void Empty();

	bool Load(LPCTSTR pName, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL);
	bool Load(UINT id, LPCTSTR pType = RT_RCDATA, HMODULE hInst = NULL)
												{ return Load(MAKEINTRESOURCE(id), pType, hInst); }
	bool Load(UINT id, UINT type, HMODULE hInst = NULL)
												{ return Load(MAKEINTRESOURCE(id), MAKEINTRESOURCE(type), hInst); }

	bool LoadRaw(unsigned char* bitmapData, int imageSize) 
	{
		Empty();

		m_hBuffer  = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
		if (m_hBuffer)
		{
			void* pBuffer = ::GlobalLock(m_hBuffer);
			if (pBuffer)
			{
				CopyMemory(pBuffer, bitmapData, imageSize);

				IStream* pStream = NULL;
				if (::CreateStreamOnHGlobal(m_hBuffer, FALSE, &pStream) == S_OK)
				{
					m_pBitmap.reset(Gdiplus::Bitmap:: FromStream(pStream));
					pStream->Release();
					if (m_pBitmap)
					{
						if (m_pBitmap->GetLastStatus() == Gdiplus::Ok)
							return true;

						m_pBitmap.reset();
					}
				}
				::GlobalUnlock(m_hBuffer);
			}
			::GlobalFree(m_hBuffer);
			m_hBuffer = NULL;
		}
		return false;
	}
};

inline
void CGdiPlusBitmapResource::Empty()
{
	CGdiPlusBitmap::Empty();
	if (m_hBuffer)
	{
		::GlobalUnlock(m_hBuffer);
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	} 
}

inline
const void* CGdiPlusBitmapResource::LockResourceData(LPCTSTR pName, LPCTSTR pType, HMODULE hInst, DWORD& imageSize)
{
	HRSRC hResource = ::FindResource(hInst, pName, pType);
	if (!hResource)
		return NULL;

	imageSize = ::SizeofResource(hInst, hResource);
	if (!imageSize)
		return NULL;

	HGLOBAL hResourceData = ::LoadResource(hInst, hResource);
	if (!hResourceData)
		return NULL;

	return ::LockResource(hResourceData);
}

inline
bool CGdiPlusBitmapResource::Load(LPCTSTR pName, LPCTSTR pType, HMODULE hInst)
{
	Empty();

	DWORD imageSize{};
	const void* pResourceData = LockResourceData(pName, pType, hInst, imageSize);
	if (!pResourceData)
		return false;

	m_hBuffer  = ::GlobalAlloc(GMEM_MOVEABLE, imageSize);
	if (m_hBuffer)
	{
		void* pBuffer = ::GlobalLock(m_hBuffer);
		if (pBuffer)
		{
			CopyMemory(pBuffer, pResourceData, imageSize);

			IStream* pStream = NULL;
			if (::CreateStreamOnHGlobal(m_hBuffer, FALSE, &pStream) == S_OK)
			{
				m_pBitmap.reset(Gdiplus::Bitmap:: FromStream(pStream));
				pStream->Release();
				if (m_pBitmap)
				{
					if (m_pBitmap->GetLastStatus() == Gdiplus::Ok)
						return true;

					m_pBitmap.reset();
				}
			}
			::GlobalUnlock(m_hBuffer);
		}
		::GlobalFree(m_hBuffer);
		m_hBuffer = NULL;
	}
	return false;
}

