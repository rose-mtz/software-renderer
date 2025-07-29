#include "Vec.h"

struct Mat3x3f
{
    float mat[3][3]; // row-major

    Mat3x3f();
    Mat3x3f(const Vec3f& c0, const Vec3f& c1, const Vec3f& c2);
    Mat3x3f(
        float e00, float e01, float e02, 
        float e10, float e11, float e12, 
        float e20, float e21, float e22
    );

    Vec3f   operator * (const Vec3f& v) const;
    Mat3x3f operator * (float s)        const;

    Mat3x3f inv()          const;
    float   determinant()  const;
    Mat3x3f transposed()   const;
    void    print()        const;
    Vec3f   get_col(int i) const;

    static Mat3x3f zero_matrix();
    static Mat3x3f identity_matrix();

    private:
    Mat3x3f cofactor() const;
    Mat3x3f adj()      const;
};

struct Mat4x4f
{
    float mat[4][4]; // row-major

    Mat4x4f();
    Mat4x4f(const Vec4f& c0, const Vec4f& c1, const Vec4f& c2, const Vec4f& c3);
    Mat4x4f(
        float e00, float e01, float e02, float e03,
        float e10, float e11, float e12, float e13,
        float e20, float e21, float e22, float e23,
        float e30, float e31, float e32, float e33
    );

    Vec4f   operator * (const Vec4f& right)   const;
    Mat4x4f operator * (const Mat4x4f& right) const;

    Vec4f   get_col(int i) const;
    Mat3x3f truncated()    const;
    void    print()        const;
    Mat4x4f transpose()    const;

    static Mat4x4f affine_matrix(const Mat3x3f& basis, const Vec3f& translation);
    static Mat4x4f look_at(const Vec3f& pos, const Vec3f& at, const Vec3f& up);
    static Mat4x4f rotation_x(float theta);
    static Mat4x4f rotation_y(float theta);
    static Mat4x4f rotation_z(float theta);
    static Mat4x4f translation(const Vec3f& v);
    static Mat4x4f scale(const Vec3f& scale);
};

// Utility functions
float radians(float degree);