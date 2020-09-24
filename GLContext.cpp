#include "GLContext.h"

#include "EngineWindow.h"

#ifdef _DEBUG
void GLAPIENTRY glMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar* message,
                                  const void* userParam) {
  console::out() << "GL: " << message;
  // if (type == GL_DEBUG_TYPE_ERROR || severity == GL_DEBUG_SEVERITY_HIGH)
  //  __debugbreak();
}
#endif

GLContext::GLContext(EngineWindow& window) {
  assert(currentContext == nullptr);
  currentContext = this;
  window.makeContextCurrent();
  static const bool glad =
      (0 != gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)));
  if (!glad)
    throw std::exception("Glad could not initialize OpenGl");
  if (!GLAD_GL_VERSION_2_1)
    throw std::exception("OpenGL 2.1 is not supported");
  if (!GLAD_GL_EXT_texture_filter_anisotropic)
    throw std::exception("Missing support for anisotropic textures");

  IF_DEBUG(glEnable(GL_DEBUG_OUTPUT));
  IF_DEBUG(glDebugMessageCallback(glMessageCallback, 0));

  glShadeModel(GL_SMOOTH);
  glClearDepth(1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
  glEnable(GL_TEXTURE_2D);

  glFogi(GL_FOG_MODE, GL_EXP);
  glFogf(GL_FOG_DENSITY, 5);
  glHint(GL_FOG_HINT, GL_NICEST);  // Per-Pixel Fog Calculation
  glFogi(GL_FOG_COORD_SRC, GL_FOG_COORD);  // Set Fog Based On Vertice Coordinates
}

GLContext::~GLContext() {
  if (!wasReset) {
    glfwMakeContextCurrent(nullptr);
  }
  currentContext = nullptr;
}

thread_local GLContext* GLContext::currentContext = nullptr;

void GLContext::checkGraphicsReset() {
  if (GLAD_GL_ARB_robustness && currentContext) {
    if (glGetGraphicsResetStatusARB() != GL_NO_ERROR) {
      currentContext->wasReset = true;
      std::chrono::milliseconds sleep_for{10};
      std::chrono::milliseconds const sleep_for_max{100};
      while (glGetGraphicsResetStatusARB() != GL_NO_ERROR) {
        std::this_thread::sleep_for(sleep_for);
        sleep_for = std::min(sleep_for * 2, sleep_for_max);
      }
      throw graphics_reset();
    }
  }
}
