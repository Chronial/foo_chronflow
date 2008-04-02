#pragma once

class TextDisplay
{
	AppInstance* appInstance;
public:
	TextDisplay(AppInstance* appInstance);
	~TextDisplay();
public:
	enum HAlignment {
		left,
		center,
		right,
	};
	enum VAlignment {
		top,
		middle,
		bottom,
	};

public:
	void displayText(const char* text, int x, int y, HAlignment hAlign, VAlignment vAlign);
	void clearCache();
	void displayBitmapText(const char* text, int x, int y);

private:
	GLuint bitmapDisplayList;
	bool bitmapFontInitialized;
	void buildDisplayFont();

private:
	struct DisplayTexture {
		GLuint glTex;
		char * text;
		COLORREF color;
		int textWidth;
		int textHeight;
		int texWidth;
		int texHeight;
	};

private:
	DisplayTexture createTexture(const char * text);
	struct CacheElement : public DisplayTexture {
		unsigned int age;
		CacheElement(){
			age = ~0;
			text = 0;
		};
		CacheElement(const DisplayTexture& tex){
			text = tex.text;
			glTex = tex.glTex;
			color = tex.color;
			textWidth = tex.textWidth;
			textHeight = tex.textHeight;
			texWidth = tex.texWidth;
			texHeight = tex.texHeight;
		}
	};
	static const int CACHE_SIZE = 10;
	CacheElement texCache[CACHE_SIZE];
};
