//-----------
// constants
//-----------

// trigonometry
#define PI      3.1415926535897932384626433832795028841971693993751f
#define PI_HALF 1.5707963267948966192313216916397514420985846996876f
#define DEG2RAD 0.0174532925199432957692369076848861271344287188854f

// logarithms
#define LOG_10  2.3025850929940456840179914546843642076011014886288f
#define LOG_PI  1.1447298858494001741434273513530587116472948129153f
#define LOG_2PI 1.8378770664093454835606594728112352797227949472756f


//------------
// image data
//------------

// true center of the image
#define IMAGE_CENTER ((float2)(0.5f*(IMAGE_WIDTH + 1), 0.5f*(IMAGE_HEIGHT + 1)))


//----------
// matrices
//----------

// 2x2 matrix type
typedef float4 mat22;

// 2x2 matrix vector multiplication
inline static float2 mv22(mat22 m, float2 v)
{
    return (float2)(dot(m.lo, v), dot(m.hi, v));
}
