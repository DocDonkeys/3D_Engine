#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

#include "Globals.h"
#include "Module.h"
#include "libs/glew/include/GL/glew.h"

class TextureLoader : public Module
{
public:
	TextureLoader(Application* app, const char* name = "null", bool start_enabled = true);
	~TextureLoader() {};

public:
	bool Init();
	bool Start();
	bool CleanUp();

public:
	uint LoadTextureFile(const char* path,
		uint target = GL_TEXTURE_2D,		// GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP
		int filterType = GL_NEAREST,		// GL_NEAREST, GL_LINEAR
		int fillingType = GL_REPEAT) const;	// GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT);

	uint CreateTexture(const void* imgData, uint width, uint height,
		int internalFormat = GL_RGBA,	// L_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, GL_RED_INTEGER, GL_RG_INTEGER, GL_RGB_INTEGER, --->
		uint format = GL_RGBA,			// ---> GL_BGR_INTEGER, GL_RGBA_INTEGER, GL_BGRA_INTEGER, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT, GL_DEPTH_STENCIL.
		uint target = GL_TEXTURE_2D,	// GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP
		int filterType = GL_NEAREST,	// GL_NEAREST, GL_LINEAR
		int fillingType = GL_REPEAT,	// GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT
		bool defaultTex = false) const;

	uint LoadDefaultTex() const;
	uint GetDefaultTex() const;

private:
	uint defaultTex = 0;
};

#endif //TEXTURELOADER_H