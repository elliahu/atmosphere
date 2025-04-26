/*
  HandmadeMath.h v2.0.0

  This is a single header file with a bunch of useful types and functions for
  games and graphics. Consider it a lightweight alternative to GLM that works
  both C and C++.

  =============================================================================
  CONFIG
  =============================================================================

  By default, all angles in Handmade Math are specified in radians. However, it
  can be configured to use degrees or turns instead. Use one of the following
  defines to specify the default unit for angles:

    #define HANDMADE_MATH_USE_RADIANS
    #define HANDMADE_MATH_USE_DEGREES
    #define HANDMADE_MATH_USE_TURNS

  Regardless of the default angle, you can use the following functions to
  specify an angle in a particular unit:

    HmckAngleRad(radians)
    HmckAngleDeg(degrees)
    HmckAngleTurn(turns)

  The definitions of these functions change depending on the default unit.

  -----------------------------------------------------------------------------

  Handmade Math ships with SSE (SIMD) implementations of several common
  operations. To disable the use of SSE intrinsics, you must define
  HANDMADE_MATH_NO_SSE before including this file:

    #define HANDMADE_MATH_NO_SSE
    #include "HandmadeMath.h"

  -----------------------------------------------------------------------------

  To use Handmade Math without the C runtime library, you must provide your own
  implementations of basic math functions. Otherwise, HandmadeMath.h will use
  the runtime library implementation of these functions.

  Define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS and provide your own
  implementations of HmckSINF, HmckCOSF, HmckTANF, HmckACOSF, and HmckSQRTF
  before including HandmadeMath.h, like so:

    #define HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS
    #define HmckSINF MySinF
    #define HmckCOSF MyCosF
    #define HmckTANF MyTanF
    #define HmckACOSF MyACosF
    #define HmckSQRTF MySqrtF
    #include "HandmadeMath.h"
  
  By default, it is assumed that your math functions take radians. To use
  different units, you must define HmckANGLE_USER_TO_INTERNAL and
  HmckANGLE_INTERNAL_TO_USER. For example, if you want to use degrees in your
  code but your math functions use turns:

    #define HmckANGLE_USER_TO_INTERNAL(a) ((a)*HmckDegToTurn)
    #define HmckANGLE_INTERNAL_TO_USER(a) ((a)*HmckTurnToDeg)

  =============================================================================
  
  LICENSE

  This software is in the public domain. Where that dedication is not
  recognized, you are granted a perpetual, irrevocable license to copy,
  distribute, and modify this file as you see fit.

  =============================================================================

  CREDITS

  Originally written by Zakary Strange.

  Functionality:
   Zakary Strange (strangezak@protonmail.com && @strangezak)
   Matt Mascarenhas (@miblo_)
   Aleph
   FieryDrake (@fierydrake)
   Gingerbill (@TheGingerBill)
   Ben Visness (@bvisness)
   Trinton Bullard (@Peliex_Dev)
   @AntonDan
   Logan Forman (@dev_dwarf)

  Fixes:
   Jeroen van Rijn (@J_vanRijn)
   Kiljacken (@Kiljacken)
   Insofaras (@insofaras)
   Daniel Gibson (@DanielGibson)
*/

#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

// Dummy macros for when test framework is not present.
#ifndef COVERAGE
# define COVERAGE(a, b)
#endif

#ifndef ASSERT_COVERED
# define ASSERT_COVERED(a)
#endif

/* let's figure out if SSE is really available (unless disabled anyway)
   (it isn't on non-x86/x86_64 platforms or even x86 without explicit SSE support)
   => only use "#ifdef HANDMADE_MATH__USE_SSE" to check for SSE support below this block! */
#ifndef HANDMADE_MATH_NO_SSE
# ifdef _MSC_VER /* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#  if defined(_M_AMD64) || ( defined(_M_IX86_FP) && _M_IX86_FP >= 1 )
#   define HANDMADE_MATH__USE_SSE 1
#  endif
# else /* not MSVC, probably GCC, clang, icc or something that doesn't support SSE anyway */
#  ifdef __SSE__ /* they #define __SSE__ if it's supported */
#   define HANDMADE_MATH__USE_SSE 1
#  endif /*  __SSE__ */
# endif /* not _MSC_VER */
#endif /* #ifndef HANDMADE_MATH_NO_SSE */

#if (!defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L)
# define HANDMADE_MATH__USE_C11_GENERICS 1
#endif

#ifdef HANDMADE_MATH__USE_SSE
# include <xmmintrin.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable:4201)
#endif

#if defined(__GNUC__) || defined(__clang__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wfloat-equal"
# if (defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 8)) || defined(__clang__)
#  pragma GCC diagnostic ignored "-Wmissing-braces"
# endif
# ifdef __clang__
#  pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
# endif
#endif

#if defined(__GNUC__) || defined(__clang__)
# define HmckDEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
# define HmckDEPRECATED(msg) __declspec(deprecated(msg))
#else
# define HmckDEPRECATED(msg)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined(HANDMADE_MATH_USE_DEGREES) \
    && !defined(HANDMADE_MATH_USE_TURNS) \
    && !defined(HANDMADE_MATH_USE_RADIANS)
# define HANDMADE_MATH_USE_RADIANS
#endif
    
#define HmckPI 3.14159265358979323846
#define HmckPI32 3.14159265359f
#define HmckDEG180 180.0
#define HmckDEG18032 180.0f
#define HmckTURNHALF 0.5
#define HmckTURNHALF32 0.5f
#define HmckRadToDeg ((float)(HmckDEG180/HmckPI))
#define HmckRadToTurn ((float)(HmckTURNHALF/HmckPI))
#define HmckDegToRad ((float)(HmckPI/HmckDEG180))
#define HmckDegToTurn ((float)(HmckTURNHALF/HmckDEG180))
#define HmckTurnToRad ((float)(HmckPI/HmckTURNHALF))
#define HmckTurnToDeg ((float)(HmckDEG180/HmckTURNHALF))

#if defined(HANDMADE_MATH_USE_RADIANS)
# define HmckAngleRad(a) (a)
# define HmckAngleDeg(a) ((a)*HmckDegToRad)
# define HmckAngleTurn(a) ((a)*HmckTurnToRad)
#elif defined(HANDMADE_MATH_USE_DEGREES)
# define HmckAngleRad(a) ((a)*HmckRadToDeg)
# define HmckAngleDeg(a) (a)
# define HmckAngleTurn(a) ((a)*HmckTurnToDeg)
#elif defined(HANDMADE_MATH_USE_TURNS)
# define HmckAngleRad(a) ((a)*HmckRadToTurn)
# define HmckAngleDeg(a) ((a)*HmckDegToTurn)
# define HmckAngleTurn(a) (a)
#endif

#if !defined(HANDMADE_MATH_PROVIDE_MATH_FUNCTIONS)
# include <math.h>
# define HmckSINF sinf
# define HmckCOSF cosf
# define HmckTANF tanf
# define HmckSQRTF sqrtf
# define HmckACOSF acosf
#endif

#if !defined(HmckANGLE_USER_TO_INTERNAL)
# define HmckANGLE_USER_TO_INTERNAL(a) (HmckToRad(a))
#endif

#if !defined(HmckANGLE_INTERNAL_TO_USER)
# if defined(HANDMADE_MATH_USE_RADIANS)
#  define HmckANGLE_INTERNAL_TO_USER(a) (a)
# elif defined(HANDMADE_MATH_USE_DEGREES)
#  define HmckANGLE_INTERNAL_TO_USER(a) ((a)*HmckRadToDeg)
# elif defined(HANDMADE_MATH_USE_TURNS)
#  define HmckANGLE_INTERNAL_TO_USER(a) ((a)*HmckRadToTurn)
# endif
#endif

#define HmckMIN(a, b) ((a) > (b) ? (b) : (a))
#define HmckMAX(a, b) ((a) < (b) ? (b) : (a))
#define HmckABS(a) ((a) > 0 ? (a) : -(a))
#define HmckMOD(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define HmckSQUARE(x) ((x) * (x))

typedef union HmckVec2
{
    struct
    {
        float X, Y;
    };

    struct
    {
        float U, V;
    };

    struct
    {
        float Left, Right;
    };

    struct
    {
        float Width, Height;
    };

    float Elements[2];

#ifdef __cplusplus
    inline float &operator[](const int &Index)
    {
        return Elements[Index];
    }
#endif
} HmckVec2;

typedef union HmckVec3
{
    struct
    {
        float X, Y, Z;
    };

    struct
    {
        float U, V, W;
    };

    struct
    {
        float R, G, B;
    };

    struct
    {
        HmckVec2 XY;
        float _Ignored0;
    };

    struct
    {
        float _Ignored1;
        HmckVec2 YZ;
    };

    struct
    {
        HmckVec2 UV;
        float _Ignored2;
    };

    struct
    {
        float _Ignored3;
        HmckVec2 VW;
    };

    float Elements[3];

#ifdef __cplusplus
    inline float &operator[](const int &Index)
    {
        return Elements[Index];
    }
#endif
} HmckVec3;

typedef union HmckVec4
{
    struct
    {
        union
        {
            HmckVec3 XYZ;
            struct
            {
                float X, Y, Z;
            };
        };

        float W;
    };
    struct
    {
        union
        {
            HmckVec3 RGB;
            struct
            {
                float R, G, B;
            };
        };

        float A;
    };

    struct
    {
        HmckVec2 XY;
        float _Ignored0;
        float _Ignored1;
    };

    struct
    {
        float _Ignored2;
        HmckVec2 YZ;
        float _Ignored3;
    };

    struct
    {
        float _Ignored4;
        float _Ignored5;
        HmckVec2 ZW;
    };

    float Elements[4];

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSE;
#endif

#ifdef __cplusplus
    inline float &operator[](const int &Index)
    {
        return Elements[Index];
    }
#endif
} HmckVec4;

typedef union HmckMat2
{
    float Elements[2][2];
    HmckVec2 Columns[2];

#ifdef __cplusplus
    inline HmckVec2 &operator[](const int &Index)
    {
        return Columns[Index];
    }
#endif
} HmckMat2;
    
typedef union HmckMat3
{
    float Elements[3][3];
    HmckVec3 Columns[3];

#ifdef __cplusplus
    inline HmckVec3 &operator[](const int &Index)
    {
        return Columns[Index];
    }
#endif
} HmckMat3;

typedef union HmckMat4
{
    float Elements[4][4];
    HmckVec4 Columns[4];

#ifdef __cplusplus
    inline HmckVec4 &operator[](const int &Index)
    {
        return Columns[Index];
    }
#endif
} HmckMat4;

typedef union HmckQuat
{
    struct
    {
        union
        {
            HmckVec3 XYZ;
            struct
            {
                float X, Y, Z;
            };
        };

        float W;
    };

    float Elements[4];

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSE;
#endif
} HmckQuat;

typedef signed int HmckBool;

/*
 * Angle unit conversion functions
 */
static inline float HmckToRad(float Angle)
{
#if defined(HANDMADE_MATH_USE_RADIANS)
    float Result = Angle;
#elif defined(HANDMADE_MATH_USE_DEGREES) 
    float Result = Angle * HmckDegToRad;
#elif defined(HANDMADE_MATH_USE_TURNS)
    float Result = Angle * HmckTurnToRad;
#endif
    
    return Result;
}

static inline float HmckToDeg(float Angle)
{
#if defined(HANDMADE_MATH_USE_RADIANS)
    float Result = Angle * HmckRadToDeg;
#elif defined(HANDMADE_MATH_USE_DEGREES) 
    float Result = Angle;
#elif defined(HANDMADE_MATH_USE_TURNS)
    float Result = Angle * HmckTurnToDeg;
#endif
    
    return Result;
}

static inline float HmckToTurn(float Angle)
{
#if defined(HANDMADE_MATH_USE_RADIANS)
    float Result = Angle * HmckRadToTurn;
#elif defined(HANDMADE_MATH_USE_DEGREES) 
    float Result = Angle * HmckDegToTurn;
#elif defined(HANDMADE_MATH_USE_TURNS)
    float Result = Angle;
#endif
    
    return Result;
}

/*
 * Floating-point math functions
 */

COVERAGE(HmckSinF, 1)
static inline float HmckSinF(float Angle)
{
    ASSERT_COVERED(HmckSinF);
    return HmckSINF(HmckANGLE_USER_TO_INTERNAL(Angle));
}

COVERAGE(HmckCosF, 1)
static inline float HmckCosF(float Angle)
{
    ASSERT_COVERED(HmckCosF);
    return HmckCOSF(HmckANGLE_USER_TO_INTERNAL(Angle));
}

COVERAGE(HmckTanF, 1)
static inline float HmckTanF(float Angle)
{
    ASSERT_COVERED(HmckTanF);
    return HmckTANF(HmckANGLE_USER_TO_INTERNAL(Angle));
}

COVERAGE(HmckACosF, 1)
static inline float HmckACosF(float Arg)
{
    ASSERT_COVERED(HmckACosF);
    return HmckANGLE_INTERNAL_TO_USER(HmckACOSF(Arg));
}

COVERAGE(HmckSqrtF, 1)
static inline float HmckSqrtF(float Float)
{
    ASSERT_COVERED(HmckSqrtF);

    float Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 In = _mm_set_ss(Float);
    __m128 Out = _mm_sqrt_ss(In);
    Result = _mm_cvtss_f32(Out);
#else
    Result = HmckSQRTF(Float);
#endif

    return Result;
}

COVERAGE(HmckInvSqrtF, 1)
static inline float HmckInvSqrtF(float Float)
{
    ASSERT_COVERED(HmckInvSqrtF);

    float Result;

    Result = 1.0f/HmckSqrtF(Float);

    return Result;
}


/*
 * Utility functions
 */

COVERAGE(HmckLerp, 1)
static inline float HmckLerp(float A, float Time, float B)
{
    ASSERT_COVERED(HmckLerp);
    return (1.0f - Time) * A + Time * B;
}

COVERAGE(HmckClamp, 1)
static inline float HmckClamp(float Min, float Value, float Max)
{
    ASSERT_COVERED(HmckClamp);

    float Result = Value;

    if (Result < Min)
    {
        Result = Min;
    }

    if (Result > Max)
    {
        Result = Max;
    }

    return Result;
}


/*
 * Vector initialization
 */

COVERAGE(HmckV2, 1)
static inline HmckVec2 HmckV2(float X, float Y)
{
    ASSERT_COVERED(HmckV2);

    HmckVec2 Result;
    Result.X = X;
    Result.Y = Y;

    return Result;
}

COVERAGE(HmckV3, 1)
static inline HmckVec3 HmckV3(float X, float Y, float Z)
{
    ASSERT_COVERED(HmckV3);

    HmckVec3 Result;
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;

    return Result;
}

COVERAGE(HmckV4, 1)
static inline HmckVec4 HmckV4(float X, float Y, float Z, float W)
{
    ASSERT_COVERED(HmckV4);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_setr_ps(X, Y, Z, W);
#else
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
#endif

    return Result;
}

COVERAGE(HmckV4V, 1)
static inline HmckVec4 HmckV4V(HmckVec3 Vector, float W)
{
    ASSERT_COVERED(HmckV4V);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_setr_ps(Vector.X, Vector.Y, Vector.Z, W);
#else
    Result.XYZ = Vector;
    Result.W = W;
#endif

    return Result;
}


/*
 * Binary vector operations
 */

COVERAGE(HmckAddV2, 1)
static inline HmckVec2 HmckAddV2(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckAddV2);

    HmckVec2 Result;
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;

    return Result;
}

COVERAGE(HmckAddV3, 1)
static inline HmckVec3 HmckAddV3(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckAddV3);

    HmckVec3 Result;
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;

    return Result;
}

COVERAGE(HmckAddV4, 1)
static inline HmckVec4 HmckAddV4(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckAddV4);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;
#endif

    return Result;
}

COVERAGE(HmckSubV2, 1)
static inline HmckVec2 HmckSubV2(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckSubV2);

    HmckVec2 Result;
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;

    return Result;
}

COVERAGE(HmckSubV3, 1)
static inline HmckVec3 HmckSubV3(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckSubV3);

    HmckVec3 Result;
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;

    return Result;
}

COVERAGE(HmckSubV4, 1)
static inline HmckVec4 HmckSubV4(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckSubV4);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    Result.W = Left.W - Right.W;
#endif

    return Result;
}

COVERAGE(HmckMulV2, 1)
static inline HmckVec2 HmckMulV2(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckMulV2);

    HmckVec2 Result;
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;

    return Result;
}

COVERAGE(HmckMulV2F, 1)
static inline HmckVec2 HmckMulV2F(HmckVec2 Left, float Right)
{
    ASSERT_COVERED(HmckMulV2F);

    HmckVec2 Result;
    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;

    return Result;
}

COVERAGE(HmckMulV3, 1)
static inline HmckVec3 HmckMulV3(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckMulV3);

    HmckVec3 Result;
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;

    return Result;
}

COVERAGE(HmckMulV3F, 1)
static inline HmckVec3 HmckMulV3F(HmckVec3 Left, float Right)
{
    ASSERT_COVERED(HmckMulV3F);

    HmckVec3 Result;
    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    Result.Z = Left.Z * Right;

    return Result;
}

COVERAGE(HmckMulV4, 1)
static inline HmckVec4 HmckMulV4(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckMulV4);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_mul_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;
    Result.W = Left.W * Right.W;
#endif

    return Result;
}

COVERAGE(HmckMulV4F, 1)
static inline HmckVec4 HmckMulV4F(HmckVec4 Left, float Right)
{
    ASSERT_COVERED(HmckMulV4F);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Right);
    Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    Result.Z = Left.Z * Right;
    Result.W = Left.W * Right;
#endif

    return Result;
}

COVERAGE(HmckDivV2, 1)
static inline HmckVec2 HmckDivV2(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckDivV2);

    HmckVec2 Result;
    Result.X = Left.X / Right.X;
    Result.Y = Left.Y / Right.Y;

    return Result;
}

COVERAGE(HmckDivV2F, 1)
static inline HmckVec2 HmckDivV2F(HmckVec2 Left, float Right)
{
    ASSERT_COVERED(HmckDivV2F);

    HmckVec2 Result;
    Result.X = Left.X / Right;
    Result.Y = Left.Y / Right;

    return Result;
}

COVERAGE(HmckDivV3, 1)
static inline HmckVec3 HmckDivV3(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckDivV3);

    HmckVec3 Result;
    Result.X = Left.X / Right.X;
    Result.Y = Left.Y / Right.Y;
    Result.Z = Left.Z / Right.Z;

    return Result;
}

COVERAGE(HmckDivV3F, 1)
static inline HmckVec3 HmckDivV3F(HmckVec3 Left, float Right)
{
    ASSERT_COVERED(HmckDivV3F);

    HmckVec3 Result;
    Result.X = Left.X / Right;
    Result.Y = Left.Y / Right;
    Result.Z = Left.Z / Right;

    return Result;
}

COVERAGE(HmckDivV4, 1)
static inline HmckVec4 HmckDivV4(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckDivV4);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_div_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X / Right.X;
    Result.Y = Left.Y / Right.Y;
    Result.Z = Left.Z / Right.Z;
    Result.W = Left.W / Right.W;
#endif

    return Result;
}

COVERAGE(HmckDivV4F, 1)
static inline HmckVec4 HmckDivV4F(HmckVec4 Left, float Right)
{
    ASSERT_COVERED(HmckDivV4F);

    HmckVec4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Right);
    Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X / Right;
    Result.Y = Left.Y / Right;
    Result.Z = Left.Z / Right;
    Result.W = Left.W / Right;
#endif

    return Result;
}

COVERAGE(HmckEqV2, 1)
static inline HmckBool HmckEqV2(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckEqV2);
    return Left.X == Right.X && Left.Y == Right.Y;
}

COVERAGE(HmckEqV3, 1)
static inline HmckBool HmckEqV3(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckEqV3);
    return Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z;
}

COVERAGE(HmckEqV4, 1)
static inline HmckBool HmckEqV4(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckEqV4);
    return Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z && Left.W == Right.W;
}

COVERAGE(HmckDotV2, 1)
static inline float HmckDotV2(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckDotV2);
    return (Left.X * Right.X) + (Left.Y * Right.Y);
}

COVERAGE(HmckDotV3, 1)
static inline float HmckDotV3(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckDotV3);
    return (Left.X * Right.X) + (Left.Y * Right.Y) + (Left.Z * Right.Z);
}

COVERAGE(HmckDotV4, 1)
static inline float HmckDotV4(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckDotV4);

    float Result;

    // NOTE(zak): IN the future if we wanna check what version SSE is support
    // we can use _mm_dp_ps (4.3) but for now we will use the old way.
    // Or a r = _mm_mul_ps(v1, v2), r = _mm_hadd_ps(r, r), r = _mm_hadd_ps(r, r) for SSE3
#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&Result, SSEResultOne);
#else
    Result = ((Left.X * Right.X) + (Left.Z * Right.Z)) + ((Left.Y * Right.Y) + (Left.W * Right.W));
#endif

    return Result;
}

COVERAGE(HmckCross, 1)
static inline HmckVec3 HmckCross(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckCross);

    HmckVec3 Result;
    Result.X = (Left.Y * Right.Z) - (Left.Z * Right.Y);
    Result.Y = (Left.Z * Right.X) - (Left.X * Right.Z);
    Result.Z = (Left.X * Right.Y) - (Left.Y * Right.X);

    return Result;
}


/*
 * Unary vector operations
 */

COVERAGE(HmckLenSqrV2, 1)
static inline float HmckLenSqrV2(HmckVec2 A)
{
    ASSERT_COVERED(HmckLenSqrV2);
    return HmckDotV2(A, A);
}

COVERAGE(HmckLenSqrV3, 1)
static inline float HmckLenSqrV3(HmckVec3 A)
{
    ASSERT_COVERED(HmckLenSqrV3);
    return HmckDotV3(A, A);
}

COVERAGE(HmckLenSqrV4, 1)
static inline float HmckLenSqrV4(HmckVec4 A)
{
    ASSERT_COVERED(HmckLenSqrV4);
    return HmckDotV4(A, A);
}

COVERAGE(HmckLenV2, 1)
static inline float HmckLenV2(HmckVec2 A)
{
    ASSERT_COVERED(HmckLenV2);
    return HmckSqrtF(HmckLenSqrV2(A));
}

COVERAGE(HmckLenV3, 1)
static inline float HmckLenV3(HmckVec3 A)
{
    ASSERT_COVERED(HmckLenV3);
    return HmckSqrtF(HmckLenSqrV3(A));
}

COVERAGE(HmckLenV4, 1)
static inline float HmckLenV4(HmckVec4 A)
{
    ASSERT_COVERED(HmckLenV4);
    return HmckSqrtF(HmckLenSqrV4(A));
}

COVERAGE(HmckNormV2, 1)
static inline HmckVec2 HmckNormV2(HmckVec2 A)
{
    ASSERT_COVERED(HmckNormV2);
    return HmckMulV2F(A, HmckInvSqrtF(HmckDotV2(A, A)));
}

COVERAGE(HmckNormV3, 1)
static inline HmckVec3 HmckNormV3(HmckVec3 A)
{
    ASSERT_COVERED(HmckNormV3);
    return HmckMulV3F(A, HmckInvSqrtF(HmckDotV3(A, A)));
}

COVERAGE(HmckNormV4, 1)
static inline HmckVec4 HmckNormV4(HmckVec4 A)
{
    ASSERT_COVERED(HmckNormV4);
    return HmckMulV4F(A, HmckInvSqrtF(HmckDotV4(A, A)));
}

/*
 * Utility vector functions
 */

COVERAGE(HmckLerpV2, 1)
static inline HmckVec2 HmckLerpV2(HmckVec2 A, float Time, HmckVec2 B)
{
    ASSERT_COVERED(HmckLerpV2);
    return HmckAddV2(HmckMulV2F(A, 1.0f - Time), HmckMulV2F(B, Time));
}

COVERAGE(HmckLerpV3, 1)
static inline HmckVec3 HmckLerpV3(HmckVec3 A, float Time, HmckVec3 B)
{
    ASSERT_COVERED(HmckLerpV3);
    return HmckAddV3(HmckMulV3F(A, 1.0f - Time), HmckMulV3F(B, Time));
}

COVERAGE(HmckLerpV4, 1)
static inline HmckVec4 HmckLerpV4(HmckVec4 A, float Time, HmckVec4 B)
{
    ASSERT_COVERED(HmckLerpV4);
    return HmckAddV4(HmckMulV4F(A, 1.0f - Time), HmckMulV4F(B, Time));
}

/*
 * SSE stuff
 */

COVERAGE(HmckLinearCombineV4M4, 1)
static inline HmckVec4 HmckLinearCombineV4M4(HmckVec4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckLinearCombineV4M4);

    HmckVec4 Result;
#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x00), Right.Columns[0].SSE);
    Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0x55), Right.Columns[1].SSE));
    Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xaa), Right.Columns[2].SSE));
    Result.SSE = _mm_add_ps(Result.SSE, _mm_mul_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, 0xff), Right.Columns[3].SSE));
#else
    Result.X = Left.Elements[0] * Right.Columns[0].X;
    Result.Y = Left.Elements[0] * Right.Columns[0].Y;
    Result.Z = Left.Elements[0] * Right.Columns[0].Z;
    Result.W = Left.Elements[0] * Right.Columns[0].W;

    Result.X += Left.Elements[1] * Right.Columns[1].X;
    Result.Y += Left.Elements[1] * Right.Columns[1].Y;
    Result.Z += Left.Elements[1] * Right.Columns[1].Z;
    Result.W += Left.Elements[1] * Right.Columns[1].W;

    Result.X += Left.Elements[2] * Right.Columns[2].X;
    Result.Y += Left.Elements[2] * Right.Columns[2].Y;
    Result.Z += Left.Elements[2] * Right.Columns[2].Z;
    Result.W += Left.Elements[2] * Right.Columns[2].W;

    Result.X += Left.Elements[3] * Right.Columns[3].X;
    Result.Y += Left.Elements[3] * Right.Columns[3].Y;
    Result.Z += Left.Elements[3] * Right.Columns[3].Z;
    Result.W += Left.Elements[3] * Right.Columns[3].W;
#endif

    return Result;
}

/*
 * 2x2 Matrices
 */

COVERAGE(HmckM2, 1)
static inline HmckMat2 HmckM2(void)
{
    ASSERT_COVERED(HmckM2);
    HmckMat2 Result = {0};
    return Result;
}

COVERAGE(HmckM2D, 1)
static inline HmckMat2 HmckM2D(float Diagonal)
{
    ASSERT_COVERED(HmckM2D);
    
    HmckMat2 Result = {0};
    Result.Elements[0][0] = Diagonal;
    Result.Elements[1][1] = Diagonal;

    return Result;
}

COVERAGE(HmckTransposeM2, 1)
static inline HmckMat2 HmckTransposeM2(HmckMat2 Matrix)
{
    ASSERT_COVERED(HmckTransposeM2);
    
    HmckMat2 Result = Matrix;

    Result.Elements[0][1] = Matrix.Elements[1][0];
    Result.Elements[1][0] = Matrix.Elements[0][1];
    
    return Result;
}

COVERAGE(HmckAddM2, 1)
static inline HmckMat2 HmckAddM2(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckAddM2);
    
    HmckMat2 Result;

    Result.Elements[0][0] = Left.Elements[0][0] + Right.Elements[0][0];
    Result.Elements[0][1] = Left.Elements[0][1] + Right.Elements[0][1];
    Result.Elements[1][0] = Left.Elements[1][0] + Right.Elements[1][0];
    Result.Elements[1][1] = Left.Elements[1][1] + Right.Elements[1][1];
   
    return Result;    
}

COVERAGE(HmckSubM2, 1)
static inline HmckMat2 HmckSubM2(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckSubM2);
    
    HmckMat2 Result;

    Result.Elements[0][0] = Left.Elements[0][0] - Right.Elements[0][0];
    Result.Elements[0][1] = Left.Elements[0][1] - Right.Elements[0][1];
    Result.Elements[1][0] = Left.Elements[1][0] - Right.Elements[1][0];
    Result.Elements[1][1] = Left.Elements[1][1] - Right.Elements[1][1];
    
    return Result;
}

COVERAGE(HmckMulM2V2, 1)
static inline HmckVec2 HmckMulM2V2(HmckMat2 Matrix, HmckVec2 Vector)
{
    ASSERT_COVERED(HmckMulM2V2);
    
    HmckVec2 Result;

    Result.X = Vector.Elements[0] * Matrix.Columns[0].X;
    Result.Y = Vector.Elements[0] * Matrix.Columns[0].Y;

    Result.X += Vector.Elements[1] * Matrix.Columns[1].X;
    Result.Y += Vector.Elements[1] * Matrix.Columns[1].Y;

    return Result;    
}

COVERAGE(HmckMulM2, 1)
static inline HmckMat2 HmckMulM2(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckMulM2);
    
    HmckMat2 Result;
    Result.Columns[0] = HmckMulM2V2(Left, Right.Columns[0]);
    Result.Columns[1] = HmckMulM2V2(Left, Right.Columns[1]);

    return Result;    
}

COVERAGE(HmckMulM2F, 1)
static inline HmckMat2 HmckMulM2F(HmckMat2 Matrix, float Scalar)
{
    ASSERT_COVERED(HmckMulM2F);
    
    HmckMat2 Result;

    Result.Elements[0][0] = Matrix.Elements[0][0] * Scalar;
    Result.Elements[0][1] = Matrix.Elements[0][1] * Scalar;
    Result.Elements[1][0] = Matrix.Elements[1][0] * Scalar;
    Result.Elements[1][1] = Matrix.Elements[1][1] * Scalar;
    
    return Result;
}

COVERAGE(HmckDivM2F, 1)
static inline HmckMat2 HmckDivM2F(HmckMat2 Matrix, float Scalar)
{
    ASSERT_COVERED(HmckDivM2F);
    
    HmckMat2 Result;

    Result.Elements[0][0] = Matrix.Elements[0][0] / Scalar;
    Result.Elements[0][1] = Matrix.Elements[0][1] / Scalar;
    Result.Elements[1][0] = Matrix.Elements[1][0] / Scalar;
    Result.Elements[1][1] = Matrix.Elements[1][1] / Scalar;

    return Result;
}

COVERAGE(HmckDeterminantM2, 1)
static inline float HmckDeterminantM2(HmckMat2 Matrix)
{
    ASSERT_COVERED(HmckDeterminantM2);
    return Matrix.Elements[0][0]*Matrix.Elements[1][1] - Matrix.Elements[0][1]*Matrix.Elements[1][0];
}


COVERAGE(HmckInvGeneralM2, 1)
static inline HmckMat2 HmckInvGeneralM2(HmckMat2 Matrix)
{
    ASSERT_COVERED(HmckInvGeneralM2);

    HmckMat2 Result;
    float InvDeterminant = 1.0f / HmckDeterminantM2(Matrix);
    Result.Elements[0][0] = InvDeterminant * +Matrix.Elements[1][1];
    Result.Elements[1][1] = InvDeterminant * +Matrix.Elements[0][0];
    Result.Elements[0][1] = InvDeterminant * -Matrix.Elements[0][1];
    Result.Elements[1][0] = InvDeterminant * -Matrix.Elements[1][0];

    return Result;
}

/*
 * 3x3 Matrices
 */

COVERAGE(HmckM3, 1)
static inline HmckMat3 HmckM3(void)
{
    ASSERT_COVERED(HmckM3);
    HmckMat3 Result = {0};
    return Result;
}

COVERAGE(HmckM3D, 1)
static inline HmckMat3 HmckM3D(float Diagonal)
{
    ASSERT_COVERED(HmckM3D);
    
    HmckMat3 Result = {0};
    Result.Elements[0][0] = Diagonal;
    Result.Elements[1][1] = Diagonal;
    Result.Elements[2][2] = Diagonal;

    return Result;
}

COVERAGE(HmckTransposeM3, 1)
static inline HmckMat3 HmckTransposeM3(HmckMat3 Matrix)
{
    ASSERT_COVERED(HmckTransposeM3);

    HmckMat3 Result = Matrix;

    Result.Elements[0][1] = Matrix.Elements[1][0];
    Result.Elements[0][2] = Matrix.Elements[2][0];
    Result.Elements[1][0] = Matrix.Elements[0][1];
    Result.Elements[1][2] = Matrix.Elements[2][1];
    Result.Elements[2][1] = Matrix.Elements[1][2];
    Result.Elements[2][0] = Matrix.Elements[0][2];
    
    return Result;
}

COVERAGE(HmckAddM3, 1)
static inline HmckMat3 HmckAddM3(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckAddM3);
    
    HmckMat3 Result;
    
    Result.Elements[0][0] = Left.Elements[0][0] + Right.Elements[0][0];
    Result.Elements[0][1] = Left.Elements[0][1] + Right.Elements[0][1];
    Result.Elements[0][2] = Left.Elements[0][2] + Right.Elements[0][2];
    Result.Elements[1][0] = Left.Elements[1][0] + Right.Elements[1][0];
    Result.Elements[1][1] = Left.Elements[1][1] + Right.Elements[1][1];
    Result.Elements[1][2] = Left.Elements[1][2] + Right.Elements[1][2];
    Result.Elements[2][0] = Left.Elements[2][0] + Right.Elements[2][0];
    Result.Elements[2][1] = Left.Elements[2][1] + Right.Elements[2][1];
    Result.Elements[2][2] = Left.Elements[2][2] + Right.Elements[2][2];

    return Result;    
}

COVERAGE(HmckSubM3, 1)
static inline HmckMat3 HmckSubM3(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckSubM3);

    HmckMat3 Result;

    Result.Elements[0][0] = Left.Elements[0][0] - Right.Elements[0][0];
    Result.Elements[0][1] = Left.Elements[0][1] - Right.Elements[0][1];
    Result.Elements[0][2] = Left.Elements[0][2] - Right.Elements[0][2];
    Result.Elements[1][0] = Left.Elements[1][0] - Right.Elements[1][0];
    Result.Elements[1][1] = Left.Elements[1][1] - Right.Elements[1][1];
    Result.Elements[1][2] = Left.Elements[1][2] - Right.Elements[1][2];
    Result.Elements[2][0] = Left.Elements[2][0] - Right.Elements[2][0];
    Result.Elements[2][1] = Left.Elements[2][1] - Right.Elements[2][1];
    Result.Elements[2][2] = Left.Elements[2][2] - Right.Elements[2][2];

    return Result;
}

COVERAGE(HmckMulM3V3, 1)
static inline HmckVec3 HmckMulM3V3(HmckMat3 Matrix, HmckVec3 Vector)
{
    ASSERT_COVERED(HmckMulM3V3);
    
    HmckVec3 Result;

    Result.X = Vector.Elements[0] * Matrix.Columns[0].X;
    Result.Y = Vector.Elements[0] * Matrix.Columns[0].Y;
    Result.Z = Vector.Elements[0] * Matrix.Columns[0].Z;

    Result.X += Vector.Elements[1] * Matrix.Columns[1].X;
    Result.Y += Vector.Elements[1] * Matrix.Columns[1].Y;
    Result.Z += Vector.Elements[1] * Matrix.Columns[1].Z;

    Result.X += Vector.Elements[2] * Matrix.Columns[2].X;
    Result.Y += Vector.Elements[2] * Matrix.Columns[2].Y;
    Result.Z += Vector.Elements[2] * Matrix.Columns[2].Z;
    
    return Result;    
}

COVERAGE(HmckMulM3, 1)
static inline HmckMat3 HmckMulM3(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckMulM3);

    HmckMat3 Result;
    Result.Columns[0] = HmckMulM3V3(Left, Right.Columns[0]);
    Result.Columns[1] = HmckMulM3V3(Left, Right.Columns[1]);
    Result.Columns[2] = HmckMulM3V3(Left, Right.Columns[2]);

    return Result;    
}

COVERAGE(HmckMulM3F, 1)
static inline HmckMat3 HmckMulM3F(HmckMat3 Matrix, float Scalar)
{
    ASSERT_COVERED(HmckMulM3F);

    HmckMat3 Result;

    Result.Elements[0][0] = Matrix.Elements[0][0] * Scalar;
    Result.Elements[0][1] = Matrix.Elements[0][1] * Scalar;
    Result.Elements[0][2] = Matrix.Elements[0][2] * Scalar;
    Result.Elements[1][0] = Matrix.Elements[1][0] * Scalar;
    Result.Elements[1][1] = Matrix.Elements[1][1] * Scalar;
    Result.Elements[1][2] = Matrix.Elements[1][2] * Scalar;
    Result.Elements[2][0] = Matrix.Elements[2][0] * Scalar;
    Result.Elements[2][1] = Matrix.Elements[2][1] * Scalar;
    Result.Elements[2][2] = Matrix.Elements[2][2] * Scalar;

    return Result;            
}

COVERAGE(HmckDivM3, 1)
static inline HmckMat3 HmckDivM3F(HmckMat3 Matrix, float Scalar)
{
    ASSERT_COVERED(HmckDivM3);

    HmckMat3 Result;
    
    Result.Elements[0][0] = Matrix.Elements[0][0] / Scalar;
    Result.Elements[0][1] = Matrix.Elements[0][1] / Scalar;
    Result.Elements[0][2] = Matrix.Elements[0][2] / Scalar;
    Result.Elements[1][0] = Matrix.Elements[1][0] / Scalar;
    Result.Elements[1][1] = Matrix.Elements[1][1] / Scalar;
    Result.Elements[1][2] = Matrix.Elements[1][2] / Scalar;
    Result.Elements[2][0] = Matrix.Elements[2][0] / Scalar;
    Result.Elements[2][1] = Matrix.Elements[2][1] / Scalar;
    Result.Elements[2][2] = Matrix.Elements[2][2] / Scalar;

    return Result;                    
}

COVERAGE(HmckDeterminantM3, 1)
static inline float HmckDeterminantM3(HmckMat3 Matrix)
{
    ASSERT_COVERED(HmckDeterminantM3);

    HmckMat3 Cross;
    Cross.Columns[0] = HmckCross(Matrix.Columns[1], Matrix.Columns[2]);
    Cross.Columns[1] = HmckCross(Matrix.Columns[2], Matrix.Columns[0]);
    Cross.Columns[2] = HmckCross(Matrix.Columns[0], Matrix.Columns[1]);

    return HmckDotV3(Cross.Columns[2], Matrix.Columns[2]);
}

COVERAGE(HmckInvGeneralM3, 1)
static inline HmckMat3 HmckInvGeneralM3(HmckMat3 Matrix)
{
    ASSERT_COVERED(HmckInvGeneralM3);

    HmckMat3 Cross;
    Cross.Columns[0] = HmckCross(Matrix.Columns[1], Matrix.Columns[2]);
    Cross.Columns[1] = HmckCross(Matrix.Columns[2], Matrix.Columns[0]);
    Cross.Columns[2] = HmckCross(Matrix.Columns[0], Matrix.Columns[1]);

    float InvDeterminant = 1.0f / HmckDotV3(Cross.Columns[2], Matrix.Columns[2]);

    HmckMat3 Result;
    Result.Columns[0] = HmckMulV3F(Cross.Columns[0], InvDeterminant);
    Result.Columns[1] = HmckMulV3F(Cross.Columns[1], InvDeterminant);
    Result.Columns[2] = HmckMulV3F(Cross.Columns[2], InvDeterminant);

    return HmckTransposeM3(Result);
}

/*
 * 4x4 Matrices
 */

COVERAGE(HmckM4, 1)
static inline HmckMat4 HmckM4(void)
{
    ASSERT_COVERED(HmckM4);
    HmckMat4 Result = {0};
    return Result;
}

COVERAGE(HmckM4D, 1)
static inline HmckMat4 HmckM4D(float Diagonal)
{
    ASSERT_COVERED(HmckM4D);

    HmckMat4 Result = {0};
    Result.Elements[0][0] = Diagonal;
    Result.Elements[1][1] = Diagonal;
    Result.Elements[2][2] = Diagonal;
    Result.Elements[3][3] = Diagonal;

    return Result;
}

COVERAGE(HmckTransposeM4, 1)
static inline HmckMat4 HmckTransposeM4(HmckMat4 Matrix)
{
    ASSERT_COVERED(HmckTransposeM4);

    HmckMat4 Result = Matrix;
#ifdef HANDMADE_MATH__USE_SSE
    _MM_TRANSPOSE4_PS(Result.Columns[0].SSE, Result.Columns[1].SSE, Result.Columns[2].SSE, Result.Columns[3].SSE);
#else
    Result.Elements[0][1] = Matrix.Elements[1][0];
    Result.Elements[0][2] = Matrix.Elements[2][0];
    Result.Elements[0][3] = Matrix.Elements[3][0];
    Result.Elements[1][0] = Matrix.Elements[0][1];
    Result.Elements[1][2] = Matrix.Elements[2][1];
    Result.Elements[1][3] = Matrix.Elements[3][1];
    Result.Elements[2][1] = Matrix.Elements[1][2];
    Result.Elements[2][0] = Matrix.Elements[0][2];
    Result.Elements[2][3] = Matrix.Elements[3][2];
    Result.Elements[3][1] = Matrix.Elements[1][3];
    Result.Elements[3][2] = Matrix.Elements[2][3];
    Result.Elements[3][0] = Matrix.Elements[0][3];
#endif

    return Result;
}

COVERAGE(HmckAddM4, 1)
static inline HmckMat4 HmckAddM4(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckAddM4);

    HmckMat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.Columns[0].SSE = _mm_add_ps(Left.Columns[0].SSE, Right.Columns[0].SSE);
    Result.Columns[1].SSE = _mm_add_ps(Left.Columns[1].SSE, Right.Columns[1].SSE);
    Result.Columns[2].SSE = _mm_add_ps(Left.Columns[2].SSE, Right.Columns[2].SSE);
    Result.Columns[3].SSE = _mm_add_ps(Left.Columns[3].SSE, Right.Columns[3].SSE);
#else
    Result.Elements[0][0] = Left.Elements[0][0] + Right.Elements[0][0];
    Result.Elements[0][1] = Left.Elements[0][1] + Right.Elements[0][1];
    Result.Elements[0][2] = Left.Elements[0][2] + Right.Elements[0][2];
    Result.Elements[0][3] = Left.Elements[0][3] + Right.Elements[0][3];
    Result.Elements[1][0] = Left.Elements[1][0] + Right.Elements[1][0];
    Result.Elements[1][1] = Left.Elements[1][1] + Right.Elements[1][1];
    Result.Elements[1][2] = Left.Elements[1][2] + Right.Elements[1][2];
    Result.Elements[1][3] = Left.Elements[1][3] + Right.Elements[1][3];
    Result.Elements[2][0] = Left.Elements[2][0] + Right.Elements[2][0];
    Result.Elements[2][1] = Left.Elements[2][1] + Right.Elements[2][1];
    Result.Elements[2][2] = Left.Elements[2][2] + Right.Elements[2][2];
    Result.Elements[2][3] = Left.Elements[2][3] + Right.Elements[2][3];
    Result.Elements[3][0] = Left.Elements[3][0] + Right.Elements[3][0];
    Result.Elements[3][1] = Left.Elements[3][1] + Right.Elements[3][1];
    Result.Elements[3][2] = Left.Elements[3][2] + Right.Elements[3][2];
    Result.Elements[3][3] = Left.Elements[3][3] + Right.Elements[3][3];
#endif

    return Result;
}

COVERAGE(HmckSubM4, 1)
static inline HmckMat4 HmckSubM4(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckSubM4);

    HmckMat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.Columns[0].SSE = _mm_sub_ps(Left.Columns[0].SSE, Right.Columns[0].SSE);
    Result.Columns[1].SSE = _mm_sub_ps(Left.Columns[1].SSE, Right.Columns[1].SSE);
    Result.Columns[2].SSE = _mm_sub_ps(Left.Columns[2].SSE, Right.Columns[2].SSE);
    Result.Columns[3].SSE = _mm_sub_ps(Left.Columns[3].SSE, Right.Columns[3].SSE);
#else
    Result.Elements[0][0] = Left.Elements[0][0] - Right.Elements[0][0];
    Result.Elements[0][1] = Left.Elements[0][1] - Right.Elements[0][1];
    Result.Elements[0][2] = Left.Elements[0][2] - Right.Elements[0][2];
    Result.Elements[0][3] = Left.Elements[0][3] - Right.Elements[0][3];
    Result.Elements[1][0] = Left.Elements[1][0] - Right.Elements[1][0];
    Result.Elements[1][1] = Left.Elements[1][1] - Right.Elements[1][1];
    Result.Elements[1][2] = Left.Elements[1][2] - Right.Elements[1][2];
    Result.Elements[1][3] = Left.Elements[1][3] - Right.Elements[1][3];
    Result.Elements[2][0] = Left.Elements[2][0] - Right.Elements[2][0];
    Result.Elements[2][1] = Left.Elements[2][1] - Right.Elements[2][1];
    Result.Elements[2][2] = Left.Elements[2][2] - Right.Elements[2][2];
    Result.Elements[2][3] = Left.Elements[2][3] - Right.Elements[2][3];
    Result.Elements[3][0] = Left.Elements[3][0] - Right.Elements[3][0];
    Result.Elements[3][1] = Left.Elements[3][1] - Right.Elements[3][1];
    Result.Elements[3][2] = Left.Elements[3][2] - Right.Elements[3][2];
    Result.Elements[3][3] = Left.Elements[3][3] - Right.Elements[3][3];
#endif
 
    return Result;
}

COVERAGE(HmckMulM4, 1)
static inline HmckMat4 HmckMulM4(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckMulM4);

    HmckMat4 Result;
    Result.Columns[0] = HmckLinearCombineV4M4(Right.Columns[0], Left);
    Result.Columns[1] = HmckLinearCombineV4M4(Right.Columns[1], Left);
    Result.Columns[2] = HmckLinearCombineV4M4(Right.Columns[2], Left);
    Result.Columns[3] = HmckLinearCombineV4M4(Right.Columns[3], Left);

    return Result;
}

COVERAGE(HmckMulM4F, 1)
static inline HmckMat4 HmckMulM4F(HmckMat4 Matrix, float Scalar)
{
    ASSERT_COVERED(HmckMulM4F);

    HmckMat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEScalar = _mm_set1_ps(Scalar);
    Result.Columns[0].SSE = _mm_mul_ps(Matrix.Columns[0].SSE, SSEScalar);
    Result.Columns[1].SSE = _mm_mul_ps(Matrix.Columns[1].SSE, SSEScalar);
    Result.Columns[2].SSE = _mm_mul_ps(Matrix.Columns[2].SSE, SSEScalar);
    Result.Columns[3].SSE = _mm_mul_ps(Matrix.Columns[3].SSE, SSEScalar);
#else
    Result.Elements[0][0] = Matrix.Elements[0][0] * Scalar;
    Result.Elements[0][1] = Matrix.Elements[0][1] * Scalar;
    Result.Elements[0][2] = Matrix.Elements[0][2] * Scalar;
    Result.Elements[0][3] = Matrix.Elements[0][3] * Scalar;
    Result.Elements[1][0] = Matrix.Elements[1][0] * Scalar;
    Result.Elements[1][1] = Matrix.Elements[1][1] * Scalar;
    Result.Elements[1][2] = Matrix.Elements[1][2] * Scalar;
    Result.Elements[1][3] = Matrix.Elements[1][3] * Scalar;
    Result.Elements[2][0] = Matrix.Elements[2][0] * Scalar;
    Result.Elements[2][1] = Matrix.Elements[2][1] * Scalar;
    Result.Elements[2][2] = Matrix.Elements[2][2] * Scalar;
    Result.Elements[2][3] = Matrix.Elements[2][3] * Scalar;
    Result.Elements[3][0] = Matrix.Elements[3][0] * Scalar;
    Result.Elements[3][1] = Matrix.Elements[3][1] * Scalar;
    Result.Elements[3][2] = Matrix.Elements[3][2] * Scalar;
    Result.Elements[3][3] = Matrix.Elements[3][3] * Scalar;
#endif

    return Result;
}

COVERAGE(HmckMulM4V4, 1)
static inline HmckVec4 HmckMulM4V4(HmckMat4 Matrix, HmckVec4 Vector)
{
    ASSERT_COVERED(HmckMulM4V4);
    return HmckLinearCombineV4M4(Vector, Matrix);
}

COVERAGE(HmckDivM4F, 1)
static inline HmckMat4 HmckDivM4F(HmckMat4 Matrix, float Scalar)
{
    ASSERT_COVERED(HmckDivM4F);

    HmckMat4 Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEScalar = _mm_set1_ps(Scalar);
    Result.Columns[0].SSE = _mm_div_ps(Matrix.Columns[0].SSE, SSEScalar);
    Result.Columns[1].SSE = _mm_div_ps(Matrix.Columns[1].SSE, SSEScalar);
    Result.Columns[2].SSE = _mm_div_ps(Matrix.Columns[2].SSE, SSEScalar);
    Result.Columns[3].SSE = _mm_div_ps(Matrix.Columns[3].SSE, SSEScalar);
#else
    Result.Elements[0][0] = Matrix.Elements[0][0] / Scalar;
    Result.Elements[0][1] = Matrix.Elements[0][1] / Scalar;
    Result.Elements[0][2] = Matrix.Elements[0][2] / Scalar;
    Result.Elements[0][3] = Matrix.Elements[0][3] / Scalar;
    Result.Elements[1][0] = Matrix.Elements[1][0] / Scalar;
    Result.Elements[1][1] = Matrix.Elements[1][1] / Scalar;
    Result.Elements[1][2] = Matrix.Elements[1][2] / Scalar;
    Result.Elements[1][3] = Matrix.Elements[1][3] / Scalar;
    Result.Elements[2][0] = Matrix.Elements[2][0] / Scalar;
    Result.Elements[2][1] = Matrix.Elements[2][1] / Scalar;
    Result.Elements[2][2] = Matrix.Elements[2][2] / Scalar;
    Result.Elements[2][3] = Matrix.Elements[2][3] / Scalar;
    Result.Elements[3][0] = Matrix.Elements[3][0] / Scalar;
    Result.Elements[3][1] = Matrix.Elements[3][1] / Scalar;
    Result.Elements[3][2] = Matrix.Elements[3][2] / Scalar;
    Result.Elements[3][3] = Matrix.Elements[3][3] / Scalar;
#endif

    return Result;
}

COVERAGE(HmckDeterminantM4, 1)
static inline float HmckDeterminantM4(HmckMat4 Matrix)
{
    ASSERT_COVERED(HmckDeterminantM4);

    HmckVec3 C01 = HmckCross(Matrix.Columns[0].XYZ, Matrix.Columns[1].XYZ);
    HmckVec3 C23 = HmckCross(Matrix.Columns[2].XYZ, Matrix.Columns[3].XYZ);
    HmckVec3 B10 = HmckSubV3(HmckMulV3F(Matrix.Columns[0].XYZ, Matrix.Columns[1].W), HmckMulV3F(Matrix.Columns[1].XYZ, Matrix.Columns[0].W));
    HmckVec3 B32 = HmckSubV3(HmckMulV3F(Matrix.Columns[2].XYZ, Matrix.Columns[3].W), HmckMulV3F(Matrix.Columns[3].XYZ, Matrix.Columns[2].W));
    
    return HmckDotV3(C01, B32) + HmckDotV3(C23, B10);
}

COVERAGE(HmckInvGeneralM4, 1)
// Returns a general-purpose inverse of an HmckMat4. Note that special-purpose inverses of many transformations
// are available and will be more efficient.
static inline HmckMat4 HmckInvGeneralM4(HmckMat4 Matrix)
{
    ASSERT_COVERED(HmckInvGeneralM4);

    HmckVec3 C01 = HmckCross(Matrix.Columns[0].XYZ, Matrix.Columns[1].XYZ);
    HmckVec3 C23 = HmckCross(Matrix.Columns[2].XYZ, Matrix.Columns[3].XYZ);
    HmckVec3 B10 = HmckSubV3(HmckMulV3F(Matrix.Columns[0].XYZ, Matrix.Columns[1].W), HmckMulV3F(Matrix.Columns[1].XYZ, Matrix.Columns[0].W));
    HmckVec3 B32 = HmckSubV3(HmckMulV3F(Matrix.Columns[2].XYZ, Matrix.Columns[3].W), HmckMulV3F(Matrix.Columns[3].XYZ, Matrix.Columns[2].W));
    
    float InvDeterminant = 1.0f / (HmckDotV3(C01, B32) + HmckDotV3(C23, B10));
    C01 = HmckMulV3F(C01, InvDeterminant);
    C23 = HmckMulV3F(C23, InvDeterminant);
    B10 = HmckMulV3F(B10, InvDeterminant);
    B32 = HmckMulV3F(B32, InvDeterminant);

    HmckMat4 Result;
    Result.Columns[0] = HmckV4V(HmckAddV3(HmckCross(Matrix.Columns[1].XYZ, B32), HmckMulV3F(C23, Matrix.Columns[1].W)), -HmckDotV3(Matrix.Columns[1].XYZ, C23));
    Result.Columns[1] = HmckV4V(HmckSubV3(HmckCross(B32, Matrix.Columns[0].XYZ), HmckMulV3F(C23, Matrix.Columns[0].W)), +HmckDotV3(Matrix.Columns[0].XYZ, C23));
    Result.Columns[2] = HmckV4V(HmckAddV3(HmckCross(Matrix.Columns[3].XYZ, B10), HmckMulV3F(C01, Matrix.Columns[3].W)), -HmckDotV3(Matrix.Columns[3].XYZ, C01));
    Result.Columns[3] = HmckV4V(HmckSubV3(HmckCross(B10, Matrix.Columns[2].XYZ), HmckMulV3F(C01, Matrix.Columns[2].W)), +HmckDotV3(Matrix.Columns[2].XYZ, C01));
        
    return HmckTransposeM4(Result);
}

/*
 * Common graphics transformations
 */

COVERAGE(HmckOrthographic_RH_NO, 1)
// Produces a right-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline HmckMat4 HmckOrthographic_RH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(HmckOrthographic_RH_NO);

    HmckMat4 Result = {0};

    Result.Elements[0][0] = 2.0f / (Right - Left);
    Result.Elements[1][1] = 2.0f / (Top - Bottom);
    Result.Elements[2][2] = 2.0f / (Near - Far);
    Result.Elements[3][3] = 1.0f;

    Result.Elements[3][0] = (Left + Right) / (Left - Right);
    Result.Elements[3][1] = (Bottom + Top) / (Bottom - Top);
    Result.Elements[3][2] = (Near + Far) / (Near - Far);

    return Result;
}

COVERAGE(HmckOrthographic_RH_ZO, 1)
// Produces a right-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline HmckMat4 HmckOrthographic_RH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(HmckOrthographic_RH_ZO);

    HmckMat4 Result = {0};

    Result.Elements[0][0] = 2.0f / (Right - Left);
    Result.Elements[1][1] = 2.0f / (Top - Bottom);
    Result.Elements[2][2] = 1.0f / (Near - Far);
    Result.Elements[3][3] = 1.0f;

    Result.Elements[3][0] = (Left + Right) / (Left - Right);
    Result.Elements[3][1] = (Bottom + Top) / (Bottom - Top);
    Result.Elements[3][2] = (Near) / (Near - Far);

    return Result;
}

COVERAGE(HmckOrthographic_LH_NO, 1)
// Produces a left-handed orthographic projection matrix with Z ranging from -1 to 1 (the GL convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline HmckMat4 HmckOrthographic_LH_NO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(HmckOrthographic_LH_NO);

    HmckMat4 Result = HmckOrthographic_RH_NO(Left, Right, Bottom, Top, Near, Far);
    Result.Elements[2][2] = -Result.Elements[2][2];
    
    return Result;
}

COVERAGE(HmckOrthographic_LH_ZO, 1)
// Produces a left-handed orthographic projection matrix with Z ranging from 0 to 1 (the DirectX convention).
// Left, Right, Bottom, and Top specify the coordinates of their respective clipping planes.
// Near and Far specify the distances to the near and far clipping planes.
static inline HmckMat4 HmckOrthographic_LH_ZO(float Left, float Right, float Bottom, float Top, float Near, float Far)
{
    ASSERT_COVERED(HmckOrthographic_LH_ZO);

    HmckMat4 Result = HmckOrthographic_RH_ZO(Left, Right, Bottom, Top, Near, Far);
    Result.Elements[2][2] = -Result.Elements[2][2];
    
    return Result;
}

COVERAGE(HmckInvOrthographic, 1)
// Returns an inverse for the given orthographic projection matrix. Works for all orthographic
// projection matrices, regardless of handedness or NDC convention.
static inline HmckMat4 HmckInvOrthographic(HmckMat4 OrthoMatrix)
{
    ASSERT_COVERED(HmckInvOrthographic);

    HmckMat4 Result = {0};
    Result.Elements[0][0] = 1.0f / OrthoMatrix.Elements[0][0];
    Result.Elements[1][1] = 1.0f / OrthoMatrix.Elements[1][1];
    Result.Elements[2][2] = 1.0f / OrthoMatrix.Elements[2][2];
    Result.Elements[3][3] = 1.0f;
    
    Result.Elements[3][0] = -OrthoMatrix.Elements[3][0] * Result.Elements[0][0];
    Result.Elements[3][1] = -OrthoMatrix.Elements[3][1] * Result.Elements[1][1];
    Result.Elements[3][2] = -OrthoMatrix.Elements[3][2] * Result.Elements[2][2];

    return Result;
}

COVERAGE(HmckPerspective_RH_NO, 1)
static inline HmckMat4 HmckPerspective_RH_NO(float FOV, float AspectRatio, float Near, float Far)
{
    ASSERT_COVERED(HmckPerspective_RH_NO);

    HmckMat4 Result = {0};

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    float Cotangent = 1.0f / HmckTanF(FOV / 2.0f);
    Result.Elements[0][0] = Cotangent / AspectRatio;
    Result.Elements[1][1] = Cotangent;
    Result.Elements[2][3] = -1.0f;

    Result.Elements[2][2] = (Near + Far) / (Near - Far);
    Result.Elements[3][2] = (2.0f * Near * Far) / (Near - Far);
    
    return Result;
}

COVERAGE(HmckPerspective_RH_ZO, 1)
static inline HmckMat4 HmckPerspective_RH_ZO(float FOV, float AspectRatio, float Near, float Far)
{
    ASSERT_COVERED(HmckPerspective_RH_ZO);

    HmckMat4 Result = {0};

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    float Cotangent = 1.0f / HmckTanF(FOV / 2.0f);
    Result.Elements[0][0] = Cotangent / AspectRatio;
    Result.Elements[1][1] = Cotangent;
    Result.Elements[2][3] = -1.0f;

    Result.Elements[2][2] = (Far) / (Near - Far);
    Result.Elements[3][2] = (Near * Far) / (Near - Far);

    return Result;
}

COVERAGE(HmckPerspective_LH_NO, 1)
static inline HmckMat4 HmckPerspective_LH_NO(float FOV, float AspectRatio, float Near, float Far)
{ 
    ASSERT_COVERED(HmckPerspective_LH_NO);

    HmckMat4 Result = HmckPerspective_RH_NO(FOV, AspectRatio, Near, Far);
    Result.Elements[2][2] = -Result.Elements[2][2];
    Result.Elements[2][3] = -Result.Elements[2][3];
    
    return Result;
}

COVERAGE(HmckPerspective_LH_ZO, 1)
static inline HmckMat4 HmckPerspective_LH_ZO(float FOV, float AspectRatio, float Near, float Far)
{ 
    ASSERT_COVERED(HmckPerspective_LH_ZO);

    HmckMat4 Result = HmckPerspective_RH_ZO(FOV, AspectRatio, Near, Far);
    Result.Elements[2][2] = -Result.Elements[2][2];
    Result.Elements[2][3] = -Result.Elements[2][3];
    
    return Result;
}

COVERAGE(HmckInvPerspective_RH, 1)
static inline HmckMat4 HmckInvPerspective_RH(HmckMat4 PerspectiveMatrix)
{
    ASSERT_COVERED(HmckInvPerspective_RH);

    HmckMat4 Result = {0};
    Result.Elements[0][0] = 1.0f / PerspectiveMatrix.Elements[0][0];
    Result.Elements[1][1] = 1.0f / PerspectiveMatrix.Elements[1][1];
    Result.Elements[2][2] = 0.0f;

    Result.Elements[2][3] = 1.0f / PerspectiveMatrix.Elements[3][2];
    Result.Elements[3][3] = PerspectiveMatrix.Elements[2][2] * Result.Elements[2][3];
    Result.Elements[3][2] = PerspectiveMatrix.Elements[2][3];

    return Result;
}

COVERAGE(HmckInvPerspective_LH, 1)
static inline HmckMat4 HmckInvPerspective_LH(HmckMat4 PerspectiveMatrix)
{
    ASSERT_COVERED(HmckInvPerspective_LH);

    HmckMat4 Result = {0};
    Result.Elements[0][0] = 1.0f / PerspectiveMatrix.Elements[0][0];
    Result.Elements[1][1] = 1.0f / PerspectiveMatrix.Elements[1][1];
    Result.Elements[2][2] = 0.0f;

    Result.Elements[2][3] = 1.0f / PerspectiveMatrix.Elements[3][2];
    Result.Elements[3][3] = PerspectiveMatrix.Elements[2][2] * -Result.Elements[2][3];
    Result.Elements[3][2] = PerspectiveMatrix.Elements[2][3];

    return Result;
}

COVERAGE(HmckTranslate, 1)
static inline HmckMat4 HmckTranslate(HmckVec3 Translation)
{
    ASSERT_COVERED(HmckTranslate);

    HmckMat4 Result = HmckM4D(1.0f);
    Result.Elements[3][0] = Translation.X;
    Result.Elements[3][1] = Translation.Y;
    Result.Elements[3][2] = Translation.Z;

    return Result;
}

COVERAGE(HmckInvTranslate, 1)
static inline HmckMat4 HmckInvTranslate(HmckMat4 TranslationMatrix)
{
    ASSERT_COVERED(HmckInvTranslate);

    HmckMat4 Result = TranslationMatrix;
    Result.Elements[3][0] = -Result.Elements[3][0];
    Result.Elements[3][1] = -Result.Elements[3][1];
    Result.Elements[3][2] = -Result.Elements[3][2];

    return Result;
}

COVERAGE(HmckRotate_RH, 1)
static inline HmckMat4 HmckRotate_RH(float Angle, HmckVec3 Axis)
{
    ASSERT_COVERED(HmckRotate_RH);

    HmckMat4 Result = HmckM4D(1.0f);

    Axis = HmckNormV3(Axis);

    float SinTheta = HmckSinF(Angle);
    float CosTheta = HmckCosF(Angle);
    float CosValue = 1.0f - CosTheta;

    Result.Elements[0][0] = (Axis.X * Axis.X * CosValue) + CosTheta;
    Result.Elements[0][1] = (Axis.X * Axis.Y * CosValue) + (Axis.Z * SinTheta);
    Result.Elements[0][2] = (Axis.X * Axis.Z * CosValue) - (Axis.Y * SinTheta);

    Result.Elements[1][0] = (Axis.Y * Axis.X * CosValue) - (Axis.Z * SinTheta);
    Result.Elements[1][1] = (Axis.Y * Axis.Y * CosValue) + CosTheta;
    Result.Elements[1][2] = (Axis.Y * Axis.Z * CosValue) + (Axis.X * SinTheta);

    Result.Elements[2][0] = (Axis.Z * Axis.X * CosValue) + (Axis.Y * SinTheta);
    Result.Elements[2][1] = (Axis.Z * Axis.Y * CosValue) - (Axis.X * SinTheta);
    Result.Elements[2][2] = (Axis.Z * Axis.Z * CosValue) + CosTheta;

    return Result;
}

COVERAGE(HmckRotate_LH, 1)
static inline HmckMat4 HmckRotate_LH(float Angle, HmckVec3 Axis)
{
    ASSERT_COVERED(HmckRotate_LH);
    /* NOTE(lcf): Matrix will be inverse/transpose of RH. */
    return HmckRotate_RH(-Angle, Axis);
}

COVERAGE(HmckInvRotate, 1)
static inline HmckMat4 HmckInvRotate(HmckMat4 RotationMatrix)
{
    ASSERT_COVERED(HmckInvRotate);
    return HmckTransposeM4(RotationMatrix);
}

COVERAGE(HmckScale, 1)
static inline HmckMat4 HmckScale(HmckVec3 Scale)
{
    ASSERT_COVERED(HmckScale);

    HmckMat4 Result = HmckM4D(1.0f);
    Result.Elements[0][0] = Scale.X;
    Result.Elements[1][1] = Scale.Y;
    Result.Elements[2][2] = Scale.Z;

    return Result;
}

COVERAGE(HmckInvScale, 1)
static inline HmckMat4 HmckInvScale(HmckMat4 ScaleMatrix)
{
    ASSERT_COVERED(HmckInvScale);

    HmckMat4 Result = ScaleMatrix;
    Result.Elements[0][0] = 1.0f / Result.Elements[0][0];
    Result.Elements[1][1] = 1.0f / Result.Elements[1][1];
    Result.Elements[2][2] = 1.0f / Result.Elements[2][2];

    return Result;
}

static inline HmckMat4 _HmckLookAt(HmckVec3 F,  HmckVec3 S, HmckVec3 U,  HmckVec3 Eye)
{
    HmckMat4 Result;

    Result.Elements[0][0] = S.X;
    Result.Elements[0][1] = U.X;
    Result.Elements[0][2] = -F.X;
    Result.Elements[0][3] = 0.0f;

    Result.Elements[1][0] = S.Y;
    Result.Elements[1][1] = U.Y;
    Result.Elements[1][2] = -F.Y;
    Result.Elements[1][3] = 0.0f;

    Result.Elements[2][0] = S.Z;
    Result.Elements[2][1] = U.Z;
    Result.Elements[2][2] = -F.Z;
    Result.Elements[2][3] = 0.0f;

    Result.Elements[3][0] = -HmckDotV3(S, Eye);
    Result.Elements[3][1] = -HmckDotV3(U, Eye);
    Result.Elements[3][2] = HmckDotV3(F, Eye);
    Result.Elements[3][3] = 1.0f;

    return Result;
}

COVERAGE(HmckLookAt_RH, 1)
static inline HmckMat4 HmckLookAt_RH(HmckVec3 Eye, HmckVec3 Center, HmckVec3 Up)
{
    ASSERT_COVERED(HmckLookAt_RH);

    HmckVec3 F = HmckNormV3(HmckSubV3(Center, Eye));
    HmckVec3 S = HmckNormV3(HmckCross(F, Up));
    HmckVec3 U = HmckCross(S, F);

    return _HmckLookAt(F, S, U, Eye);
}

COVERAGE(HmckLookAt_LH, 1)
static inline HmckMat4 HmckLookAt_LH(HmckVec3 Eye, HmckVec3 Center, HmckVec3 Up)
{
    ASSERT_COVERED(HmckLookAt_LH);

    HmckVec3 F = HmckNormV3(HmckSubV3(Eye, Center));
    HmckVec3 S = HmckNormV3(HmckCross(F, Up));
    HmckVec3 U = HmckCross(S, F);

    return _HmckLookAt(F, S, U, Eye);
}

COVERAGE(HmckInvLookAt, 1)
static inline HmckMat4 HmckInvLookAt(HmckMat4 Matrix)
{
    ASSERT_COVERED(HmckInvLookAt);
    HmckMat4 Result;

    HmckMat3 Rotation = {0};
    Rotation.Columns[0] = Matrix.Columns[0].XYZ;
    Rotation.Columns[1] = Matrix.Columns[1].XYZ;
    Rotation.Columns[2] = Matrix.Columns[2].XYZ;
    Rotation = HmckTransposeM3(Rotation);

    Result.Columns[0] = HmckV4V(Rotation.Columns[0], 0.0f);
    Result.Columns[1] = HmckV4V(Rotation.Columns[1], 0.0f);
    Result.Columns[2] = HmckV4V(Rotation.Columns[2], 0.0f);
    Result.Columns[3] = HmckMulV4F(Matrix.Columns[3], -1.0f);
    Result.Elements[3][0] = -1.0f * Matrix.Elements[3][0] /
        (Rotation.Elements[0][0] + Rotation.Elements[0][1] + Rotation.Elements[0][2]);
    Result.Elements[3][1] = -1.0f * Matrix.Elements[3][1] /
        (Rotation.Elements[1][0] + Rotation.Elements[1][1] + Rotation.Elements[1][2]);
    Result.Elements[3][2] = -1.0f * Matrix.Elements[3][2] /
        (Rotation.Elements[2][0] + Rotation.Elements[2][1] + Rotation.Elements[2][2]);
    Result.Elements[3][3] = 1.0f;

    return Result;
}

/*
 * Quaternion operations
 */

COVERAGE(HmckQ, 1)
static inline HmckQuat HmckQ(float X, float Y, float Z, float W)
{
    ASSERT_COVERED(HmckQ);

    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_setr_ps(X, Y, Z, W);
#else
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
#endif

    return Result;
}

COVERAGE(HmckQV4, 1)
static inline HmckQuat HmckQV4(HmckVec4 Vector)
{
    ASSERT_COVERED(HmckQV4);

    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = Vector.SSE;
#else
    Result.X = Vector.X;
    Result.Y = Vector.Y;
    Result.Z = Vector.Z;
    Result.W = Vector.W;
#endif

    return Result;
}

COVERAGE(HmckAddQ, 1)
static inline HmckQuat HmckAddQ(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckAddQ);

    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_add_ps(Left.SSE, Right.SSE);
#else

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;
#endif

    return Result;
}

COVERAGE(HmckSubQ, 1)
static inline HmckQuat HmckSubQ(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckSubQ);

    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_sub_ps(Left.SSE, Right.SSE);
#else
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    Result.W = Left.W - Right.W;
#endif

    return Result;
}

COVERAGE(HmckMulQ, 1)
static inline HmckQuat HmckMulQ(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckMulQ);

    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.f, -0.f, 0.f, -0.f));
    __m128 SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 SSEResultThree = _mm_mul_ps(SSEResultTwo, SSEResultOne);

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(1, 1, 1, 1)) , _mm_setr_ps(0.f, 0.f, -0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(1, 0, 3, 2));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.f, 0.f, 0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_shuffle_ps(Left.SSE, Left.SSE, _MM_SHUFFLE(3, 3, 3, 3));
    SSEResultTwo = _mm_shuffle_ps(Right.SSE, Right.SSE, _MM_SHUFFLE(3, 2, 1, 0));
    Result.SSE = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));
#else
    Result.X =  Right.Elements[3] * +Left.Elements[0];
    Result.Y =  Right.Elements[2] * -Left.Elements[0];
    Result.Z =  Right.Elements[1] * +Left.Elements[0];
    Result.W =  Right.Elements[0] * -Left.Elements[0];

    Result.X += Right.Elements[2] * +Left.Elements[1];
    Result.Y += Right.Elements[3] * +Left.Elements[1];
    Result.Z += Right.Elements[0] * -Left.Elements[1];
    Result.W += Right.Elements[1] * -Left.Elements[1];
    
    Result.X += Right.Elements[1] * -Left.Elements[2];
    Result.Y += Right.Elements[0] * +Left.Elements[2];
    Result.Z += Right.Elements[3] * +Left.Elements[2];
    Result.W += Right.Elements[2] * -Left.Elements[2];

    Result.X += Right.Elements[0] * +Left.Elements[3];
    Result.Y += Right.Elements[1] * +Left.Elements[3];
    Result.Z += Right.Elements[2] * +Left.Elements[3];
    Result.W += Right.Elements[3] * +Left.Elements[3];
#endif

    return Result;
}

COVERAGE(HmckMulQF, 1)
static inline HmckQuat HmckMulQF(HmckQuat Left, float Multiplicative)
{
    ASSERT_COVERED(HmckMulQF);

    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Multiplicative);
    Result.SSE = _mm_mul_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X * Multiplicative;
    Result.Y = Left.Y * Multiplicative;
    Result.Z = Left.Z * Multiplicative;
    Result.W = Left.W * Multiplicative;
#endif

    return Result;
}

COVERAGE(HmckDivQF, 1)
static inline HmckQuat HmckDivQF(HmckQuat Left, float Divnd)
{
    ASSERT_COVERED(HmckDivQF);

    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Divnd);
    Result.SSE = _mm_div_ps(Left.SSE, Scalar);
#else
    Result.X = Left.X / Divnd;
    Result.Y = Left.Y / Divnd;
    Result.Z = Left.Z / Divnd;
    Result.W = Left.W / Divnd;
#endif

    return Result;
}

COVERAGE(HmckDotQ, 1)
static inline float HmckDotQ(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckDotQ);

    float Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_mul_ps(Left.SSE, Right.SSE);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&Result, SSEResultOne);
#else
    Result = ((Left.X * Right.X) + (Left.Z * Right.Z)) + ((Left.Y * Right.Y) + (Left.W * Right.W));
#endif

    return Result;
}

COVERAGE(HmckInvQ, 1)
static inline HmckQuat HmckInvQ(HmckQuat Left)
{
    ASSERT_COVERED(HmckInvQ);
    
    HmckQuat Result;
    Result.X = -Left.X;
    Result.Y = -Left.Y;
    Result.Z = -Left.Z;
    Result.W = Left.W;

    return HmckDivQF(Result, (HmckDotQ(Left, Left)));
}

COVERAGE(HmckNormQ, 1)
static inline HmckQuat HmckNormQ(HmckQuat Quat)
{
    ASSERT_COVERED(HmckNormQ);

    /* NOTE(lcf): Take advantage of SSE implementation in HmckNormV4 */
    HmckVec4 Vec = {Quat.X, Quat.Y, Quat.Z, Quat.W};
    Vec = HmckNormV4(Vec);
    HmckQuat Result = {Vec.X, Vec.Y, Vec.Z, Vec.W};

    return Result;
}

static inline HmckQuat _HmckMixQ(HmckQuat Left, float MixLeft, HmckQuat Right, float MixRight) {
    HmckQuat Result;

#ifdef HANDMADE_MATH__USE_SSE
    __m128 ScalarLeft = _mm_set1_ps(MixLeft);
    __m128 ScalarRight = _mm_set1_ps(MixRight);
    __m128 SSEResultOne = _mm_mul_ps(Left.SSE, ScalarLeft);
    __m128 SSEResultTwo = _mm_mul_ps(Right.SSE, ScalarRight);
    Result.SSE = _mm_add_ps(SSEResultOne, SSEResultTwo);
#else
    Result.X = Left.X*MixLeft + Right.X*MixRight;
    Result.Y = Left.Y*MixLeft + Right.Y*MixRight;
    Result.Z = Left.Z*MixLeft + Right.Z*MixRight;
    Result.W = Left.W*MixLeft + Right.W*MixRight;
#endif

    return Result;
}

COVERAGE(HmckNLerp, 1)
static inline HmckQuat HmckNLerp(HmckQuat Left, float Time, HmckQuat Right)
{
    ASSERT_COVERED(HmckNLerp);

    HmckQuat Result = _HmckMixQ(Left, 1.0f-Time, Right, Time);
    Result = HmckNormQ(Result);

    return Result;
}

COVERAGE(HmckSLerp, 1)
static inline HmckQuat HmckSLerp(HmckQuat Left, float Time, HmckQuat Right)
{
    ASSERT_COVERED(HmckSLerp);

    HmckQuat Result;

    float Cos_Theta = HmckDotQ(Left, Right);

    if (Cos_Theta < 0.0f) { /* NOTE(lcf): Take shortest path on Hyper-sphere */
        Cos_Theta = -Cos_Theta;
        Right = HmckQ(-Right.X, -Right.Y, -Right.Z, -Right.W);
    }
    
    /* NOTE(lcf): Use Normalized Linear interpolation when vectors are roughly not L.I. */
    if (Cos_Theta > 0.9995f) {
        Result = HmckNLerp(Left, Time, Right);
    } else {
        float Angle = HmckACosF(Cos_Theta);
        float MixLeft = HmckSinF((1.0f - Time) * Angle);
        float MixRight = HmckSinF(Time * Angle);

        Result = _HmckMixQ(Left, MixLeft, Right, MixRight);
        Result = HmckNormQ(Result);
    }
    
    return Result;
}

COVERAGE(HmckQToM4, 1)
static inline HmckMat4 HmckQToM4(HmckQuat Left)
{
    ASSERT_COVERED(HmckQToM4);

    HmckMat4 Result;

    HmckQuat NormalizedQ = HmckNormQ(Left);

    float XX, YY, ZZ,
          XY, XZ, YZ,
          WX, WY, WZ;

    XX = NormalizedQ.X * NormalizedQ.X;
    YY = NormalizedQ.Y * NormalizedQ.Y;
    ZZ = NormalizedQ.Z * NormalizedQ.Z;
    XY = NormalizedQ.X * NormalizedQ.Y;
    XZ = NormalizedQ.X * NormalizedQ.Z;
    YZ = NormalizedQ.Y * NormalizedQ.Z;
    WX = NormalizedQ.W * NormalizedQ.X;
    WY = NormalizedQ.W * NormalizedQ.Y;
    WZ = NormalizedQ.W * NormalizedQ.Z;

    Result.Elements[0][0] = 1.0f - 2.0f * (YY + ZZ);
    Result.Elements[0][1] = 2.0f * (XY + WZ);
    Result.Elements[0][2] = 2.0f * (XZ - WY);
    Result.Elements[0][3] = 0.0f;

    Result.Elements[1][0] = 2.0f * (XY - WZ);
    Result.Elements[1][1] = 1.0f - 2.0f * (XX + ZZ);
    Result.Elements[1][2] = 2.0f * (YZ + WX);
    Result.Elements[1][3] = 0.0f;

    Result.Elements[2][0] = 2.0f * (XZ + WY);
    Result.Elements[2][1] = 2.0f * (YZ - WX);
    Result.Elements[2][2] = 1.0f - 2.0f * (XX + YY);
    Result.Elements[2][3] = 0.0f;

    Result.Elements[3][0] = 0.0f;
    Result.Elements[3][1] = 0.0f;
    Result.Elements[3][2] = 0.0f;
    Result.Elements[3][3] = 1.0f;

    return Result;
}

// This method taken from Mike Day at Insomniac Games.
// https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
//
// Note that as mentioned at the top of the paper, the paper assumes the matrix
// would be *post*-multiplied to a vector to rotate it, meaning the matrix is
// the transpose of what we're dealing with. But, because our matrices are
// stored in column-major order, the indices *appear* to match the paper.
//
// For example, m12 in the paper is row 1, column 2. We need to transpose it to
// row 2, column 1. But, because the column comes first when referencing
// elements, it looks like M.Elements[1][2].
//
// Don't be confused! Or if you must be confused, at least trust this
// comment. :)
COVERAGE(HmckM4ToQ_RH, 4)
static inline HmckQuat HmckM4ToQ_RH(HmckMat4 M)
{
    float T;
    HmckQuat Q;

    if (M.Elements[2][2] < 0.0f) {
        if (M.Elements[0][0] > M.Elements[1][1]) {
            ASSERT_COVERED(HmckM4ToQ_RH);

            T = 1 + M.Elements[0][0] - M.Elements[1][1] - M.Elements[2][2];
            Q = HmckQ(
                T,
                M.Elements[0][1] + M.Elements[1][0],
                M.Elements[2][0] + M.Elements[0][2],
                M.Elements[1][2] - M.Elements[2][1]
            );
        } else {
            ASSERT_COVERED(HmckM4ToQ_RH);

            T = 1 - M.Elements[0][0] + M.Elements[1][1] - M.Elements[2][2];
            Q = HmckQ(
                M.Elements[0][1] + M.Elements[1][0],
                T,
                M.Elements[1][2] + M.Elements[2][1],
                M.Elements[2][0] - M.Elements[0][2]
            );
        }
    } else {
        if (M.Elements[0][0] < -M.Elements[1][1]) {
            ASSERT_COVERED(HmckM4ToQ_RH);

            T = 1 - M.Elements[0][0] - M.Elements[1][1] + M.Elements[2][2];
            Q = HmckQ(
                M.Elements[2][0] + M.Elements[0][2],
                M.Elements[1][2] + M.Elements[2][1],
                T,
                M.Elements[0][1] - M.Elements[1][0]
            );
        } else {
            ASSERT_COVERED(HmckM4ToQ_RH);

            T = 1 + M.Elements[0][0] + M.Elements[1][1] + M.Elements[2][2];
            Q = HmckQ(
                M.Elements[1][2] - M.Elements[2][1],
                M.Elements[2][0] - M.Elements[0][2],
                M.Elements[0][1] - M.Elements[1][0],
                T
            );
        }
    }

    Q = HmckMulQF(Q, 0.5f / HmckSqrtF(T));

    return Q;
}

COVERAGE(HmckM4ToQ_LH, 4)
static inline HmckQuat HmckM4ToQ_LH(HmckMat4 M)
{
    float T;
    HmckQuat Q;

    if (M.Elements[2][2] < 0.0f) {
        if (M.Elements[0][0] > M.Elements[1][1]) {
            ASSERT_COVERED(HmckM4ToQ_LH);

            T = 1 + M.Elements[0][0] - M.Elements[1][1] - M.Elements[2][2];
            Q = HmckQ(
                T,
                M.Elements[0][1] + M.Elements[1][0],
                M.Elements[2][0] + M.Elements[0][2],
                M.Elements[2][1] - M.Elements[1][2]
            );
        } else {
            ASSERT_COVERED(HmckM4ToQ_LH);

            T = 1 - M.Elements[0][0] + M.Elements[1][1] - M.Elements[2][2];
            Q = HmckQ(
                M.Elements[0][1] + M.Elements[1][0],
                T,
                M.Elements[1][2] + M.Elements[2][1],
                M.Elements[0][2] - M.Elements[2][0]
            );
        }
    } else {
        if (M.Elements[0][0] < -M.Elements[1][1]) {
            ASSERT_COVERED(HmckM4ToQ_LH);

            T = 1 - M.Elements[0][0] - M.Elements[1][1] + M.Elements[2][2];
            Q = HmckQ(
                M.Elements[2][0] + M.Elements[0][2],
                M.Elements[1][2] + M.Elements[2][1],
                T,
                M.Elements[1][0] - M.Elements[0][1]
            );
        } else {
            ASSERT_COVERED(HmckM4ToQ_LH);

            T = 1 + M.Elements[0][0] + M.Elements[1][1] + M.Elements[2][2];
            Q = HmckQ(
                M.Elements[2][1] - M.Elements[1][2],
                M.Elements[0][2] - M.Elements[2][0],
                M.Elements[1][0] - M.Elements[0][2],
                T
            );
        }
    }

    Q = HmckMulQF(Q, 0.5f / HmckSqrtF(T));

    return Q;
}


COVERAGE(HmckQFromAxisAngle_RH, 1)
static inline HmckQuat HmckQFromAxisAngle_RH(HmckVec3 Axis, float AngleOfRotation)
{
    ASSERT_COVERED(HmckQFromAxisAngle_RH);

    HmckQuat Result;

    HmckVec3 AxisNormalized = HmckNormV3(Axis);
    float SineOfRotation = HmckSinF(AngleOfRotation / 2.0f);

    Result.XYZ = HmckMulV3F(AxisNormalized, SineOfRotation);
    Result.W = HmckCosF(AngleOfRotation / 2.0f);

    return Result;
}

COVERAGE(HmckQFromAxisAngle_LH, 1)
static inline HmckQuat HmckQFromAxisAngle_LH(HmckVec3 Axis, float AngleOfRotation)
{
    ASSERT_COVERED(HmckQFromAxisAngle_LH);

    return HmckQFromAxisAngle_RH(Axis, -AngleOfRotation);
}


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

COVERAGE(HmckLenV2CPP, 1)
static inline float HmckLen(HmckVec2 A)
{
    ASSERT_COVERED(HmckLenV2CPP);
    return HmckLenV2(A);
}

COVERAGE(HmckLenV3CPP, 1)
static inline float HmckLen(HmckVec3 A)
{
    ASSERT_COVERED(HmckLenV3CPP);
    return HmckLenV3(A);
}

COVERAGE(HmckLenV4CPP, 1)
static inline float HmckLen(HmckVec4 A)
{
    ASSERT_COVERED(HmckLenV4CPP);
    return HmckLenV4(A);
}

COVERAGE(HmckLenSqrV2CPP, 1)
static inline float HmckLenSqr(HmckVec2 A)
{
    ASSERT_COVERED(HmckLenSqrV2CPP);
    return HmckLenSqrV2(A);
}

COVERAGE(HmckLenSqrV3CPP, 1)
static inline float HmckLenSqr(HmckVec3 A)
{
    ASSERT_COVERED(HmckLenSqrV3CPP);
    return HmckLenSqrV3(A);
}

COVERAGE(HmckLenSqrV4CPP, 1)
static inline float HmckLenSqr(HmckVec4 A)
{
    ASSERT_COVERED(HmckLenSqrV4CPP);
    return HmckLenSqrV4(A);
}

COVERAGE(HmckNormV2CPP, 1)
static inline HmckVec2 HmckNorm(HmckVec2 A)
{
    ASSERT_COVERED(HmckNormV2CPP);
    return HmckNormV2(A);
}

COVERAGE(HmckNormV3CPP, 1)
static inline HmckVec3 HmckNorm(HmckVec3 A)
{
    ASSERT_COVERED(HmckNormV3CPP);
    return HmckNormV3(A);
}

COVERAGE(HmckNormV4CPP, 1)
static inline HmckVec4 HmckNorm(HmckVec4 A)
{
    ASSERT_COVERED(HmckNormV4CPP);
    return HmckNormV4(A);
}

COVERAGE(HmckNormQCPP, 1)
static inline HmckQuat HmckNorm(HmckQuat A)
{
    ASSERT_COVERED(HmckNormQCPP);
    return HmckNormQ(A);
}

COVERAGE(HmckDotV2CPP, 1)
static inline float HmckDot(HmckVec2 Left, HmckVec2 VecTwo)
{
    ASSERT_COVERED(HmckDotV2CPP);
    return HmckDotV2(Left, VecTwo);
}

COVERAGE(HmckDotV3CPP, 1)
static inline float HmckDot(HmckVec3 Left, HmckVec3 VecTwo)
{
    ASSERT_COVERED(HmckDotV3CPP);
    return HmckDotV3(Left, VecTwo);
}

COVERAGE(HmckDotV4CPP, 1)
static inline float HmckDot(HmckVec4 Left, HmckVec4 VecTwo)
{
    ASSERT_COVERED(HmckDotV4CPP);
    return HmckDotV4(Left, VecTwo);
}
 
COVERAGE(HmckLerpV2CPP, 1)
static inline HmckVec2 HmckLerp(HmckVec2 Left, float Time, HmckVec2 Right)
{
    ASSERT_COVERED(HmckLerpV2CPP);
    return HmckLerpV2(Left, Time, Right);
}

COVERAGE(HmckLerpV3CPP, 1)
static inline HmckVec3 HmckLerp(HmckVec3 Left, float Time, HmckVec3 Right)
{
    ASSERT_COVERED(HmckLerpV3CPP);
    return HmckLerpV3(Left, Time, Right);
}

COVERAGE(HmckLerpV4CPP, 1)
static inline HmckVec4 HmckLerp(HmckVec4 Left, float Time, HmckVec4 Right)
{
    ASSERT_COVERED(HmckLerpV4CPP);
    return HmckLerpV4(Left, Time, Right);
}

COVERAGE(HmckTransposeM2CPP, 1)
static inline HmckMat2 HmckTranspose(HmckMat2 Matrix)
{
    ASSERT_COVERED(HmckTransposeM2CPP);
    return HmckTransposeM2(Matrix);
}

COVERAGE(HmckTransposeM3CPP, 1)
static inline HmckMat3 HmckTranspose(HmckMat3 Matrix)
{
    ASSERT_COVERED(HmckTransposeM3CPP);
    return HmckTransposeM3(Matrix);
}

COVERAGE(HmckTransposeM4CPP, 1)
static inline HmckMat4 HmckTranspose(HmckMat4 Matrix)
{
    ASSERT_COVERED(HmckTransposeM4CPP);
    return HmckTransposeM4(Matrix);
}

COVERAGE(HmckDeterminantM2CPP, 1)
static inline float HmckDeterminant(HmckMat2 Matrix)
{
    ASSERT_COVERED(HmckDeterminantM2CPP);
    return HmckDeterminantM2(Matrix);
}

COVERAGE(HmckDeterminantM3CPP, 1)
static inline float HmckDeterminant(HmckMat3 Matrix)
{
    ASSERT_COVERED(HmckDeterminantM3CPP);
    return HmckDeterminantM3(Matrix);
}

COVERAGE(HmckDeterminantM4CPP, 1)
static inline float HmckDeterminant(HmckMat4 Matrix)
{
    ASSERT_COVERED(HmckDeterminantM4CPP);
    return HmckDeterminantM4(Matrix);
}

COVERAGE(HmckInvGeneralM2CPP, 1)
static inline HmckMat2 HmckInvGeneral(HmckMat2 Matrix)
{
    ASSERT_COVERED(HmckInvGeneralM2CPP);
    return HmckInvGeneralM2(Matrix);
}

COVERAGE(HmckInvGeneralM3CPP, 1)
static inline HmckMat3 HmckInvGeneral(HmckMat3 Matrix)
{
    ASSERT_COVERED(HmckInvGeneralM3CPP);
    return HmckInvGeneralM3(Matrix);
}

COVERAGE(HmckInvGeneralM4CPP, 1)
static inline HmckMat4 HmckInvGeneral(HmckMat4 Matrix)
{
    ASSERT_COVERED(HmckInvGeneralM4CPP);
    return HmckInvGeneralM4(Matrix);
}

COVERAGE(HmckDotQCPP, 1)
static inline float HmckDot(HmckQuat QuatOne, HmckQuat QuatTwo)
{
    ASSERT_COVERED(HmckDotQCPP);
    return HmckDotQ(QuatOne, QuatTwo);
}

COVERAGE(HmckAddV2CPP, 1)
static inline HmckVec2 HmckAdd(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckAddV2CPP);
    return HmckAddV2(Left, Right);
}

COVERAGE(HmckAddV3CPP, 1)
static inline HmckVec3 HmckAdd(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckAddV3CPP);
    return HmckAddV3(Left, Right);
}

COVERAGE(HmckAddV4CPP, 1)
static inline HmckVec4 HmckAdd(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckAddV4CPP);
    return HmckAddV4(Left, Right);
}

COVERAGE(HmckAddM2CPP, 1)
static inline HmckMat2 HmckAdd(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckAddM2CPP);
    return HmckAddM2(Left, Right);
}

COVERAGE(HmckAddM3CPP, 1)
static inline HmckMat3 HmckAdd(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckAddM3CPP);
    return HmckAddM3(Left, Right);
}

COVERAGE(HmckAddM4CPP, 1)
static inline HmckMat4 HmckAdd(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckAddM4CPP);
    return HmckAddM4(Left, Right);
}

COVERAGE(HmckAddQCPP, 1)
static inline HmckQuat HmckAdd(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckAddQCPP);
    return HmckAddQ(Left, Right);
}

COVERAGE(HmckSubV2CPP, 1)
static inline HmckVec2 HmckSub(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckSubV2CPP);
    return HmckSubV2(Left, Right);
}

COVERAGE(HmckSubV3CPP, 1)
static inline HmckVec3 HmckSub(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckSubV3CPP);
    return HmckSubV3(Left, Right);
}

COVERAGE(HmckSubV4CPP, 1)
static inline HmckVec4 HmckSub(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckSubV4CPP);
    return HmckSubV4(Left, Right);
}

COVERAGE(HmckSubM2CPP, 1)
static inline HmckMat2 HmckSub(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckSubM2CPP);
    return HmckSubM2(Left, Right);
}

COVERAGE(HmckSubM3CPP, 1)
static inline HmckMat3 HmckSub(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckSubM3CPP);
    return HmckSubM3(Left, Right);
}

COVERAGE(HmckSubM4CPP, 1)
static inline HmckMat4 HmckSub(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckSubM4CPP);
    return HmckSubM4(Left, Right);
}

COVERAGE(HmckSubQCPP, 1)
static inline HmckQuat HmckSub(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckSubQCPP);
    return HmckSubQ(Left, Right);
}

COVERAGE(HmckMulV2CPP, 1)
static inline HmckVec2 HmckMul(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckMulV2CPP);
    return HmckMulV2(Left, Right);
}

COVERAGE(HmckMulV2FCPP, 1)
static inline HmckVec2 HmckMul(HmckVec2 Left, float Right)
{
    ASSERT_COVERED(HmckMulV2FCPP);
    return HmckMulV2F(Left, Right);
}

COVERAGE(HmckMulV3CPP, 1)
static inline HmckVec3 HmckMul(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckMulV3CPP);
    return HmckMulV3(Left, Right);
}

COVERAGE(HmckMulV3FCPP, 1)
static inline HmckVec3 HmckMul(HmckVec3 Left, float Right)
{
    ASSERT_COVERED(HmckMulV3FCPP);
    return HmckMulV3F(Left, Right);
}

COVERAGE(HmckMulV4CPP, 1)
static inline HmckVec4 HmckMul(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckMulV4CPP);
    return HmckMulV4(Left, Right);
}

COVERAGE(HmckMulV4FCPP, 1)
static inline HmckVec4 HmckMul(HmckVec4 Left, float Right)
{
    ASSERT_COVERED(HmckMulV4FCPP);
    return HmckMulV4F(Left, Right);
}

COVERAGE(HmckMulM2CPP, 1)
static inline HmckMat2 HmckMul(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckMulM2CPP);
    return HmckMulM2(Left, Right);
}

COVERAGE(HmckMulM3CPP, 1)
static inline HmckMat3 HmckMul(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckMulM3CPP);
    return HmckMulM3(Left, Right);
}

COVERAGE(HmckMulM4CPP, 1)
static inline HmckMat4 HmckMul(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckMulM4CPP);
    return HmckMulM4(Left, Right);
}

COVERAGE(HmckMulM2FCPP, 1)
static inline HmckMat2 HmckMul(HmckMat2 Left, float Right)
{
    ASSERT_COVERED(HmckMulM2FCPP);
    return HmckMulM2F(Left, Right);
}

COVERAGE(HmckMulM3FCPP, 1)
static inline HmckMat3 HmckMul(HmckMat3 Left, float Right)
{
    ASSERT_COVERED(HmckMulM3FCPP);
    return HmckMulM3F(Left, Right);
}

COVERAGE(HmckMulM4FCPP, 1)
static inline HmckMat4 HmckMul(HmckMat4 Left, float Right)
{
    ASSERT_COVERED(HmckMulM4FCPP);
    return HmckMulM4F(Left, Right);
}

COVERAGE(HmckMulM2V2CPP, 1)
static inline HmckVec2 HmckMul(HmckMat2 Matrix, HmckVec2 Vector)
{
    ASSERT_COVERED(HmckMulM2V2CPP);
    return HmckMulM2V2(Matrix, Vector);
}

COVERAGE(HmckMulM3V3CPP, 1)
static inline HmckVec3 HmckMul(HmckMat3 Matrix, HmckVec3 Vector)
{
    ASSERT_COVERED(HmckMulM3V3CPP);
    return HmckMulM3V3(Matrix, Vector);
}

COVERAGE(HmckMulM4V4CPP, 1)
static inline HmckVec4 HmckMul(HmckMat4 Matrix, HmckVec4 Vector)
{
    ASSERT_COVERED(HmckMulM4V4CPP);
    return HmckMulM4V4(Matrix, Vector);
}

COVERAGE(HmckMulQCPP, 1)
static inline HmckQuat HmckMul(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckMulQCPP);
    return HmckMulQ(Left, Right);
}

COVERAGE(HmckMulQFCPP, 1)
static inline HmckQuat HmckMul(HmckQuat Left, float Right)
{
    ASSERT_COVERED(HmckMulQFCPP);
    return HmckMulQF(Left, Right);
}

COVERAGE(HmckDivV2CPP, 1)
static inline HmckVec2 HmckDiv(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckDivV2CPP);
    return HmckDivV2(Left, Right);
}

COVERAGE(HmckDivV2FCPP, 1)
static inline HmckVec2 HmckDiv(HmckVec2 Left, float Right)
{
    ASSERT_COVERED(HmckDivV2FCPP);
    return HmckDivV2F(Left, Right);
}

COVERAGE(HmckDivV3CPP, 1)
static inline HmckVec3 HmckDiv(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckDivV3CPP);
    return HmckDivV3(Left, Right);
}

COVERAGE(HmckDivV3FCPP, 1)
static inline HmckVec3 HmckDiv(HmckVec3 Left, float Right)
{
    ASSERT_COVERED(HmckDivV3FCPP);
    return HmckDivV3F(Left, Right);
}

COVERAGE(HmckDivV4CPP, 1)
static inline HmckVec4 HmckDiv(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckDivV4CPP);
    return HmckDivV4(Left, Right);
}

COVERAGE(HmckDivV4FCPP, 1)
static inline HmckVec4 HmckDiv(HmckVec4 Left, float Right)
{
    ASSERT_COVERED(HmckDivV4FCPP);
    return HmckDivV4F(Left, Right);
}

COVERAGE(HmckDivM2FCPP, 1)
static inline HmckMat2 HmckDiv(HmckMat2 Left, float Right)
{
    ASSERT_COVERED(HmckDivM2FCPP);
    return HmckDivM2F(Left, Right);
}

COVERAGE(HmckDivM3FCPP, 1)
static inline HmckMat3 HmckDiv(HmckMat3 Left, float Right)
{
    ASSERT_COVERED(HmckDivM3FCPP);
    return HmckDivM3F(Left, Right);
}

COVERAGE(HmckDivM4FCPP, 1)
static inline HmckMat4 HmckDiv(HmckMat4 Left, float Right)
{
    ASSERT_COVERED(HmckDivM4FCPP);
    return HmckDivM4F(Left, Right);
}

COVERAGE(HmckDivQFCPP, 1)
static inline HmckQuat HmckDiv(HmckQuat Left, float Right)
{
    ASSERT_COVERED(HmckDivQFCPP);
    return HmckDivQF(Left, Right);
}

COVERAGE(HmckEqV2CPP, 1)
static inline HmckBool HmckEq(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckEqV2CPP);
    return HmckEqV2(Left, Right);
}

COVERAGE(HmckEqV3CPP, 1)
static inline HmckBool HmckEq(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckEqV3CPP);
    return HmckEqV3(Left, Right);
}

COVERAGE(HmckEqV4CPP, 1)
static inline HmckBool HmckEq(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckEqV4CPP);
    return HmckEqV4(Left, Right);
}

COVERAGE(HmckAddV2Op, 1)
static inline HmckVec2 operator+(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckAddV2Op);
    return HmckAddV2(Left, Right);
}

COVERAGE(HmckAddV3Op, 1)
static inline HmckVec3 operator+(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckAddV3Op);
    return HmckAddV3(Left, Right);
}

COVERAGE(HmckAddV4Op, 1)
static inline HmckVec4 operator+(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckAddV4Op);
    return HmckAddV4(Left, Right);
}

COVERAGE(HmckAddM2Op, 1)
static inline HmckMat2 operator+(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckAddM2Op);
    return HmckAddM2(Left, Right);
}

COVERAGE(HmckAddM3Op, 1)
static inline HmckMat3 operator+(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckAddM3Op);
    return HmckAddM3(Left, Right);
}

COVERAGE(HmckAddM4Op, 1)
static inline HmckMat4 operator+(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckAddM4Op);
    return HmckAddM4(Left, Right);
}

COVERAGE(HmckAddQOp, 1)
static inline HmckQuat operator+(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckAddQOp);
    return HmckAddQ(Left, Right);
}

COVERAGE(HmckSubV2Op, 1)
static inline HmckVec2 operator-(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckSubV2Op);
    return HmckSubV2(Left, Right);
}

COVERAGE(HmckSubV3Op, 1)
static inline HmckVec3 operator-(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckSubV3Op);
    return HmckSubV3(Left, Right);
}

COVERAGE(HmckSubV4Op, 1)
static inline HmckVec4 operator-(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckSubV4Op);
    return HmckSubV4(Left, Right);
}

COVERAGE(HmckSubM2Op, 1)
static inline HmckMat2 operator-(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckSubM2Op);
    return HmckSubM2(Left, Right);
}

COVERAGE(HmckSubM3Op, 1)
static inline HmckMat3 operator-(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckSubM3Op);
    return HmckSubM3(Left, Right);
}

COVERAGE(HmckSubM4Op, 1)
static inline HmckMat4 operator-(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckSubM4Op);
    return HmckSubM4(Left, Right);
}

COVERAGE(HmckSubQOp, 1)
static inline HmckQuat operator-(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckSubQOp);
    return HmckSubQ(Left, Right);
}

COVERAGE(HmckMulV2Op, 1)
static inline HmckVec2 operator*(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckMulV2Op);
    return HmckMulV2(Left, Right);
}

COVERAGE(HmckMulV3Op, 1)
static inline HmckVec3 operator*(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckMulV3Op);
    return HmckMulV3(Left, Right);
}

COVERAGE(HmckMulV4Op, 1)
static inline HmckVec4 operator*(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckMulV4Op);
    return HmckMulV4(Left, Right);
}

COVERAGE(HmckMulM2Op, 1)
static inline HmckMat2 operator*(HmckMat2 Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckMulM2Op);
    return HmckMulM2(Left, Right);
}

COVERAGE(HmckMulM3Op, 1)
static inline HmckMat3 operator*(HmckMat3 Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckMulM3Op);
    return HmckMulM3(Left, Right);
}

COVERAGE(HmckMulM4Op, 1)
static inline HmckMat4 operator*(HmckMat4 Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckMulM4Op);
    return HmckMulM4(Left, Right);
}

COVERAGE(HmckMulQOp, 1)
static inline HmckQuat operator*(HmckQuat Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckMulQOp);
    return HmckMulQ(Left, Right);
}

COVERAGE(HmckMulV2FOp, 1)
static inline HmckVec2 operator*(HmckVec2 Left, float Right)
{
    ASSERT_COVERED(HmckMulV2FOp);
    return HmckMulV2F(Left, Right);
}

COVERAGE(HmckMulV3FOp, 1)
static inline HmckVec3 operator*(HmckVec3 Left, float Right)
{
    ASSERT_COVERED(HmckMulV3FOp);
    return HmckMulV3F(Left, Right);
}

COVERAGE(HmckMulV4FOp, 1)
static inline HmckVec4 operator*(HmckVec4 Left, float Right)
{
    ASSERT_COVERED(HmckMulV4FOp);
    return HmckMulV4F(Left, Right);
}

COVERAGE(HmckMulM2FOp, 1)
static inline HmckMat2 operator*(HmckMat2 Left, float Right)
{
    ASSERT_COVERED(HmckMulM2FOp);
    return HmckMulM2F(Left, Right);
}

COVERAGE(HmckMulM3FOp, 1)
static inline HmckMat3 operator*(HmckMat3 Left, float Right)
{
    ASSERT_COVERED(HmckMulM3FOp);
    return HmckMulM3F(Left, Right);
}

COVERAGE(HmckMulM4FOp, 1)
static inline HmckMat4 operator*(HmckMat4 Left, float Right)
{
    ASSERT_COVERED(HmckMulM4FOp);
    return HmckMulM4F(Left, Right);
}

COVERAGE(HmckMulQFOp, 1)
static inline HmckQuat operator*(HmckQuat Left, float Right)
{
    ASSERT_COVERED(HmckMulQFOp);
    return HmckMulQF(Left, Right);
}

COVERAGE(HmckMulV2FOpLeft, 1)
static inline HmckVec2 operator*(float Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckMulV2FOpLeft);
    return HmckMulV2F(Right, Left);
}

COVERAGE(HmckMulV3FOpLeft, 1)
static inline HmckVec3 operator*(float Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckMulV3FOpLeft);
    return HmckMulV3F(Right, Left);
}

COVERAGE(HmckMulV4FOpLeft, 1)
static inline HmckVec4 operator*(float Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckMulV4FOpLeft);
    return HmckMulV4F(Right, Left);
}

COVERAGE(HmckMulM2FOpLeft, 1)
static inline HmckMat2 operator*(float Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckMulM2FOpLeft);
    return HmckMulM2F(Right, Left);
}

COVERAGE(HmckMulM3FOpLeft, 1)
static inline HmckMat3 operator*(float Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckMulM3FOpLeft);
    return HmckMulM3F(Right, Left);
}

COVERAGE(HmckMulM4FOpLeft, 1)
static inline HmckMat4 operator*(float Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckMulM4FOpLeft);
    return HmckMulM4F(Right, Left);
}

COVERAGE(HmckMulQFOpLeft, 1)
static inline HmckQuat operator*(float Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckMulQFOpLeft);
    return HmckMulQF(Right, Left);
}

COVERAGE(HmckMulM2V2Op, 1)
static inline HmckVec2 operator*(HmckMat2 Matrix, HmckVec2 Vector)
{
    ASSERT_COVERED(HmckMulM2V2Op);
    return HmckMulM2V2(Matrix, Vector);
}

COVERAGE(HmckMulM3V3Op, 1)
static inline HmckVec3 operator*(HmckMat3 Matrix, HmckVec3 Vector)
{
    ASSERT_COVERED(HmckMulM3V3Op);
    return HmckMulM3V3(Matrix, Vector);
}

COVERAGE(HmckMulM4V4Op, 1)
static inline HmckVec4 operator*(HmckMat4 Matrix, HmckVec4 Vector)
{
    ASSERT_COVERED(HmckMulM4V4Op);
    return HmckMulM4V4(Matrix, Vector);
}

COVERAGE(HmckDivV2Op, 1)
static inline HmckVec2 operator/(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckDivV2Op);
    return HmckDivV2(Left, Right);
}

COVERAGE(HmckDivV3Op, 1)
static inline HmckVec3 operator/(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckDivV3Op);
    return HmckDivV3(Left, Right);
}

COVERAGE(HmckDivV4Op, 1)
static inline HmckVec4 operator/(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckDivV4Op);
    return HmckDivV4(Left, Right);
}

COVERAGE(HmckDivV2FOp, 1)
static inline HmckVec2 operator/(HmckVec2 Left, float Right)
{
    ASSERT_COVERED(HmckDivV2FOp);
    return HmckDivV2F(Left, Right);
}

COVERAGE(HmckDivV3FOp, 1)
static inline HmckVec3 operator/(HmckVec3 Left, float Right)
{
    ASSERT_COVERED(HmckDivV3FOp);
    return HmckDivV3F(Left, Right);
}

COVERAGE(HmckDivV4FOp, 1)
static inline HmckVec4 operator/(HmckVec4 Left, float Right)
{
    ASSERT_COVERED(HmckDivV4FOp);
    return HmckDivV4F(Left, Right);
}

COVERAGE(HmckDivM4FOp, 1)
static inline HmckMat4 operator/(HmckMat4 Left, float Right)
{
    ASSERT_COVERED(HmckDivM4FOp);
    return HmckDivM4F(Left, Right);
}

COVERAGE(HmckDivM3FOp, 1)
static inline HmckMat3 operator/(HmckMat3 Left, float Right)
{
    ASSERT_COVERED(HmckDivM3FOp);
    return HmckDivM3F(Left, Right);
}

COVERAGE(HmckDivM2FOp, 1)
static inline HmckMat2 operator/(HmckMat2 Left, float Right)
{
    ASSERT_COVERED(HmckDivM2FOp);
    return HmckDivM2F(Left, Right);
}

COVERAGE(HmckDivQFOp, 1)
static inline HmckQuat operator/(HmckQuat Left, float Right)
{
    ASSERT_COVERED(HmckDivQFOp);
    return HmckDivQF(Left, Right);
}

COVERAGE(HmckAddV2Assign, 1)
static inline HmckVec2 &operator+=(HmckVec2 &Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckAddV2Assign);
    return Left = Left + Right;
}

COVERAGE(HmckAddV3Assign, 1)
static inline HmckVec3 &operator+=(HmckVec3 &Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckAddV3Assign);
    return Left = Left + Right;
}

COVERAGE(HmckAddV4Assign, 1)
static inline HmckVec4 &operator+=(HmckVec4 &Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckAddV4Assign);
    return Left = Left + Right;
}

COVERAGE(HmckAddM2Assign, 1)
static inline HmckMat2 &operator+=(HmckMat2 &Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckAddM2Assign);
    return Left = Left + Right;
}

COVERAGE(HmckAddM3Assign, 1)
static inline HmckMat3 &operator+=(HmckMat3 &Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckAddM3Assign);
    return Left = Left + Right;
}

COVERAGE(HmckAddM4Assign, 1)
static inline HmckMat4 &operator+=(HmckMat4 &Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckAddM4Assign);
    return Left = Left + Right;
}

COVERAGE(HmckAddQAssign, 1)
static inline HmckQuat &operator+=(HmckQuat &Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckAddQAssign);
    return Left = Left + Right;
}

COVERAGE(HmckSubV2Assign, 1)
static inline HmckVec2 &operator-=(HmckVec2 &Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckSubV2Assign);
    return Left = Left - Right;
}

COVERAGE(HmckSubV3Assign, 1)
static inline HmckVec3 &operator-=(HmckVec3 &Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckSubV3Assign);
    return Left = Left - Right;
}

COVERAGE(HmckSubV4Assign, 1)
static inline HmckVec4 &operator-=(HmckVec4 &Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckSubV4Assign);
    return Left = Left - Right;
}

COVERAGE(HmckSubM2Assign, 1)
static inline HmckMat2 &operator-=(HmckMat2 &Left, HmckMat2 Right)
{
    ASSERT_COVERED(HmckSubM2Assign);
    return Left = Left - Right;
}

COVERAGE(HmckSubM3Assign, 1)
static inline HmckMat3 &operator-=(HmckMat3 &Left, HmckMat3 Right)
{
    ASSERT_COVERED(HmckSubM3Assign);
    return Left = Left - Right;
}

COVERAGE(HmckSubM4Assign, 1)
static inline HmckMat4 &operator-=(HmckMat4 &Left, HmckMat4 Right)
{
    ASSERT_COVERED(HmckSubM4Assign);
    return Left = Left - Right;
}

COVERAGE(HmckSubQAssign, 1)
static inline HmckQuat &operator-=(HmckQuat &Left, HmckQuat Right)
{
    ASSERT_COVERED(HmckSubQAssign);
    return Left = Left - Right;
}

COVERAGE(HmckMulV2Assign, 1)
static inline HmckVec2 &operator*=(HmckVec2 &Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckMulV2Assign);
    return Left = Left * Right;
}

COVERAGE(HmckMulV3Assign, 1)
static inline HmckVec3 &operator*=(HmckVec3 &Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckMulV3Assign);
    return Left = Left * Right;
}

COVERAGE(HmckMulV4Assign, 1)
static inline HmckVec4 &operator*=(HmckVec4 &Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckMulV4Assign);
    return Left = Left * Right;
}

COVERAGE(HmckMulV2FAssign, 1)
static inline HmckVec2 &operator*=(HmckVec2 &Left, float Right)
{
    ASSERT_COVERED(HmckMulV2FAssign);
    return Left = Left * Right;
}

COVERAGE(HmckMulV3FAssign, 1)
static inline HmckVec3 &operator*=(HmckVec3 &Left, float Right)
{
    ASSERT_COVERED(HmckMulV3FAssign);
    return Left = Left * Right;
}

COVERAGE(HmckMulV4FAssign, 1)
static inline HmckVec4 &operator*=(HmckVec4 &Left, float Right)
{
    ASSERT_COVERED(HmckMulV4FAssign);
    return Left = Left * Right;
}

COVERAGE(HmckMulM2FAssign, 1)
static inline HmckMat2 &operator*=(HmckMat2 &Left, float Right)
{
    ASSERT_COVERED(HmckMulM2FAssign);
    return Left = Left * Right;
}

COVERAGE(HmckMulM3FAssign, 1)
static inline HmckMat3 &operator*=(HmckMat3 &Left, float Right)
{
    ASSERT_COVERED(HmckMulM3FAssign);
    return Left = Left * Right;
}

COVERAGE(HmckMulM4FAssign, 1)
static inline HmckMat4 &operator*=(HmckMat4 &Left, float Right)
{
    ASSERT_COVERED(HmckMulM4FAssign);
    return Left = Left * Right;
}

COVERAGE(HmckMulQFAssign, 1)
static inline HmckQuat &operator*=(HmckQuat &Left, float Right)
{
    ASSERT_COVERED(HmckMulQFAssign);
    return Left = Left * Right;
}

COVERAGE(HmckDivV2Assign, 1)
static inline HmckVec2 &operator/=(HmckVec2 &Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckDivV2Assign);
    return Left = Left / Right;
}

COVERAGE(HmckDivV3Assign, 1)
static inline HmckVec3 &operator/=(HmckVec3 &Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckDivV3Assign);
    return Left = Left / Right;
}

COVERAGE(HmckDivV4Assign, 1)
static inline HmckVec4 &operator/=(HmckVec4 &Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckDivV4Assign);
    return Left = Left / Right;
}

COVERAGE(HmckDivV2FAssign, 1)
static inline HmckVec2 &operator/=(HmckVec2 &Left, float Right)
{
    ASSERT_COVERED(HmckDivV2FAssign);
    return Left = Left / Right;
}

COVERAGE(HmckDivV3FAssign, 1)
static inline HmckVec3 &operator/=(HmckVec3 &Left, float Right)
{
    ASSERT_COVERED(HmckDivV3FAssign);
    return Left = Left / Right;
}

COVERAGE(HmckDivV4FAssign, 1)
static inline HmckVec4 &operator/=(HmckVec4 &Left, float Right)
{
    ASSERT_COVERED(HmckDivV4FAssign);
    return Left = Left / Right;
}

COVERAGE(HmckDivM4FAssign, 1)
static inline HmckMat4 &operator/=(HmckMat4 &Left, float Right)
{
    ASSERT_COVERED(HmckDivM4FAssign);
    return Left = Left / Right;
}

COVERAGE(HmckDivQFAssign, 1)
static inline HmckQuat &operator/=(HmckQuat &Left, float Right)
{
    ASSERT_COVERED(HmckDivQFAssign);
    return Left = Left / Right;
}

COVERAGE(HmckEqV2Op, 1)
static inline HmckBool operator==(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckEqV2Op);
    return HmckEqV2(Left, Right);
}

COVERAGE(HmckEqV3Op, 1)
static inline HmckBool operator==(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckEqV3Op);
    return HmckEqV3(Left, Right);
}

COVERAGE(HmckEqV4Op, 1)
static inline HmckBool operator==(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckEqV4Op);
    return HmckEqV4(Left, Right);
}

COVERAGE(HmckEqV2OpNot, 1)
static inline HmckBool operator!=(HmckVec2 Left, HmckVec2 Right)
{
    ASSERT_COVERED(HmckEqV2OpNot);
    return !HmckEqV2(Left, Right);
}

COVERAGE(HmckEqV3OpNot, 1)
static inline HmckBool operator!=(HmckVec3 Left, HmckVec3 Right)
{
    ASSERT_COVERED(HmckEqV3OpNot);
    return !HmckEqV3(Left, Right);
}

COVERAGE(HmckEqV4OpNot, 1)
static inline HmckBool operator!=(HmckVec4 Left, HmckVec4 Right)
{
    ASSERT_COVERED(HmckEqV4OpNot);
    return !HmckEqV4(Left, Right);
}

COVERAGE(HmckUnaryMinusV2, 1)
static inline HmckVec2 operator-(HmckVec2 In)
{
    ASSERT_COVERED(HmckUnaryMinusV2);

    HmckVec2 Result;
    Result.X = -In.X;
    Result.Y = -In.Y;

    return Result;
}

COVERAGE(HmckUnaryMinusV3, 1)
static inline HmckVec3 operator-(HmckVec3 In)
{
    ASSERT_COVERED(HmckUnaryMinusV3);

    HmckVec3 Result;
    Result.X = -In.X;
    Result.Y = -In.Y;
    Result.Z = -In.Z;

    return Result;
}

COVERAGE(HmckUnaryMinusV4, 1)
static inline HmckVec4 operator-(HmckVec4 In)
{
    ASSERT_COVERED(HmckUnaryMinusV4);

    HmckVec4 Result;
#if HANDMADE_MATH__USE_SSE
    Result.SSE = _mm_xor_ps(In.SSE, _mm_set1_ps(-0.0f));
#else
    Result.X = -In.X;
    Result.Y = -In.Y;
    Result.Z = -In.Z;
    Result.W = -In.W;
#endif

    return Result;
}

#endif /* __cplusplus*/

#ifdef HANDMADE_MATH__USE_C11_GENERICS
#define HmckAdd(A, B) _Generic((A), \
        HmckVec2: HmckAddV2, \
        HmckVec3: HmckAddV3, \
        HmckVec4: HmckAddV4, \
        HmckMat2: HmckAddM2, \
        HmckMat3: HmckAddM3, \
        HmckMat4: HmckAddM4, \
        HmckQuat: HmckAddQ \
)(A, B)

#define HmckSub(A, B) _Generic((A), \
        HmckVec2: HmckSubV2, \
        HmckVec3: HmckSubV3, \
        HmckVec4: HmckSubV4, \
        HmckMat2: HmckSubM2, \
        HmckMat3: HmckSubM3, \
        HmckMat4: HmckSubM4, \
        HmckQuat: HmckSubQ \
)(A, B)

#define HmckMul(A, B) _Generic((B), \
     float: _Generic((A), \
        HmckVec2: HmckMulV2F, \
        HmckVec3: HmckMulV3F, \
        HmckVec4: HmckMulV4F, \
        HmckMat2: HmckMulM2F, \
        HmckMat3: HmckMulM3F, \
        HmckMat4: HmckMulM4F, \
        HmckQuat: HmckMulQF \
     ), \
     HmckMat2: HmckMulM2, \
     HmckMat3: HmckMulM3, \
     HmckMat4: HmckMulM4, \
     HmckQuat: HmckMulQ, \
     default: _Generic((A), \
        HmckVec2: HmckMulV2, \
        HmckVec3: HmckMulV3, \
        HmckVec4: HmckMulV4, \
        HmckMat2: HmckMulM2V2, \
        HmckMat3: HmckMulM3V3, \
        HmckMat4: HmckMulM4V4 \
    ) \
)(A, B)

#define HmckDiv(A, B) _Generic((B), \
     float: _Generic((A), \
        HmckMat2: HmckDivM2F, \
        HmckMat3: HmckDivM3F, \
        HmckMat4: HmckDivM4F, \
        HmckVec2: HmckDivV2F, \
        HmckVec3: HmckDivV3F, \
        HmckVec4: HmckDivV4F, \
        HmckQuat: HmckDivQF  \
     ), \
     HmckMat2: HmckDivM2, \
     HmckMat3: HmckDivM3, \
     HmckMat4: HmckDivM4, \
     HmckQuat: HmckDivQ, \
     default: _Generic((A), \
        HmckVec2: HmckDivV2, \
        HmckVec3: HmckDivV3, \
        HmckVec4: HmckDivV4  \
    ) \
)(A, B)

#define HmckLen(A) _Generic((A), \
        HmckVec2: HmckLenV2, \
        HmckVec3: HmckLenV3, \
        HmckVec4: HmckLenV4  \
)(A)

#define HmckLenSqr(A) _Generic((A), \
        HmckVec2: HmckLenSqrV2, \
        HmckVec3: HmckLenSqrV3, \
        HmckVec4: HmckLenSqrV4  \
)(A)

#define HmckNorm(A) _Generic((A), \
        HmckVec2: HmckNormV2, \
        HmckVec3: HmckNormV3, \
        HmckVec4: HmckNormV4  \
)(A)

#define HmckDot(A, B) _Generic((A), \
        HmckVec2: HmckDotV2, \
        HmckVec3: HmckDotV3, \
        HmckVec4: HmckDotV4  \
)(A, B)

#define HmckLerp(A, T, B) _Generic((A), \
        float: HmckLerp, \
        HmckVec2: HmckLerpV2, \
        HmckVec3: HmckLerpV3, \
        HmckVec4: HmckLerpV4 \
)(A, T, B)

#define HmckEq(A, B) _Generic((A), \
        HmckVec2: HmckEqV2, \
        HmckVec3: HmckEqV3, \
        HmckVec4: HmckEqV4  \
)(A, B)

#define HmckTranspose(M) _Generic((M), \
        HmckMat2: HmckTransposeM2, \
        HmckMat3: HmckTransposeM3, \
        HmckMat4: HmckTransposeM4  \
)(M)

#define HmckDeterminant(M) _Generic((M), \
        HmckMat2: HmckDeterminantM2, \
        HmckMat3: HmckDeterminantM3, \
        HmckMat4: HmckDeterminantM4  \
)(M)

#define HmckInvGeneral(M) _Generic((M), \
        HmckMat2: HmckInvGeneralM2, \
        HmckMat3: HmckInvGeneralM3, \
        HmckMat4: HmckInvGeneralM4  \
)(M)

#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* HANDMADE_MATH_H */



