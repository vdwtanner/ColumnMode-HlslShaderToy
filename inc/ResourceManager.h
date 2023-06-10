#pragma once

namespace CM_HlslShaderToy
{
	class ResourceData
	{
	private:

		struct Constants
		{
			float time;
			float deltaTime;
		} m_constants;

		union DirtyBits
		{
			UINT bitfield;

			struct
			{
				UINT constants : 1;
			};
		} m_dirtyBits;

	public:
		void SetConstants(float time, float deltaTime);
		const void* GetConstantsPtr() { return &m_constants; };
		const DirtyBits GetDirtyBits() { return m_dirtyBits; };
		void ClearDirtyBits() { m_dirtyBits.bitfield = 0; }
	
	};

	class ResourceManager
	{
	public:
		ResourceManager() {}

		LockedContainer<ResourceData>& GetData() { return m_lockedData; }

	private:
		LockedContainer<ResourceData> m_lockedData;
	};
}