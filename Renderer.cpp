#include "chronflow.h"

extern cfg_struct_t<double> cfgTitlePosH;
extern cfg_struct_t<double> cfgTitlePosV;
extern cfg_int cfgPanelBg;
extern cfg_int cfgTitleColor;

// Extensions
PFNGLFOGCOORDFPROC glFogCoordf = NULL;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
PFNGLBLENDCOLORPROC glBlendColor = NULL;

#define SELECTION_CENTER INT_MAX //Selection is an unsigned int, so this is center
#define SELECTION_COVERS 1
#define SELECTION_MIRROR 2

Renderer::Renderer(AppInstance* instance)
{
	this->appInstance = instance;
	hRC = 0;
	hDC = 0;
}

Renderer::~Renderer(void)
{
}


bool Renderer::setRenderingContext(){
	return 0 != wglMakeCurrent(hDC,hRC);
}

bool Renderer::releaseRenderingContext(){
	return 0 != wglMakeCurrent(NULL,NULL);
}

PIXELFORMATDESCRIPTOR Renderer::getPixelFormat() {
	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER |							// Must Support Double Buffering
		PFD_SWAP_EXCHANGE,							// Format should swap the buffers (faster)
		PFD_TYPE_RGBA,								// Request An RGBA Format
		16,										    // Select Our Color Depth
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
	return pfd;
}

bool Renderer::setupGlWindow()
{
	{ // Window Creation
		GLuint		PixelFormat;			// Holds The Results After Searching For A Match
		PIXELFORMATDESCRIPTOR pfd = getPixelFormat();
		
		if (!(hDC=GetDC(appInstance->mainWindow)))							// Did We Get A Device Context?
		{
			destroyGlWindow();								// Reset The Display
			MessageBox(NULL,L"Can't Create A GL Device Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;								// Return FALSE
		}

		if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
		{
			destroyGlWindow();								// Reset The Display
			MessageBox(NULL,L"Can't Find A Suitable PixelFormat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;								// Return FALSE
		}

		if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
		{
			destroyGlWindow();								// Reset The Display
			MessageBox(NULL,L"Can't Set The PixelFormat.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;								// Return FALSE
		}

		if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
		{
			destroyGlWindow();								// Reset The Display
			MessageBox(NULL,L"Can't Create A GL Rendering Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;								// Return FALSE
		}

		if(!setRenderingContext())					// Try To Activate The Rendering Context
		{
			destroyGlWindow();								// Reset The Display
			MessageBox(NULL,L"Can't Activate The GL Rendering Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;								// Return FALSE
		}

		//ShowWindow(appInstance->hWnd,SW_SHOW);		// Show The Window -- not allow in collumns UI!
		//SetForegroundWindow(hWnd);						// Slightly Higher Priority
		//SetFocus(hWnd);									// Sets Keyboard Focus To The Window
		//ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen
	}
	
	loadExtensions();

	{ // Setup OpenGL
		glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
		//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
		glClearDepth(1.0f);									// Depth Buffer Setup
		glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
		glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
		glHint(GL_TEXTURE_COMPRESSION_HINT,GL_FASTEST);
		glEnable(GL_TEXTURE_2D);

		//glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CW);

		if (!isExtensionSupported("GL_ARB_texture_non_power_of_two"))
			ImgTexture::setForcePowerOfTwo();
		GLint maxTexSize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
		ImgTexture::setMaxGlTextureSize(maxTexSize);
		if (isExtensionSupported("WGL_EXT_swap_control"))
			wglSwapIntervalEXT(1);						    // Activate Vsynch

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
	return true;
}

void Renderer::loadExtensions(){
	if(isExtensionSupported("GL_EXT_fog_coord"))
		glFogCoordf = (PFNGLFOGCOORDFPROC) wglGetProcAddress("glFogCoordf");
	if(isExtensionSupported("WGL_EXT_swap_control"))
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if(isExtensionSupported("GL_EXT_blend_color"))
		glBlendColor = (PFNGLBLENDCOLORPROC)wglGetProcAddress("glBlendColor");
}

bool Renderer::isExtensionSupported(const char *name){
	static const GLubyte *extensions = glGetString(GL_EXTENSIONS);
	const GLubyte *start;

	GLubyte *pos, *terminator;
	pos = (GLubyte *) strchr(name, ' ');
	if (pos || *name == '\0')
		return 0;
	start = extensions;
	for (;;){
		pos = (GLubyte *) strstr((const char *) start, name);
		if (!pos)
			break;
		terminator = pos + strlen(name);
		if (pos == start || *(pos - 1) == ' '){
			if (*terminator == ' ' || *terminator == '\0')
			{
				return true;
			}
		}
		start = terminator;
	}
	return false;
}

void Renderer::destroyGlWindow(void)
{
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!releaseRenderingContext())					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,L"Release Of RC Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,L"Release Rendering Context Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(appInstance->mainWindow,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,L"Release Device Context Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}
}

void Renderer::resizeGlScene(int width, int height){
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
}

void Renderer::glPopOrthoMatrix(){
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
}

bool Renderer::positionOnPoint(int x, int y, CollectionPos& out) 
{
	GLuint buf[256];
	glSelectBuffer(256, buf);
	glRenderMode(GL_SELECT);
	setProjectionMatrix(true, x, y);
	glInitNames();
	drawFrame();
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
void Renderer::drawEmptyFrame(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
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


	GLfloat	fogColor[4] = {GetRValue(cfgPanelBg)/255.0f, GetGValue(cfgPanelBg)/255.0f, GetBValue(cfgPanelBg)/255.0f, 1.0f};
	glFogfv(GL_FOG_COLOR, fogColor);
	glEnable(GL_FOG);

	glPushMatrix();
		glTranslated(2 * mirrorPos.x, 2 * mirrorPos.y, 2 * mirrorPos.z);
		glScalef(-1.0f, 1.0f, 1.0f);
		glRotated(rotAngle, rotAxis.x, rotAxis.y, rotAxis.z);

		glPushName(SELECTION_MIRROR);
			drawCovers();
		glPopName();
	glPopMatrix();

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

pfc::array_staticsize_t<double> Renderer::getMirrorClipPlane(){
	glVectord mirrorNormal = appInstance->coverPos->getMirrorNormal();
	glVectord mirrorCenter = appInstance->coverPos->getMirrorCenter();
	glVectord eye2mCent = (mirrorCenter - appInstance->coverPos->getCameraPos());

	// Angle at which the mirror normal stands to the eyePos
	double mirrorNormalAngle = rad2deg(eye2mCent.intersectAng(mirrorNormal));
	pfc::array_staticsize_t<double> clipEq(4);
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

void Renderer::drawFrame(void)
{
	glClearColor(GetRValue(cfgPanelBg), GetGValue(cfgPanelBg), GetBValue(cfgPanelBg), 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(
		appInstance->coverPos->getCameraPos().x, appInstance->coverPos->getCameraPos().y, appInstance->coverPos->getCameraPos().z, 
		appInstance->coverPos->getLookAt().x,    appInstance->coverPos->getLookAt().y,    appInstance->coverPos->getLookAt().z, 
		appInstance->coverPos->getUpVector().x,  appInstance->coverPos->getUpVector().y,  appInstance->coverPos->getUpVector().z);
	
	pfc::array_staticsize_t<double> clipEq(4);

	if (appInstance->coverPos->isMirrorPlaneEnabled()){
		clipEq = getMirrorClipPlane();
		glClipPlane(GL_CLIP_PLANE0, clipEq.get_ptr());
		glEnable(GL_CLIP_PLANE0);
			drawMirrorPass();
		glDisable(GL_CLIP_PLANE0);
		drawMirrorOverlay();

		// invert the clip equation
		for (int i=0; i < 4; i++){
			clipEq[i] *= -1;
		}
		
		glClipPlane(GL_CLIP_PLANE0, clipEq.get_ptr());
		glEnable(GL_CLIP_PLANE0);
	}
	
	glPushName(SELECTION_COVERS);
		drawCovers(true);
	glPopName();
	
	if (appInstance->coverPos->isMirrorPlaneEnabled()){
		glDisable(GL_CLIP_PLANE0);
	}
	
	pfc::string8 albumTitle;
	appInstance->albumCollection->getTitle(appInstance->displayPos->getTarget(), albumTitle);
	appInstance->textDisplay->displayText(albumTitle, int(winWidth*cfgTitlePosH), int(winHeight*(1-cfgTitlePosV)), TextDisplay::center, TextDisplay::middle);

	glFlush();
}

void Renderer::swapBuffers(){
	SwapBuffers(hDC);
}

bool Renderer::shareLists(HGLRC shareWith){
	return 0 != wglShareLists(hRC, shareWith);
}

void Renderer::drawCovers(bool showTarget){
#ifdef COVER_ALPHA
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.1f);
#endif

	int firstCover = appInstance->coverPos->getFirstCover()+1;
	int lastCover  = appInstance->coverPos->getLastCover();
	float centerOffset = appInstance->displayPos->getCenteredOffset();
	CollectionPos p = appInstance->displayPos->getCenteredPos() + firstCover;
	for (int o = firstCover; o <= lastCover; ++o, ++p){
		float co = -centerOffset + o;

		/*zPos = frontZ - co*co*0.3;
		zRot = rad2deg(atan(zPos*co));
		xPos = co*0.7;*/
		/*float radius = 3;
		float factor = co * 1.95f/radius;
		zPos = -8 - radius + cos(factor)*radius;
		xPos = sin(factor)*radius;
		zRot = float(rad2deg(factor));
		yPos = co*0.4f-1.6f;

		glTranslatef(xPos, yPos, zPos);
		glRotatef(zRot, 0, 1.0f, 0);*/

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
				showTarget = false;

				glColor3f(GetRValue(cfgTitleColor) / 255.0f, GetGValue(cfgTitleColor) / 255.0f, GetBValue(cfgTitleColor) / 255.0f);

				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
				glBindTexture(GL_TEXTURE_2D,0);
				glLineWidth(4);

				glEnable(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, (void*) &coverQuad);
				glDrawArrays(GL_QUADS, 0, 4);
			}
		}
	}

#ifdef COVER_ALPHA
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
#endif
}