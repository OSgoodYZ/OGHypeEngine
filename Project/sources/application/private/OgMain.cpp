#include "sample/OgSample.h"
#include "system/OgFileSystem.h"

using namespace Og;
using namespace Og::System;

#if defined(__DESKTOP__)
static OgSystemContext* s_systemContext = nullptr;

OgSystemContext* createSystemContext()
{
	OgSystemContext* context = new OgSystemContext();

	//context->configs.window.visible = true;
	context->configs.window.resizable = true;
	context->configs.window.decorated = true;
	context->configs.window.focused = true;
	context->configs.window.autoIconify = true;
	context->configs.window.title = "OgEngine";
#if defined(__MACOSX__)
	context->graphicLibrary.name = "METAL";
#else
	context->graphicLibrary.name = "VULKAN";
#endif

	context->executablePath = og_path_executable_current();
	static std::string executableDirectoryPath = og_path_parent(context->executablePath);
	context->executableDirectoryPath = executableDirectoryPath.c_str();

	if (og_system_init(context, NULL))
	{
		return context;
	}
	else
	{
		delete context;
		return nullptr;
	}

	//temp code
	return nullptr;
}


#endif

/*
* argv[0] : program name
* -g, --graphics graphics library name
* -et, --editortest editor test history path
*/
int main(int argc, char* argv[])
{
	s_systemContext = createSystemContext();


	// sample test를 위한 코드
#ifdef OG_SAMPLE_BUILD
	Og::Sample::OgSample sample;

	try {
		sample.Run(s_systemContext);
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;

#endif // OG_SAMPLE_BUILD

	// sample이 아닌 코드는 여기에서 본격적으로 실행
	// TODO

}
