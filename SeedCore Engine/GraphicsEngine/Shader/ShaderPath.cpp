#include <GraphicsEngine/Shader/ShaderPath.h>
#include <FoundationEngine/File/FileLocator.h>

namespace SeedCore
{
	std::filesystem::path ShaderPath::Absolute()const
	{
		return (mount_ == ShaderMount::Engine ? FileLocator::EngineRoot() : FileLocator::ProjectRoot()) / path_;
	}
}