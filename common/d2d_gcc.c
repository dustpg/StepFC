#define this_weak              __attribute__((weak))

typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[ 8 ];
} GUID;

//extern const GUID this_weak IID_ID2D1EffectImpl;
//extern const GUID this_weak IID_ID2D1DrawTransform;
//extern const GUID this_weak IID_ID2D1Transform;
//extern const GUID this_weak IID_ID2D1TransformNode;
#define MING_MISS_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

MING_MISS_GUID(IID_ID2D1EffectImpl   ,0xa248fd3f,0x3e6c,0x4e63,0x9f,0x03,0x7f,0x68,0xec,0xc9,0x1d,0xb9);
MING_MISS_GUID(IID_ID2D1DrawTransform,0x36bfdcb6,0x9739,0x435d,0xa3,0x0d,0xa6,0x53,0xbe,0xff,0x6a,0x6f);
MING_MISS_GUID(IID_ID2D1Transform    ,0xef1a287d,0x342a,0x4f76,0x8f,0xdb,0xda,0x0d,0x6e,0xa9,0xf9,0x2b);
MING_MISS_GUID(IID_ID2D1TransformNode,0xb2efe1e7,0x729f,0x4102,0x94,0x9f,0x50,0x5f,0xa2,0x1b,0xf6,0x66);


#include <stdio.h>


char * this_weak impl__gets_s(char * str, size_t num)
{
    gets(str);
}
