#include "OgSampleMain.h"
#include "public/application/OgSampleApplication.h"

OG_NAMESPACE_SAMPLE_BEGIN

void OgSampleMain::Run(OgSystemContext* systemContext) 
{
    // 애플리케이션 인스턴스 생성 및 실행
    OgSampleApplication app;
    app.Run(systemContext);
}

OG_NAMESPACE_SAMPLE_END
