#include "externHeaders.h"
#include "chronflow.h"
#include <stdio.h>

// Returns the time in seconds with maximum resolution
double Helpers::getHighresTimer(void)
{
	static double timerResolution = 0;
	static __int64 timerOffset = 0;
	static bool timerSupported;
	if (timerResolution == 0){
		LARGE_INTEGER res;
		if (!QueryPerformanceFrequency(&res) || res.QuadPart == 0){
			timerSupported = false;
			timerResolution = 1;
		} else {
			timerSupported = true;
			timerResolution = 1.0/(double)res.QuadPart;
			LARGE_INTEGER count;
			QueryPerformanceCounter(&count);
			timerOffset = count.QuadPart;
		}
	}
	if (timerSupported){
		LARGE_INTEGER count;
		QueryPerformanceCounter(&count);
		return timerResolution * (count.QuadPart - timerOffset);
	} else {
		return timeGetTime() / 1000.0;
	}
}
bool Helpers::isPerformanceCounterSupported(){
	LARGE_INTEGER res;
	if (!QueryPerformanceFrequency(&res) || res.QuadPart == 0)
		return false;
	else
		return true;
}


// adjusts a given path for certain discrepancies between how foobar2000
// and GDI+ handle paths, and other oddities
//
// Currently fixes:
//   - User might use a forward-slash instead of a
//     backslash for the directory separator
//   - GDI+ ignores trailing periods '.' in directory names
//   - GDI+ and FindFirstFile ignore double-backslashes
//   - makes relative paths absolute to core_api::get_profile_path()
// Copied from  foo_uie_albumart
void Helpers::fixPath(pfc::string_base & path)
{
    if (path.get_length() == 0)
        return;

	pfc::string8 temp;
	titleformat_compiler::remove_forbidden_chars_string(temp, path, ~0, "*?<>|" "\"");


	// fix directory separators
    temp.replace_char('/', '\\');

	bool is_unc = (pfc::strcmp_partial(temp, "\\\\") == 0);
	if ((temp[1] != ':') && (!is_unc)){
		pfc::string8 profilePath;
		filesystem::g_get_display_path(core_api::get_profile_path(), profilePath);
		profilePath.add_byte('\\');
		
		temp.insert_chars(0, profilePath);
	}


    // fix double-backslashes and trailing periods in directory names
    t_size temp_len = temp.get_length();
    path.reset();
    path.add_byte(temp[0]);
    for (t_size n = 1; n < temp_len-1; n++)
    {
        if (temp[n] == '\\')
        {
            if (temp[n+1] == '\\')
                continue;
        }
        else if (temp[n] == '.')
        {
            if ((temp[n-1] != '.' && temp[n-1] != '\\') &&
                temp[n+1] == '\\')
                continue;
        }
        path.add_byte(temp[n]);
    }
    if (temp_len > 1)
        path.add_byte(temp[temp_len-1]);
}


/*void Helpers::FPS(HWND hWnd, CollectionPos pos, float offset)					// This function calculates FPS
{
	static int fps           = 0;	
	static const int maxFPS  = 60;
	static bool left = true;
	static double previousTime  = 0.0f;	
	static char  strFPS[256];
	static char  strCount[maxFPS+20]  = {0};
	static double lastFps       = 0.0f;
	double currentTime = getHighresTimer();
	
	static double frameTimes[60];
	static int frameTimesP = 0;

	static int highFrame = 0;
	static double highDur = 0;

	if (++fps >= maxFPS){
		fps = 0;
		left = !left;
	}
	memset(strCount,(left?'1':'8'),fps);
	memset(strCount+fps,(left?'8':'1'),maxFPS-fps+1);
	*(strCount+maxFPS+1) = '\0';

	//if (currentTime != previousTime)
	//	lastFps = 1.0/(currentTime-previousTime);


	double frameSum = 0;
	int frameTimesT = frameTimesP;
	double lastTime = currentTime;
	for (int i=0; i < 30; i++){
		frameTimesT--;
		if (frameTimesT < 0)
			frameTimesT = 59;
		frameSum += lastTime - frameTimes[frameTimesT];
		lastTime = frameTimes[frameTimesT];
	}
	lastFps = 1/(frameSum / 30);

	frameTimes[frameTimesP] = currentTime;
	if (++frameTimesP == 60)
		frameTimesP = 0;
	
	highFrame++;
	if (highFrame > 30){
		highFrame = 0;
		highDur = 0;
	}

	if (currentTime - previousTime > highDur){
		highDur = currentTime - previousTime;
		highFrame = 0;
	}
	
	gTexLoader->getLoadedImgTexture(pos)->glBind();
	GLint w;
	GLint h;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	previousTime = currentTime;
	sprintf_s(strFPS, 256, "Pos: %3d, Offset: %1.5f, (%3d x %3d) FPS: %5.2lf, lowFps: %.5lf  -|%s|-", pos.toIndex(), offset, w, h, lastFps, double(1/highDur), strCount);
	SetWindowTextA(hWnd, strFPS);
}*/
