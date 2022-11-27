#ifdef RW_3DS
#include <3ds.h>
#include <citro3d.h>
#undef BIT

#define assert_break(x, fmt, ...) if(x){ printf(fmt, __ARGL__); svcBreak(USERBREAK_PANIC); }

#else

typedef uint32_t u32;
#define assert_break(x, fmt, ...) if(x){ printf(fmt, __ARGL__); exit(1); }
#define svcBreak(x) exit(1)

typedef enum
{
	GPU_RGBA8    = 0x0, ///< 8-bit Red + 8-bit Green + 8-bit Blue + 8-bit Alpha
	GPU_RGB8     = 0x1, ///< 8-bit Red + 8-bit Green + 8-bit Blue
	GPU_RGBA5551 = 0x2, ///< 5-bit Red + 5-bit Green + 5-bit Blue + 1-bit Alpha
	GPU_RGB565   = 0x3, ///< 5-bit Red + 6-bit Green + 5-bit Blue
	GPU_RGBA4    = 0x4, ///< 4-bit Red + 4-bit Green + 4-bit Blue + 4-bit Alpha
	GPU_LA8      = 0x5, ///< 8-bit Luminance + 8-bit Alpha
	GPU_HILO8    = 0x6, ///< 8-bit Hi + 8-bit Lo
	GPU_L8       = 0x7, ///< 8-bit Luminance
	GPU_A8       = 0x8, ///< 8-bit Alpha
	GPU_LA4      = 0x9, ///< 4-bit Luminance + 4-bit Alpha
	GPU_L4       = 0xA, ///< 4-bit Luminance
	GPU_A4       = 0xB, ///< 4-bit Alpha
	GPU_ETC1     = 0xC, ///< ETC1 texture compression
	GPU_ETC1A4   = 0xD, ///< ETC1 texture compression + 4-bit Alpha
} GPU_TEXCOLOR;

typedef enum
{
	GX_TRANSFER_FMT_RGBA8  = 0, ///< 8-bit Red + 8-bit Green + 8-bit Blue + 8-bit Alpha
	GX_TRANSFER_FMT_RGB8   = 1, ///< 8-bit Red + 8-bit Green + 8-bit Blue
	GX_TRANSFER_FMT_RGB565 = 2, ///< 5-bit Red + 6-bit Green + 5-bit Blue
	GX_TRANSFER_FMT_RGB5A1 = 3, ///< 5-bit Red + 5-bit Green + 5-bit Blue + 1-bit Alpha
	GX_TRANSFER_FMT_RGBA4  = 4  ///< 4-bit Red + 4-bit Green + 4-bit Blue + 4-bit Alpha
} GX_TRANSFER_FORMAT;

struct C3D_Tex
{
	uint8_t *data;
	size_t size;
	uint16_t width;
	uint16_t height;
	GPU_TEXCOLOR fmt;
	uint8_t maxLevel;
	uint8_t minLevel;
};


typedef void* C3D_FrameBuf;

#define vramFree
#define linearFree free

//Dirty fix to build Windows compilation
#ifndef _MSC_VER
static inline int C3D_TexCalcMaxLevel(uint32_t width, uint32_t height) {
	return (31-__builtin_clz(width < height ? width : height)) - 3; // avoid sizes smaller than 8
}
#else
static inline int C3D_TexCalcMaxLevel(uint32_t width, uint32_t height) {
	return 0;
}
#endif

static inline uint32_t
C3D_TexCalcLevelSize(uint32_t size, int level)
{
	return size >> (2*level);
}

static inline uint32_t
C3D_TexCalcTotalSize(uint32_t size, int maxLevel)
{
	return (size - C3D_TexCalcLevelSize(size,maxLevel+1)) * 4 / 3;
}

static uint8_t*
C3D_TexGetImagePtr(C3D_Tex* tex, uint8_t* data, int level, u32* size)
{
	if (size) *size = level >= 0 ? C3D_TexCalcLevelSize(tex->size, level) : C3D_TexCalcTotalSize(tex->size, tex->maxLevel);
	if (!level) return data;
	return (uint8_t*)data + (level > 0 ? C3D_TexCalcTotalSize(tex->size, level-1) : 0);
}

static uint8_t*
C3D_Tex2DGetImagePtr(C3D_Tex* tex, int level, u32* size)
{
	return C3D_TexGetImagePtr(tex, tex->data, level, size);
}

#endif

namespace rw {

#ifdef RW_3DS
struct EngineOpenParams
{
	void **window;
	bool32 fullscreen;
	int width, height;
	const char *windowtitle;
};
#endif

namespace c3d {

void registerPlatformPlugins(void);

extern Device renderdevice;
  
struct AttribDesc
{
	uint8 index;
	uint8 type;
	uint8 count;
	uint8 offset;
};
  
enum AttribIndices
{
	ATTRIB_POS = 0,		// v0
	ATTRIB_NORMAL,		// v1
	ATTRIB_COLOR,		// v2
	ATTRIB_TEXCOORDS0,	// v3
	MAX_ATTRIBS
	//ATTRIB_TEXCOORDS1,	// v4 (not actually used, matfx-env uses a uniform)
	//ATTRIB_TEXCOORDS2,	// v5 (not actually used)
	// ATTRIB_WEIGHTS,      // skinning done on cpu
	// ATTRIB_INDICES, 
	// ATTRIB_TEXCOORDS3,   // proctex, could be of some use
	// ATTRIB_TEXCOORDS4,
	// ATTRIB_TEXCOORDS5,
	// ATTRIB_TEXCOORDS6,
	// ATTRIB_TEXCOORDS7,
};

extern void *linearScratch;
  
// default uniform indices
//extern int32 u_matColor;
//extern int32 u_surfProps;

struct InstanceData
{
	uint32    numIndex;
	uint32    minVert;	// not used for rendering
	int32     numVertices;	//
	Material *material;
	bool32    vertexAlpha;
	uint32    program;
	uint16   *indexBuffer;
};

#ifdef RW_3DS
	
struct InstanceDataHeader : rw::InstanceDataHeader
{
	uint32      serialNumber;
	uint32      numMeshes;
	GPU_Primitive_t primType;
	uint8      *vertexBuffer;
	int32       numAttribs;
	AttribDesc  attribDesc[MAX_ATTRIBS];
	uint32      totalNumIndex;
	uint32      totalNumVertex;

	C3D_BufInfo  vbo;
	C3D_AttrInfo vao;
	ptrdiff_t    stride;

	InstanceData *inst;
};

struct Shader;
extern Shader *defaultShader;
extern Shader *im2dOverrideShader;

struct Im3DVertex
{
	V3d     position;
	uint8   r, g, b, a;
	float32 u, v;

	void setX(float32 x) { this->position.x = x; }
	void setY(float32 y) { this->position.y = y; }
	void setZ(float32 z) { this->position.z = z; }
	void setColor(uint8 r, uint8 g, uint8 b, uint8 a){
		this->r = r; this->g = g; this->b = b; this->a = a;
	}
	void setU(float32 u) { this->u = u; }
	void setV(float32 v) { this->v = v; }

	float getX(void) { return this->position.x; }
	float getY(void) { return this->position.y; }
	float getZ(void) { return this->position.z; }
	RGBA getColor(void) { return makeRGBA(this->r, this->g, this->b, this->a); }
	float getU(void) { return this->u; }
	float getV(void) { return this->v; }
};

struct Im2DVertex
{
	float32 x, y, z, w;
	uint8   r, g, b, a;
	float32 u, v;

	void setScreenX(float32 x) { this->x = x; }
	void setScreenY(float32 y) { this->y = y; }
	void setScreenZ(float32 z) { this->z = z; }
	void setCameraZ(float32 z) { this->w = z; }
	void setRecipCameraZ(float32 recipz) { this->w = 1.0f/recipz; }
	void setColor(uint8 r, uint8 g, uint8 b, uint8 a) {
		this->r = r; this->g = g; this->b = b; this->a = a;
	}
	void setU(float32 u, float recipz) { this->u = u; }
	void setV(float32 v, float recipz) { this->v = v; }

	float getScreenX(void) { return this->x; }
	float getScreenY(void) { return this->y; }
	float getScreenZ(void) { return this->z; }
	float getCameraZ(void) { return this->w; }
	float getRecipCameraZ(void) { return 1.0f/this->w; }
	RGBA getColor(void) { return makeRGBA(this->r, this->g, this->b, this->a); }
	float getU(void) { return this->u; }
	float getV(void) { return this->v; }
};

void genAttribPointers(InstanceDataHeader *header);
void setAttribPointers(InstanceDataHeader *header);
void setAttribsFixed(void);
  
// Render state

// Vertex shader bits
enum
{
	// These should be low so they could be used as indices
	VSLIGHT_DIRECT	= 1,
	VSLIGHT_POINT	= 2,
	VSLIGHT_SPOT	= 4,
	VSLIGHT_MASK	= 7,	// all the above
	// less critical
	VSLIGHT_AMBIENT = 8,
};

// per Scene
// void setProjectionMatrix(float32*);
// void setViewMatrix(float32*);

// per Object
void setWorldMatrix(Matrix*);
int32 setLights(WorldLights *lightData);

// per Mesh
void setTexture(int32 n, Texture *tex);
void setMaterial(const RGBA &color, const SurfaceProperties &surfaceprops, float extraSurfProp = 0.0f);
inline void setMaterial(uint32 flags, const RGBA &color, const SurfaceProperties &surfaceprops, float extraSurfProp = 0.0f)
{
	static RGBA white = { 255, 255, 255, 255 };
	if(flags & Geometry::MODULATE)
		setMaterial(color, surfaceprops, extraSurfProp);
	else
		setMaterial(white, surfaceprops, extraSurfProp);
}

void setAlphaBlend(bool32 enable);
bool32 getAlphaBlend(void);

void bindFramebuffer(uint32 fbo);
void bindTexture(C3D_Tex *tex);

void flushCache(void);

int cameraTilt(Camera *cam);
  
#endif

class ObjPipeline : public rw::ObjPipeline
{
public:
	void init(void);
	static ObjPipeline *create(void);

	void (*instanceCB)(Geometry *geo, InstanceDataHeader *header, bool32 reinstance);
	void (*uninstanceCB)(Geometry *geo, InstanceDataHeader *header);
	void (*renderCB)(Atomic *atomic, InstanceDataHeader *header);
};

void defaultInstanceCB(Geometry *geo, InstanceDataHeader *header, bool32 reinstance);
void defaultUninstanceCB(Geometry *geo, InstanceDataHeader *header);
void defaultRenderCB(Atomic *atomic, InstanceDataHeader *header);
int32 lightingCB(Atomic *atomic);

void drawInst_simple(InstanceDataHeader *header, InstanceData *inst);
// Emulate PS2 GS alpha test FB_ONLY case: failed alpha writes to frame- but not to depth buffer
void drawInst_GSemu(InstanceDataHeader *header, InstanceData *inst);
// This one switches between the above two depending on render state;
void drawInst(InstanceDataHeader *header, InstanceData *inst);


void *destroyNativeData(void *object, int32, int32);

ObjPipeline *makeDefaultPipeline(void);

// Native Texture and Raster

struct C3DRaster
{
	int32 totalSize;

	union
	{
		C3D_Tex *tex;
		void *zbuf;
	};
	
	GPU_TEXCOLOR format;
	GX_TRANSFER_FORMAT transfer;
	C3D_FrameBuf *fbo;
	
	// texture object
	bool  tilt;
	bool  onVram;
	bool  isCompressed;
	bool  hasAlpha;
	bool  autogenMipmap;
	int8  numLevels;
	int8  bpp;
	
	// cached filtermode and addressing
	uint8 filterMode;
	uint8 addressU;
	uint8 addressV;
	
	Raster *fboMate;	// color or zbuffer raster mate of this one
};

// void allocateETC(Raster *raster, bool hasAlpha);
static Raster *allocateETC(Raster *raster);
void rasterFromImage_etc1(rw::Raster *ras, rw::Image *img);

Texture *readNativeTexture(Stream *stream);
void writeNativeTexture(Texture *tex, Stream *stream);
uint32 getSizeNativeTexture(Texture *tex);
  
extern int32 nativeRasterOffset;
void registerNativeRaster(void);
#define GETC3DRASTEREXT(raster) PLUGINOFFSET(rw::c3d::C3DRaster, raster, rw::c3d::nativeRasterOffset)

#ifdef RW_3DS
void *safeLinearAlloc(size_t size);
#endif

void TexFree(C3DRaster *natras);
int TexAlloc(Raster *raster, int w, int h, GPU_TEXCOLOR fmt, int mipmap);
size_t fmtSize(GPU_TEXCOLOR fmt);
void texGenerateMipmap(C3D_Tex* tex);

}
}
