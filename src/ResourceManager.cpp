#include "pch.h"

void CM_HlslShaderToy::ResourceData::SetConstants(float time, float deltaTime)
{
	m_constants.time = time;
	m_constants.deltaTime = deltaTime;

	m_dirtyBits.constants = true;
}
