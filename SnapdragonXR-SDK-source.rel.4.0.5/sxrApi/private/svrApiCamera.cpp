#ifdef ENABLE_CAMERA
#include "svrCameraManager.h"

SvrCameraManager gSvrCameraManager;

/**********************************************************
 * svrRenderCamera
 **********************************************************/
SxrResult svrRenderCamera()
{
	SxrResult result = gSvrCameraManager.Render();

    return result;
}
#endif
