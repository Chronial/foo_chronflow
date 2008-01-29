#include "chronflow.h"

// Extensions
PFNGLFOGCOORDFPROC glFogCoordf = NULL;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;

#define SELECTION_CENTER INT_MAX //Selection is an unsigned int, so this is center
#define SELECTION_COVERS 1
#define SELECTION_MIRROR 2

Renderer::Renderer(DisplayPosition* disp)
{
	this->displayPosition = disp;
}

Renderer::~Renderer(void)
{
}

void Renderer::setRenderingContext(){
	wglMakeCurrent(hDC,hRC);
}

bool Renderer::setupGlWindow(HWND hWnd)
{
	{ // Window Creation
		this->hWnd = hWnd;
		GLuint		PixelFormat;			// Holds The Results After Searching For A Match
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
		
		if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
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

		if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
		{
			destroyGlWindow();								// Reset The Display
			MessageBox(NULL,L"Can't Activate The GL Rendering Context.",L"ERROR",MB_OK|MB_ICONEXCLAMATION);
			return FALSE;								// Return FALSE
		}

		ShowWindow(hWnd,SW_SHOW);						// Show The Window
		SetForegroundWindow(hWnd);						// Slightly Higher Priority
		SetFocus(hWnd);									// Sets Keyboard Focus To The Window
		//ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen
	}
	
	loadExtensions();

	{ // Setup OpenGL
		glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);				// Black Background
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
			glFogi(GL_FOG_MODE, GL_LINEAR);						// Fog Fade Is Linear
			GLfloat	fogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};     // Fog Color - should be BG color
			glFogfv(GL_FOG_COLOR, fogColor);					// Set The Fog Color
			glFogf(GL_FOG_START,  0.0f);						// Set The Fog Start (Least Dense)
			glFogf(GL_FOG_END,    0.35f);						// Set The Fog End (Most Dense)
			glHint(GL_FOG_HINT, GL_NICEST);						// Per-Pixel Fog Calculation
			glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);		// Set Fog Based On Vertice Coordinates
		}
	}
	return true;
}

void Renderer::loadExtensions(){
	if(isExtensionSupported("GL_EXT_fog_coord"))
		glFogCoordf = (PFNGLFOGCOORDFPROC) wglGetProcAddress("glFogCoordf");
	if(isExtensionSupported("WGL_EXT_swap_control"))
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
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
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,L"Release Of DC And RC Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,L"Release Rendering Context Failed.",L"SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
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
	static const double FRUSTDIM = 0.0415;
	glFrustum(-FRUSTDIM*winWidth/winHeight, FRUSTDIM*winWidth/winHeight, -FRUSTDIM, FRUSTDIM, 0.1f,50.0f);
	//gluPerspective (45.0, (double)winWidth/winHeight, 0.1, 50.0);

	glMatrixMode(GL_MODELVIEW);
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
		out = displayPosition->getCenteredPos() + (selectedName - SELECTION_CENTER);
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

void Renderer::drawFrame(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(0, -0.3f, 0,   0, -0.3f, -4.0f,   0, 1, 0);

	
	/*glPushName(SELECTION_MIRROR);
	glPushMatrix(); // Draw Mirrored Images
		glTranslatef(0, -2, 0);
		glScalef(1.0, -1.0, 1.0);
		glEnable(GL_FOG);
		glFrontFace(GL_CCW);
		drawCovers();
		glFrontFace(GL_CW);
		glDisable(GL_FOG);
	glPopMatrix();
	glPopName();
	

	glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(0.0, 0.0, 0.0, 0.60f);
		drawFloor();
	glDisable(GL_BLEND);*/
	
	glPushName(SELECTION_COVERS);
	drawCovers(true);
	glPopName();
	
	glFlush();
}

void Renderer::swapBuffers(){
	SwapBuffers(hDC);
}

void Renderer::drawFloor(){
	glBindTexture(GL_TEXTURE_2D, 0);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glBegin(GL_QUADS);				// Draw A Quad
		glVertex3f(-50.0f,-1.0f, -100.0f);		// back Left
		glVertex3f( 50.0f,-1.0f, -100.0f);		// back Right
		glVertex3f( 50.0f,-1.0f, 0.0f);		// front Right
		glVertex3f(-50.0f,-1.0f, 0.0f);		// front Left
	glEnd();
}

void Renderer::drawCovers(bool showTarget){
	static float backRot = -80;      // rotation of the covers on the side (in degrees)
	static float coverSpacing = 0.5; // space between the single covers
	static int xBorder = 2;          // Distance of the -1. and 1. cover towards the center
	static float backZ = -8.0f;   // zPos of covers in the back
	static float frontZ = -4.0f;  // zPos of cover in the front

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.1f);

	float centerOffset = displayPosition->getCenteredOffset();
	CollectionPos p = displayPosition->getCenteredPos() - 10;
	for (int o=-10; o < 15; ++o, ++p){
		glPushMatrix();
		float xPos;
		float yPos = 0;
		float zRot;
		float zPos;
		float co = -centerOffset + o;
		
		/*if ((o == 0) || ((centerOffset > 0) && (o == 1))){ // position for center covers
			zPos = frontZ +(abs(co) * (backZ - frontZ));
			zRot = co * backRot;
			xPos = co * xBorder;
		} else {
			zPos = backZ;
			zRot = (o > 0 ? 1 : -1) * backRot;
			xPos = (o > 0 ? 1 : -1) * (xBorder + coverSpacing*(abs(co)-1));
		}*/
		/*zPos = frontZ - co*co*0.3;
		zRot = rad2deg(atan(zPos*co));
		xPos = co*0.7;*/
		float radius = 3;
		float factor = co * 1.95f/radius;
		zPos = -8 - radius + cos(factor)*radius;
		xPos = sin(factor)*radius;
		zRot = float(rad2deg(factor));
		yPos = co*0.4f-1.6f;

		glTranslatef(xPos, yPos, zPos);
		glRotatef(zRot, 0, 1.0f, 0);

		ImgTexture* tex = gTexLoader->getLoadedImgTexture(p);
		tex->glBind();
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

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

		// calculate polygon dimensions
		float aspect = tex->getAspect();
		float scale = ((aspect < 0.6f) ? (aspect/0.6f) : 1.0f);
		float pw = 1.0f * 2 * scale;
		float ph = 2 * scale / aspect;
		float px = -pw/2;
		float py = -1;
		

		glPushName(SELECTION_CENTER + o);
		glBegin(GL_QUADS);
			if (isExtensionSupported("GL_EXT_fog_coord"))
				glFogCoordf(1);
			glTexCoord2f(0.0f, 1.0f); // top left
			glVertex3f(px, py+ph, 0);
			glTexCoord2f(1.0f, 1.0f); // top right
			glVertex3f(px+pw, py+ph, 0);

			if (isExtensionSupported("GL_EXT_fog_coord"))
				glFogCoordf(0);
			glTexCoord2f(1.0f, 0.0f); // bottom right
			glVertex3f(px+pw, py, 0);
			glTexCoord2f(0.0f, 0.0f); // bottom left
			glVertex3f(px, py, 0);
		glEnd();
		glPopName();

		if (showTarget){
			if (p == displayPosition->getTarget()){
				showTarget = false;

				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
				glBindTexture(GL_TEXTURE_2D,0);
				glLineWidth(4);
				glBegin(GL_QUADS);
					glColor3f( 1.0f,1.0f,1.0f);
					glVertex3f(px, py, 0);
					glVertex3f(px, py+ph, 0);
					glVertex3f(px+pw, py+ph, 0);
					glVertex3f(px+pw, py, 0);
				glEnd();		
			}
		}

		glPopMatrix();
	}

	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
}