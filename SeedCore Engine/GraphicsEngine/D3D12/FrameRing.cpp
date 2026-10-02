#include <GraphicsEngine/D3D12/FrameRing.h>

namespace SeedCore
{
	Uint FrameRing::Index()
	{
		return index_;
	}

	void FrameRing::Advance() 
	{
		index_ = (index_ + 1) % frameCount;
	}
}