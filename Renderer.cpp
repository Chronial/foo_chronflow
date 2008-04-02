#include "chronflow.h"

extern cfg_struct_t<double> cfgTitlePosH;
extern cfg_struct_t<double> cfgTitlePosV;
extern cfg_int cfgPanelBg;
extern cfg_int cfgTitleColor;
extern cfg_int cfgHighlightWidth;

// Extensions
PFNGLFOGCOORDFPROC glFogCoordf = NULL;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
PFNGLBLENDCOLORPROC glBlendColor = NULL;

#define SELECTION_CENTER INT_MAX //Selection is an unsigned int, so this is center
#define SELECTION_COVERS 1
#define SELECTION_MIRROR 2

Renderer::Renderer(AppInstance* instance)
: appInstance(instance),
  multisampleEnabled(false)
{
}

Renderer::~Renderer(void)
{
}

int Renderer::getPixelFormat() {
	return pixelFormat;
}

const PIXELFORMATDESCRIPTOR Renderer::pixelFormatDescriptor = {
			sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
			1,											// Version Number
			PFD_DRAW_TO_WINDOW |						// Format Must Support Window
			PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
			PFD_DOUBLEBUFFER |							// Must Support Double Buffering
			PFD_SWAP_EXCHANGE,							// Format should swap the buffers (faster)
			PFD_TYPE_RGBA,								// Request An RGBA Format
			24,										    // Select Our Color Depth
			0, 0, 0, 0, 0, 0,							// Color Bits Ignored
			0,											// No Alpha Buffer
			0,											// Shift Bit Ignored
			4,											// Accumulation Buffer
			0, 0, 0, 0,									// Accumulation Bits Ignored
			16,											// 16Bit Z-Buffer (Depth Buffer)  
			0,											// No Stencil Buffer
			0,											// No Auxiliary Buffer
			PFD_MAIN_PLANE,								// Main Drawing Layer
			0,											// Reserved
			0, 0, 0										// Layer Masks Ignored
		};

bool Renderer::attachGlWindow()
{
	HDC hDC;
	HGLRC hRC;
	if (!(hDC=GetDC(appInstance->mainWindow)))							// Did We Get A Device Context?
	{
		destroyGlWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Create A GL Device Context.",L"Foo_chronflow Error",MB_OK|MB_ICONERROR);
		return FALSE;								// Return FALSE
	}

	if (!multisampleEnabled){
		if (!(pixelFormat=ChoosePixelFormat(hDC,&pixelFormatDescriptor)))	// Did Windows Find A Matching Pixel Format?
		{
			destroyGlWindow();								// Reset The Display
			MessageBox(NULL,L"Can't Find A Suitable PixelFormat.",L"Foo_chronflow Error",MB_OK|MB_ICONERROR);
			return FALSE;								// Return FALSE
		}

		{
			PIXELFORMATDESCRIPTOR  pfd; 

			DescribePixelFormat(hDC, pixelFormat,  
				sizeof(PIXELFORMATDESCRIPTOR), &pfd); 
			if ((pfd.dwFlags & PFD_GENERIC_FORMAT) && !(pfd.dwFlags & PFD_GENERIC_ACCELERATED)){
				MessageBox(appInstance->mainWindow, L"Rendering in Software mode. Expect slow display.",L"Foo_chronflow Error",MB_ICONINFORMATION);
			} else if ((pfd.dwFlags & PFD_GENERIC_FORMAT) || (pfd.dwFlags & PFD_GENERIC_ACCELERATED)){
				pfc::string8 message("Foo_chronflow Problem: Rendering is not fully hardware accelerated. Details: ");
				if (pfd.dwFlags & PFD_GENERIC_FORMAT)
					message << "PFD_GENERIC_FORMAT ";
				if (pfd.dwFlags & PFD_GENERIC_ACCELERATED)
					message << "PFD_GENERIC_ACCELERATED";
				console::print(message);
			}
		}
	}

	if(!SetPixelFormat(hDC,pixelFormat,&pixelFormatDescriptor))		// Are We Able To Set The Pixel Format?
	{
		destroyGlWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Set The PixelFormat.",L"ERROR",MB_OK|MB_ICONERROR);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		destroyGlWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Create A GL Rendering Context.",L"Foo_chronflow Error",MB_OK|MB_ICONERROR);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC, hRC))					// Try To Activate The Rendering Context
	{
		destroyGlWindow();								// Reset The Display
		MessageBox(NULL,L"Can't Activate The GL Rendering Context.",L"Foo_chronflow Error",MB_OK|MB_ICONERROR);
		return FALSE;								// Return FALSE
	}
	appInstance->lockedRC.setRC(hDC, hRC);
	return true;
}


void Renderer::initGlState()
{
	ScopeRCLock scopeLock(appInstance->lockedRC);

	// move this to an extra function - otherwice it's called twice due to mulitsampling
	loadExtensions();

	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	glHint(GL_TEXTURE_COMPRESSION_HINT,GL_FASTEST);
	glEnable(GL_TEXTURE_2D);

	if (!isExtensionSupported("GL_ARB_texture_non_power_of_two"))
		ImgTexture::setForcePowerOfTwo();
	GLint maxTexSize;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	ImgTexture::setMaxGlTextureSize(maxTexSize);
	if (isWglExtensionSupported("WGL_EXT_swap_control"))
		wglSwapIntervalEXT(1);						    // Activate Vsynch
	
	if (multisampleEnabled)
		glEnable(GL_MULTISAMPLE_ARB);

	if (isExtensionSupported("GL_EXT_fog_coord")){
		glFogi(GL_FOG_MODE, GL_EXP);
		glFogf(GL_FOG_DENSITY, 5);
		GLfloat	fogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};     // Fog Color - should be BG color
		glFogfv(GL_FOG_COLOR, fogColor);					// Set The Fog Color
		glHint(GL_FOG_HINT, GL_NICEST);						// Per-Pixel Fog Calculation
		glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);		// Set Fog Based On Vertice Coordinates
		showFog = true;
	} else {
		showFog = false;
	}
}

bool Renderer::initMultisampling(){
	ScopeRCLock scopeLock(appInstance->lockedRC);

	// See If The String Exists In WGL!
	if (!isWglExtensionSupported("WGL_ARB_multisample")){
		multisampleEnabled = false;
		return false;
	}

	// Get Our Pixel Format
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");	
	if (!wglChoosePixelFormatARB){
		multisampleEnabled = false;
		return false;
	}

	int		valid;
	UINT	numFormats;
	float	fAttributes[] = {0,0};

	// These Attributes Are The Bits We Want To Test For In Our Sample
	// Everything Is Pretty Standard, The Only One We Want To 
	// Really Focus On Is The SAMPLE BUFFERS ARB And WGL SAMPLES
	// These Two Are Going To Do The Main Testing For Whether Or Not
	// We Support Multisampling On This Hardware.
	int iAttributes[] =
	{
		WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
		WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB,24,
		WGL_ALPHA_BITS_ARB,8,
		WGL_DEPTH_BITS_ARB,16,
		WGL_STENCIL_BITS_ARB,0,
		WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
		WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
		WGL_SAMPLES_ARB,4,
		0,0
	};

	// First We Check To See If We Can Get A Pixel Format For 4 Samples
	valid = wglChoosePixelFormatARB(appInstance->lockedRC.getHDC(),iAttributes,fAttributes,1,&pixelFormat,&numFormats);
 
	// If We Returned True, And Our Format Count Is Greater Than 1
	/*if (valid && numFormats >= 1)
	{
		multisampleEnabled = true;
		return true;
	}*/

	// Our Pixel Format With 4 Samples Failed, Test For 2 Samples
	iAttributes[19] = 2;
	valid = wglChoosePixelFormatARB(appInstance->lockedRC.getHDC(),iAttributes,fAttributes,1,&pixelFormat,&numFormats);
	if (valid && numFormats >= 1)
	{
		multisampleEnabled = true;
		return true;
	}

	return false;
}

void Renderer::loadExtensions(){
	if(isExtensionSupported("GL_EXT_fog_coord"))
		glFogCoordf = (PFNGLFOGCOORDFPROC) wglGetProcAddress("glFogCoordf");
	if(isWglExtensionSupported("WGL_EXT_swap_control"))
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if(isExtensionSupported("GL_EXT_blend_color"))
		glBlendColor = (PFNGLBLENDCOLORPROC)wglGetProcAddress("glBlendColor");
}

bool Renderer::isWglExtensionSupported(const char *name){
	static const char* extensions = NULL;
	if (extensions == NULL){
		// Try To Use wglGetExtensionStringARB On Current DC, If Possible
		PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtString = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

		if (wglGetExtString)
			extensions = wglGetExtString(wglGetCurrentDC());

		// If That Failed, Try Standard Opengl Extensions String
		if (extensions == NULL)
			extensions = (char*)glGetString(GL_EXTENSIONS);

		if (extensions == NULL)
			return false;
	}

	const size_t extlen = strlen(name);
	for (const char* p = extensions; ; p++)
	{
		p = strstr(p, name);
		if (p == NULL)
			return false;						// No Match
		if ((p==extensions || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
			return true;						// Match
	}
	return false;
}

bool Renderer::isExtensionSupported(const char *name){
	static const char* extensions = (char*)glGetString(GL_EXTENSIONS);

	const size_t extlen = strlen(name);
	for (const char* p = extensions; ; p++)
	{
		p = strstr(p, name);
		if (p == NULL)
			return false;						// No Match
		if ((p==extensions || p[-1]==' ') && (p[extlen]=='\0' || p[extlen]==' '))
			return true;						// Match
	}
	return false;
}

void Renderer::destroyGlWindow(void)
{
	ScopeRCLock scopeLock(appInstance->lockedRC);
	HGLRC hRC = appInstance->lockedRC.getHRC();
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(0, 0))					// Are We Able To Release The DC And RC Contexts?
		{
			console::print(pfc::string8("Foo_chronflow Error: Release Of RC Failed. (mainWin) Details:") << format_win32_error(GetLastError()));
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,L"Release Rendering Context Failed.",L"Foo_chronflow Error",MB_OK | MB_ICONINFORMATION);
		}
	}
	HDC hDC = appInstance->lockedRC.getHDC();	
	if (hDC && !ReleaseDC(appInstance->mainWindow,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,L"Release Device Context Failed.",L"Foo_chronflow Error",MB_OK | MB_ICONINFORMATION);
	}
	appInstance->lockedRC.setRC(0, 0);
}

void Renderer::resizeGlScene(int width, int height){
	ScopeRCLock scopeLock(appInstance->lockedRC);

	if (height == 0)
		height = 1;
	winWidth = width;
	winHeight = height;

	glViewport(0, 0, width, height); // Reset The Current Viewport
	setProjectionMatrix();
}

void Renderer::resetViewport(){
	setProjectionMatrix();
}

void Renderer::setProjectionMatrix(bool pickMatrix, int x, int y){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (pickMatrix){
		GLint viewport[4];
		glGetIntegerv (GL_VIEWPORT, viewport);
		gluPickMatrix ((GLdouble) x, (GLdouble) (viewport[3] - y), 
					  1.0, 1.0, viewport);
	}

	// Calculate The Aspect Ratio Of The Window
	static const double fov = 45;
	static const double squareLength = tan(deg2rad(fov)/2)*0.1;
	fovAspectBehaviour weight = appInstance->coverPos->getAspectBehaviour();
	double aspect = (double)winHeight/winWidth;
	double width = squareLength / pow(aspect, (double)weight.y);
	double height = squareLength * pow(aspect, (double)weight.x);

	glFrustum(-width, +width, -height, +height, 0.1f, 1000.0f);

	glMatrixMode(GL_MODELVIEW);
}

void Renderer::glPushOrthoMatrix(){
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, winWidth, 0, winHeight, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}

void Renderer::glPopOrthoMatrix(){
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

bool Renderer::positionOnPoint(int x, int y, CollectionPos& out) 
{
	ScopeRCLock scopeLock(appInstance->lockedRC);
	GLuint buf[256];
	glSelectBuffer(256, buf);
	glRenderMode(GL_SELECT);
	setProjectionMatrix(true, x, y);
	glInitNames();
	drawFrame(true);
	GLint hits = glRenderMode(GL_RENDER);
	setProjectionMatrix();
	GLuint *p = buf;
	GLuint minZ = INFINITE;
	GLuint selectedName = 0;
	for (int i=0; i < hits; i++){
		GLuint names = *p; p++;
		GLuint z = *p; p++; p++;
		if ((names > 1) && (z < minZ) && (*p == SELECTION_COVERS)){
			minZ = z;
			selectedName = *(p+1);
		}
		p += names;
	}
	if (minZ != INFINITE){
		out = appInstance->displayPos->getCenteredPos() + (selectedName - SELECTION_CENTER);
		return true;
	} else {
		return false;
	}
}

void Renderer::drawMirrorPass(){
	glVectord mirrorNormal = appInstance->coverPos->getMirrorNormal();
	glVectord mirrorCenter = appInstance->coverPos->getMirrorCenter();

	double mirrorDist; // distance from origin
	mirrorDist = mirrorCenter * mirrorNormal;
	glVectord mirrorPos = mirrorDist * mirrorNormal;

	glVectord scaleAxis(1, 0, 0);
	glVectord rotAxis = scaleAxis.cross(mirrorNormal);
	double rotAngle = rad2deg(scaleAxis.intersectAng(mirrorNormal));
	rotAngle = -2*rotAngle;

	if (showFog){
		GLfloat	fogColor[4] = {GetRValue(cfgPanelBg)/255.0f, GetGValue(cfgPanelBg)/255.0f, GetBValue(cfgPanelBg)/255.0f, 1.0f};
		glFogfv(GL_FOG_COLOR, fogColor);
		glEnable(GL_FOG);
	}

	glPushMatrix();
		glTranslated(2 * mirrorPos.x, 2 * mirrorPos.y, 2 * mirrorPos.z);
		glScalef(-1.0f, 1.0f, 1.0f);
		glRotated(rotAngle, rotAxis.x, rotAxis.y, rotAxis.z);

		drawCovers();
	glPopMatrix();
	
	if (showFog)
		glDisable(GL_FOG);
}

void Renderer::drawMirrorOverlay(){
	glBindTexture(GL_TEXTURE_2D, 0);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glColor4f(GetRValue(cfgPanelBg)/255.0f, GetGValue(cfgPanelBg)/255.0f, GetBValue(cfgPanelBg)/255.0f, 0.60f);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPushMatrix();
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
			glLoadIdentity();
			glBegin(GL_QUADS);
				glVertex3i (-1, -1, -1);
				glVertex3i (1, -1, -1);
				glVertex3i (1, 1, -1); 
				glVertex3i (-1, 1, -1);
			glEnd();
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

pfc::array_t<double> Renderer::getMirrorClipPlane(){
	glVectord mirrorNormal = appInstance->coverPos->getMirrorNormal();
	glVectord mirrorCenter = appInstance->coverPos->getMirrorCenter();
	glVectord eye2mCent = (mirrorCenter - appInstance->coverPos->getCameraPos());

	// Angle at which the mirror normal stands to the eyePos
	double mirrorNormalAngle = rad2deg(eye2mCent.intersectAng(mirrorNormal));
	pfc::array_t<double> clipEq;
	clipEq.set_size(4);
	if (mirrorNormalAngle > 90){
		clipEq[0] = -mirrorNormal.x;
		clipEq[1] = -mirrorNormal.y;
		clipEq[2] = -mirrorNormal.z;
		clipEq[3] = mirrorNormal * mirrorCenter;
	} else {
		clipEq[0] = mirrorNormal.x;
		clipEq[1] = mirrorNormal.y;
		clipEq[2] = mirrorNormal.z;
		clipEq[3] = -(mirrorNormal * mirrorCenter);
	}
	return clipEq;
}

void Renderer::drawFrame(bool selectionPass)
{
	ScopeRCLock scopeLock(appInstance->lockedRC);
	glClearColor(GetRValue(cfgPanelBg)/255.0f, GetGValue(cfgPanelBg)/255.0f, GetBValue(cfgPanelBg)/255.0f, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(
		appInstance->coverPos->getCameraPos().x, appInstance->coverPos->getCameraPos().y, appInstance->coverPos->getCameraPos().z, 
		appInstance->coverPos->getLookAt().x,    appInstance->coverPos->getLookAt().y,    appInstance->coverPos->getLookAt().z, 
		appInstance->coverPos->getUpVector().x,  appInstance->coverPos->getUpVector().y,  appInstance->coverPos->getUpVector().z);
	
	pfc::array_t<double> clipEq;

	if (appInstance->coverPos->isMirrorPlaneEnabled()){
		clipEq = getMirrorClipPlane();
		if (!selectionPass){
			glClipPlane(GL_CLIP_PLANE0, clipEq.get_ptr());
			glEnable(GL_CLIP_PLANE0);
			glPushName(SELECTION_MIRROR);
				drawMirrorPass();
			glPopName();
			glDisable(GL_CLIP_PLANE0);
			drawMirrorOverlay();
		}

		// invert the clip equation
		for (int i=0; i < 4; i++){
			clipEq[i] *= -1;
		}
		
		glClipPlane(GL_CLIP_PLANE0, clipEq.get_ptr());
		glEnable(GL_CLIP_PLANE0);
	}
	
	ImgTexture* ptr = 0;

	glPushName(SELECTION_COVERS);
		drawCovers(true);
	glPopName();
	
	if (appInstance->coverPos->isMirrorPlaneEnabled()){
		glDisable(GL_CLIP_PLANE0);
	}
	
	if (!selectionPass){
		pfc::string8 albumTitle;
		appInstance->albumCollection->getTitle(appInstance->displayPos->getTarget(), albumTitle);
		if (albumTitle.length() > 0)
			appInstance->textDisplay->displayText(albumTitle, int(winWidth*cfgTitlePosH), int(winHeight*(1-cfgTitlePosV)), TextDisplay::center, TextDisplay::middle);


		double fps, msPerFrame, longestFrame;
		appInstance->fpsCounter.getFPS(fps, msPerFrame, longestFrame);
		pfc::string8 dispString;
		dispString << "FPS: " << pfc::format_float(fps, 4, 1);
		dispString << "  ms/f: " << pfc::format_float(msPerFrame, 5, 2);
		dispString << "  max: " << pfc::format_float(msPerFrame, 5, 2);
		// pfc::string8("FPS: ") << int(fps) << " ms/f: " << int(msPerFrame) << " max: " << int(longestFrame)

		appInstance->textDisplay->displayBitmapText(dispString, winWidth - 300, winHeight - 20);
	}
}

bool Renderer::shareLists(HGLRC shareWith){
	appInstance->lockedRC.lock();
	bool ret = 0 != wglShareLists(appInstance->lockedRC.getHRC(), shareWith);
	appInstance->lockedRC.unlock();
	return ret;
}

void Renderer::drawCovers(bool showTarget){
#ifdef COVER_ALPHA
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.1f);
#endif
	
	if (cfgHighlightWidth == 0)
		showTarget = false;

	int firstCover = appInstance->coverPos->getFirstCover()+1;
	int lastCover  = appInstance->coverPos->getLastCover();
	float centerOffset = appInstance->displayPos->getCenteredOffset();
	CollectionPos p = appInstance->displayPos->getCenteredPos() + firstCover;
	for (int o = firstCover; o <= lastCover; ++o, ++p){
		float co = -centerOffset + o;

		ImgTexture* tex = appInstance->texLoader->getLoadedImgTexture(p);
		tex->glBind();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// calculate darkening
		float g = 1-(min(1,(abs(co)-2)/5))*0.5f;
		if (abs(co) < 2)
			g = 1;
		/*float g = 1 - (abs(co)-2)*0.2f;
		g = 1 - abs(co)*0.1f;
		g = 1 - abs(zRot)/80;
		g= 1;
		if (g < 0) g = 0;*/
		glColor3f( g, g, g);

		glQuad coverQuad = appInstance->coverPos->getCoverQuad(co, tex->getAspect());
		
		glPushName(SELECTION_CENTER + o);

		glBegin(GL_QUADS);
			if (showFog) glFogCoordf((GLfloat)appInstance->coverPos->distanceToMirror(coverQuad.topLeft));
			glTexCoord2f(0.0f, 1.0f); // top left
			glVertex3fv((GLfloat*)&(coverQuad.topLeft.x));

			if (showFog) glFogCoordf((GLfloat)appInstance->coverPos->distanceToMirror(coverQuad.topRight));
			glTexCoord2f(1.0f, 1.0f); // top right
			glVertex3fv((GLfloat*)&(coverQuad.topRight.x));

			if (showFog) glFogCoordf((GLfloat)appInstance->coverPos->distanceToMirror(coverQuad.bottomRight));
			glTexCoord2f(1.0f, 0.0f); // bottom right
			glVertex3fv((GLfloat*)&(coverQuad.bottomRight.x));

			if (showFog) glFogCoordf((GLfloat)appInstance->coverPos->distanceToMirror(coverQuad.bottomLeft));
			glTexCoord2f(0.0f, 0.0f); // bottom left
			glVertex3fv((GLfloat*)&(coverQuad.bottomLeft.x));
		glEnd();
		glPopName();

		if (showTarget){
			if (p == appInstance->displayPos->getTarget()){
				bool clipPlane = false;
				if (glIsEnabled(GL_CLIP_PLANE0)){
					glDisable(GL_CLIP_PLANE0);
					clipPlane = true;
				}

				showTarget = false;
				
				glColor3f(GetRValue(cfgTitleColor) / 255.0f, GetGValue(cfgTitleColor) / 255.0f, GetBValue(cfgTitleColor) / 255.0f);

				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
				glBindTexture(GL_TEXTURE_2D,0);

				glLineWidth((GLfloat)cfgHighlightWidth);
				glPolygonOffset(-1.0f, -1.0f);
				glEnable(GL_POLYGON_OFFSET_LINE);

				glEnable(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, (void*) &coverQuad);
				glDrawArrays(GL_QUADS, 0, 4);


				glDisable(GL_POLYGON_OFFSET_LINE);

				if (clipPlane)
					glEnable(GL_CLIP_PLANE0);
			}
		}
	}

#ifdef COVER_ALPHA
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
#endif
}