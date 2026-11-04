#include <Editor/Editor/Framework.h>
#include <Editor/Editor/Engine.h>
#include <FoundationEngine/Utility/Bootstrap.h>

int WinMain(HINSTANCE hCurrentInstance, HINSTANCE hPreviousInstance, LPSTR lpCommandLine, int nShowCommand)
{
	setlocale(LC_CTYPE, ".UTF8");

	SeedCore::Bootstrap boot{};

	SeedCore::Engine engine;
	SeedCore::Framework framework(engine, boot);

	return framework.Run();
}