#ifdef OG_SAMPLE_BUILD
#include "sample/OgSample.h"
#endif // OG_SAMPLE_BUILD


int main()
{

	// sample test를 위한 코드
#ifdef OG_SAMPLE_BUILD
	Og::Sample::OgSample sample;
	try {
		sample.Run();
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
