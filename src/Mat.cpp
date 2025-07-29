#include "Mat.h"
#include <cassert>

// Zero-matrix
Mat3x3f::Mat3x3f() : Mat3x3f(Vec3f(0.0f), Vec3f(0.0f), Vec3f(0.0f)) {}

// Sets columns of matrix
Mat3x3f::Mat3x3f(const Vec3f& c0, const Vec3f& c1, const Vec3f& c2)
{
    mat[0][0] = c0.x;
    mat[1][0] = c0.y;
    mat[2][0] = c0.z;

    mat[0][1] = c1.x;
    mat[1][1] = c1.y;
    mat[2][1] = c1.z;

    mat[0][2] = c2.x;
    mat[1][2] = c2.y;
    mat[2][2] = c2.z;
}

// Sets elements of matrix wit row-major ordering (first index of element is row index)
Mat3x3f::Mat3x3f
(
    float e00, float e01, float e02, 
    float e10, float e11, float e12, 
    float e20, float e21, float e22
)
{
    mat[0][0] = e00;
    mat[0][1] = e01;
    mat[0][2] = e02;

    mat[1][0] = e10;
    mat[1][1] = e11;
    mat[1][2] = e12;

    mat[2][0] = e20;
    mat[2][1] = e21;
    mat[2][2] = e22;
}

Vec3f Mat3x3f::operator*(const Vec3f& v) const
{
    return Vec3f(
        v.x * mat[0][0] + v.y * mat[0][1] + v.z * mat[0][2],
        v.x * mat[1][0] + v.y * mat[1][1] + v.z * mat[1][2],
        v.x * mat[2][0] + v.y * mat[2][1] + v.z * mat[2][2]
    );
}

Mat3x3f Mat3x3f::operator*(float s) const
{
    return Mat3x3f(
        mat[0][0] * s, mat[0][1] * s, mat[0][2] * s,
        mat[1][0] * s, mat[1][1] * s, mat[1][2] * s,
        mat[2][0] * s, mat[2][1] * s, mat[2][2] * s
    );
}

float Mat3x3f::determinant() const
{
    return (
        mat[0][0] * (mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1]) -
        mat[0][1] * (mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0]) + 
        mat[0][2] * (mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0])
    );
}

Mat3x3f Mat3x3f::inv() const
{
    assert(determinant() != 0.0f);

    return adj() * (1.0f / determinant());
}

Mat3x3f Mat3x3f::adj() const
{
    return cofactor().transposed();
}

Mat3x3f Mat3x3f::cofactor() const
{
    return Mat3x3f(
        (mat[1][1]*mat[2][2] - mat[1][2]*mat[2][1]), (mat[1][2]*mat[2][0] - mat[1][0]*mat[2][2]), (mat[1][0]*mat[1][2] - mat[1][1]*mat[2][0]),
        (mat[0][2]*mat[2][1] - mat[0][1]*mat[2][2]), (mat[0][0]*mat[2][2] - mat[0][2]*mat[2][0]), (mat[0][2]*mat[1][0] - mat[0][0]*mat[1][2]),
        (mat[0][1]*mat[2][1] - mat[0][2]*mat[1][1]), (mat[0][1]*mat[2][0] - mat[0][0]*mat[2][1]), (mat[0][0]*mat[1][1] - mat[0][1]*mat[1][0])
    );
}

Mat3x3f Mat3x3f::transposed() const
{
    Mat3x3f trans;

    trans.mat[0][0] = mat[0][0];
    trans.mat[0][1] = mat[1][0];
    trans.mat[0][2] = mat[2][0];

    trans.mat[1][0] = mat[0][1];
    trans.mat[1][1] = mat[1][1];
    trans.mat[1][2] = mat[2][1];

    trans.mat[2][0] = mat[0][2];
    trans.mat[2][1] = mat[1][2];
    trans.mat[2][2] = mat[2][2];

    return trans;
}

void Mat3x3f::print() const
{
    std::cout << "(" << mat[0][0] << ", " << mat[0][1] << ", " << mat[0][2] << ")\n";
    std::cout << "(" << mat[1][0] << ", " << mat[1][1] << ", " << mat[1][2] << ")\n";
    std::cout << "(" << mat[2][0] << ", " << mat[2][1] << ", " << mat[2][2] << ")\n";
}

Vec3f Mat3x3f::get_col(int i) const
{
    assert(i > -1 && i < 3);

    return Vec3f(
        mat[0][i],
        mat[1][i],
        mat[2][i]
    );
}

Mat3x3f Mat3x3f::zero_matrix()
{
    return Mat3x3f(
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f
    );
}

Mat3x3f Mat3x3f::identity_matrix()
{
    return Mat3x3f(
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
}

Mat4x4f::Mat4x4f() : Mat4x4f(Vec4f(0.0f), Vec4f(0.0f), Vec4f(0.0f), Vec4f(0.0f)) {}

Mat4x4f::Mat4x4f(const Vec4f& c0, const Vec4f& c1, const Vec4f& c2, const Vec4f& c3)
{
    mat[0][0] = c0.x;
    mat[1][0] = c0.y;
    mat[2][0] = c0.z;
    mat[3][0] = c0.w;

    mat[0][1] = c1.x;
    mat[1][1] = c1.y;
    mat[2][1] = c1.z;
    mat[3][1] = c1.w;

    mat[0][2] = c2.x;
    mat[1][2] = c2.y;
    mat[2][2] = c2.z;
    mat[3][2] = c2.w;

    mat[0][3] = c3.x;
    mat[1][3] = c3.y;
    mat[2][3] = c3.z;
    mat[3][3] = c3.w;
}

Mat4x4f::Mat4x4f
(
    float e00, float e01, float e02, float e03,
    float e10, float e11, float e12, float e13,
    float e20, float e21, float e22, float e23,
    float e30, float e31, float e32, float e33
)
{
    mat[0][0] = e00;
    mat[0][1] = e01;
    mat[0][2] = e02;
    mat[0][3] = e03;

    mat[1][0] = e10;
    mat[1][1] = e11;
    mat[1][2] = e12;
    mat[1][3] = e13;

    mat[2][0] = e20;
    mat[2][1] = e21;
    mat[2][2] = e22;
    mat[2][3] = e23;

    mat[3][0] = e30;
    mat[3][1] = e31;
    mat[3][2] = e32;
    mat[3][3] = e33;
}

Vec4f Mat4x4f::operator*(const Vec4f& right) const
{
    return Vec4f(
        (mat[0][0] * right.x) + (mat[0][1] * right.y) + (mat[0][2] * right.z) + (mat[0][3] * right.w),
        (mat[1][0] * right.x) + (mat[1][1] * right.y) + (mat[1][2] * right.z) + (mat[1][3] * right.w),
        (mat[2][0] * right.x) + (mat[2][1] * right.y) + (mat[2][2] * right.z) + (mat[2][3] * right.w),
        (mat[3][0] * right.x) + (mat[3][1] * right.y) + (mat[3][2] * right.z) + (mat[3][3] * right.w)
    );
}

Mat4x4f Mat4x4f::operator*(const Mat4x4f& right) const
{
    return Mat4x4f(
        (*this) * right.get_col(0), 
        (*this) * right.get_col(1), 
        (*this) * right.get_col(2), 
        (*this) * right.get_col(3)
    );
}

Vec4f Mat4x4f::get_col(int i) const
{
    assert(i > -1 && i < 4);

    return Vec4f(
        mat[0][i],
        mat[1][i],
        mat[2][i],
        mat[3][i]
    );
}

// Returns upper left 3x3
Mat3x3f Mat4x4f::truncated() const
{
    return Mat3x3f(
        get_col(0).xyz(),
        get_col(1).xyz(),
        get_col(2).xyz()
    );
}

void Mat4x4f::print() const
{
    std::cout << "(" << mat[0][0] << ", " << mat[0][1] << ", " << mat[0][2] << ", " << mat[0][3] << ")\n";
    std::cout << "(" << mat[1][0] << ", " << mat[1][1] << ", " << mat[1][2] << ", " << mat[1][3] << ")\n";
    std::cout << "(" << mat[2][0] << ", " << mat[2][1] << ", " << mat[2][2] << ", " << mat[2][3] << ")\n";
    std::cout << "(" << mat[3][0] << ", " << mat[3][1] << ", " << mat[3][2] << ", " << mat[3][3] << ")\n";
}

Mat4x4f Mat4x4f::transpose() const
{
    Mat4x4f transposed_mat;

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            transposed_mat.mat[i][j] = mat[j][i];
        }
    }

    return transposed_mat;
}

Mat4x4f Mat4x4f::affine_matrix(const Mat3x3f& basis, const Vec3f& translation)
{
    return Mat4x4f(
        Vec4f(basis.get_col(0), 0.0f),
        Vec4f(basis.get_col(1), 0.0f),
        Vec4f(basis.get_col(2), 0.0f),
        Vec4f(translation,      1.0f)
    );
}

Mat4x4f Mat4x4f::look_at(const Vec3f& pos, const Vec3f& at, const Vec3f& up)
{
    Vec3f z = (pos - at).normalize(); // 'backwards' of camera
    Vec3f x = (up ^ z).normalize();   // right of camera
    Vec3f y = (z ^ x).normalize();    // up of camera

    Mat4x4f rotation_transposed = Mat4x4f(
        Vec4f(x, 0.0f),
        Vec4f(y, 0.0f),
        Vec4f(z, 0.0f),
        Vec4f(0.0f, 0.0f, 0.0f, 1.0f)
    ).transpose();

    Mat4x4f translation_inv = translation(pos * -1.0f);

    return rotation_transposed * translation_inv;
}

Mat4x4f Mat4x4f::rotation_x(float theta)
{

    return Mat4x4f(
        1.0f,       0.0f,        0.0f, 0.0f,
        0.0f, cos(theta), -sin(theta), 0.0f,
        0.0f, sin(theta),  cos(theta), 0.0f,
        0.0f,       0.0f,        0.0f, 1.0f
    );
}

Mat4x4f Mat4x4f::rotation_y(float theta)
{
    return Mat4x4f(
         cos(theta), 0.0f, sin(theta), 0.0f,
               0.0f, 1.0f,       0.0f, 0.0f,
        -sin(theta), 0.0f, cos(theta), 0.0f,
               0.0f, 0.0f,       0.0f, 1.0f
    );
}

Mat4x4f Mat4x4f::rotation_z(float theta)
{
    return Mat4x4f(
        cos(theta), -sin(theta), 0.0f, 0.0f,
        sin(theta),  cos(theta), 0.0f, 0.0f,
              0.0f,        0.0f, 1.0f, 0.0f,
              0.0f,        0.0f, 0.0f, 1.0f
    );
}

Mat4x4f Mat4x4f::translation(const Vec3f& v)
{
    return Mat4x4f(
        1.0f, 0.0f, 0.0f, v.x,
        0.0f, 1.0f, 0.0f, v.y,
        0.0f, 0.0f, 1.0f, v.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

Mat4x4f Mat4x4f::scale(const Vec3f& scale)
{
    return Mat4x4f(
        scale.x,    0.0f,    0.0f, 0.0f,
           0.0f, scale.y,    0.0f, 0.0f,
           0.0f,    0.0f, scale.z, 0.0f,
           0.0f,    0.0f,    0.0f, 1.0f
    );
}

float radians(float degree)
{
    return degree * (3.14159265358979f / 180.0f);
}