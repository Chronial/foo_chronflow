#include "style_manager.h"
#include "ConfigData.h"

namespace render {

using coverflow::configData;

COLORREF StyleManager::getTitleColor() {
  return configData->TitleColorCustom ? configData->TitleColor : cachedTitleColor;
};

LOGFONT StyleManager::getTitleFont() {
  return configData->TitleFontCustom ? configData->TitleFont : cachedTitleFont;
};

COLORREF StyleManager::getBgColor() {
  return configData->PanelBgCustom ? configData->PanelBg : cachedBgColor;
};

} // namespace
