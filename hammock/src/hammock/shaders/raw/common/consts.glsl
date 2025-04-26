#ifndef CONSTS_GLSL
#define CONSTS_GLSL

#define INVALID_TEXTURE -1

#define MAX_MESHES 256

// Visibility Flags
#define VISIBILITY_NONE            0       // 00000
#define VISIBILITY_VISIBLE         (1 << 0) // 00001
#define VISIBILITY_OPAQUE          (1 << 1) // 00010
#define VISIBILITY_TRANSPARENT     (1 << 2) // 00100
#define VISIBILITY_CASTS_SHADOW    (1 << 3) // 01000
#define VISIBILITY_RECEIVES_SHADOW (1 << 4) // 10000

const float PI = 3.14159265359;

#endif // CONSTS_GLSL