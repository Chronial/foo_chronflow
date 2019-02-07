#include "cover_positions.h"

#include "cover_positions_compiler.h"
#include "lib/script_control.h"

void cfg_compiledCPInfoPtr::get_data_raw(stream_writer* p_stream,
                                         abort_callback& p_abort) {
  auto data = this->get();
  if (data) {
    p_stream->write_lendian_t(true, p_abort);
    data->serialize(p_stream, p_abort);
  } else {
    p_stream->write_lendian_t(false, p_abort);
  }
}

void cfg_compiledCPInfoPtr::set_data_raw(stream_reader* p_stream, t_size p_sizehint,
                                         abort_callback& p_abort) {
  bool configNotEmpty;
  p_stream->read_lendian_t(configNotEmpty, p_abort);
  if (configNotEmpty) {
    auto data = make_shared<CompiledCPInfo>();
    CompiledCPInfo::unserialize(*data, p_stream, p_abort);
    this->set(data);
  } else {
    this->reset();
  }
}

ScriptedCoverPositions::ScriptedCoverPositions() {
  cInfo = sessionCompiledCPInfo.get();
  if (!cInfo) {
    const CoverConfig* config = cfgCoverConfigs.getPtrByName(cfgCoverConfigSel);
    pfc::string8 errorMsg;
    if (!config || !setScript(config->script, errorMsg)) {
      if (!setScript(defaultCoverConfig, errorMsg)) {
        popup_message::g_show(errorMsg, "JScript Compile Error",
                              popup_message::icon_error);
        throw new pfc::exception(errorMsg);
      }
    }
  }
}

bool ScriptedCoverPositions::setScript(const char* script, pfc::string_base& errorMsg) {
  try {
    auto compiled = make_shared<CompiledCPInfo>();
    CPScriptCompiler compiler;
    if (compiler.compileScript(script, *compiled, errorMsg)) {
      sessionCompiledCPInfo.set(compiled);
      cInfo = std::move(compiled);
      return true;
    } else {
      return false;
    }
  } catch (_com_error) {
    errorMsg =
        "Windows Script Control not installed. Download it from "
        "<http://www.microsoft.com/downloads/"
        "details.aspx?FamilyId=D7E31492-2595-49E6-8C02-1426FEC693AC>.";
    return false;
  }
}

const fovAspectBehaviour& ScriptedCoverPositions::getAspectBehaviour() {
  return cInfo->aspectBehaviour;
}

const glVectord& ScriptedCoverPositions::getLookAt() {
  return cInfo->lookAt;
}

const glVectord& ScriptedCoverPositions::getUpVector() {
  return cInfo->upVector;
}

bool ScriptedCoverPositions::isMirrorPlaneEnabled() {
  return cInfo->showMirrorPlane;
}

const glVectord& ScriptedCoverPositions::getMirrorNormal() {
  return cInfo->mirrorNormal;
}

const glVectord& ScriptedCoverPositions::getMirrorCenter() {
  return cInfo->mirrorCenter;
}

const glVectord& ScriptedCoverPositions::getCameraPos() {
  return cInfo->cameraPos;
}

const int ScriptedCoverPositions::getFirstCover() {
  return cInfo->firstCover;
}

const int ScriptedCoverPositions::getLastCover() {
  return cInfo->lastCover;
}

double ScriptedCoverPositions::distanceToMirror(glVectord point) {
  return abs((cInfo->mirrorCenter - point) * cInfo->mirrorNormal);
}

glQuad ScriptedCoverPositions::getCoverQuad(float coverId, float coverAspect) {
  CoverPosInfo cPos = cInfo->getCoverPosInfo(coverId);

  double sizeLimAspect = cPos.sizeLim.w / cPos.sizeLim.h;

  float w;
  float h;
  if (coverAspect > sizeLimAspect) {
    w = cPos.sizeLim.w;
    h = w / coverAspect;
  } else {
    h = cPos.sizeLim.h;
    w = h * coverAspect;
  }

  glQuad out;

  // out.topLeft.x = -w/2 -align.x * w/2;
  out.topLeft.x = (-1 - cPos.alignment.x) * w / 2;
  out.bottomLeft.x = out.topLeft.x;
  out.topRight.x = (1 - cPos.alignment.x) * w / 2;
  out.bottomRight.x = out.topRight.x;

  out.topLeft.y = (1 - cPos.alignment.y) * h / 2;
  out.topRight.y = out.topLeft.y;
  out.bottomLeft.y = (-1 - cPos.alignment.y) * h / 2;
  out.bottomRight.y = out.bottomLeft.y;

  out.topLeft.z = 0;
  out.topRight.z = 0;
  out.bottomLeft.z = 0;
  out.bottomRight.z = 0;

  out.rotate(cPos.rotation.a, cPos.rotation.axis);

  out.topLeft = cPos.position + out.topLeft;
  out.topRight = cPos.position + out.topRight;
  out.bottomLeft = cPos.position + out.bottomLeft;
  out.bottomRight = cPos.position + out.bottomRight;

  return out;
}
