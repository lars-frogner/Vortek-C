#include "geometry.h"

#include "error.h"
#include "extra_math.h"

#include <stdio.h>
#include <math.h>


const Matrix4f IDENTITY_MATRIX4F =
{{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
}};


Vector3f vector3_to_vector3f(const Vector3* vector3)
{
    assert(vector3);
    Vector3f vector3f = {{(float)vector3->a[0], (float)vector3->a[1], (float)vector3->a[2]}};
    return vector3f;
}

void print_vector2f(const Vector2f* v)
{
    assert(v);
    printf("\n%7.3f\n%7.3f\n",
           v->a[0], v->a[1]);
}

void print_vector3(const Vector3* v)
{
    assert(v);
    printf("\n%7.3f\n%7.3f\n%7.3f\n",
           v->a[0], v->a[1], v->a[2]);
}

void print_vector3f(const Vector3f* v)
{
    assert(v);
    printf("\n%7.3f\n%7.3f\n%7.3f\n",
           v->a[0], v->a[1], v->a[2]);
}

void print_vector4f(const Vector4f* v)
{
    assert(v);
    printf("\n%7.3f\n%7.3f\n%7.3f\n%7.3f\n",
           v->a[0], v->a[1], v->a[2], v->a[3]);
}

void print_matrix4f(const Matrix4f* m)
{
    assert(m);
    printf("\n%7.3f %7.3f %7.3f %7.3f\n%7.3f %7.3f %7.3f %7.3f\n%7.3f %7.3f %7.3f %7.3f\n%7.3f %7.3f %7.3f %7.3f\n",
           m->a[ 0], m->a[ 1], m->a[ 2], m->a[ 3],
           m->a[ 4], m->a[ 5], m->a[ 6], m->a[ 7],
           m->a[ 8], m->a[ 9], m->a[10], m->a[11],
           m->a[12], m->a[13], m->a[14], m->a[15]);
}

Vector2f create_vector2f(float x, float y)
{
    Vector2f result = {{x, y}};
    return result;
}

Vector3 create_vector3(double x, double y, double z)
{
    Vector3 result = {{x, y, z}};
    return result;
}

Vector3f create_vector3f(float x, float y, float z)
{
    Vector3f result = {{x, y, z}};
    return result;
}

Vector4f create_vector4f(float x, float y, float z, float w)
{
    Vector4f result = {{x, y, z, w}};
    return result;
}

Vector4f extend_vector3f_to_vector4f(const Vector3f* v, float w)
{
    Vector4f result = {{v->a[0], v->a[1], v->a[2], w}};
    return result;
}

Vector3f extract_vector3f_from_vector4f(const Vector4f* v)
{
    Vector3f result = {{v->a[0], v->a[1], v->a[2]}};
    return result;
}

Vector3f homogenize_vector4f(const Vector4f* v)
{
    assert(v->a[3] != 0);
    const float norm = 1.0f/v->a[3];
    Vector3f result = {{v->a[0]*norm, v->a[1]*norm, v->a[2]*norm}};
    return result;
}

void set_matrix4f_elements(Matrix4f* m,
                           float a11, float a12, float a13, float a14,
                           float a21, float a22, float a23, float a24,
                           float a31, float a32, float a33, float a34,
                           float a41, float a42, float a43, float a44)
{
    assert(m);
    m->a[0] = a11;
    m->a[1] = a12;
    m->a[2] = a13;
    m->a[3] = a14;
    m->a[4] = a21;
    m->a[5] = a22;
    m->a[6] = a23;
    m->a[7] = a24;
    m->a[8] = a31;
    m->a[9] = a32;
    m->a[10] = a33;
    m->a[11] = a34;
    m->a[12] = a41;
    m->a[13] = a42;
    m->a[14] = a43;
    m->a[15] = a44;
}

void set_vector2f_elements(Vector2f* v, float x, float y)
{
    assert(v);
    v->a[0] = x;
    v->a[1] = y;
}

void set_vector3_elements(Vector3* v, double x, double y, double z)
{
    assert(v);
    v->a[0] = x;
    v->a[1] = y;
    v->a[2] = z;
}

void set_vector3f_elements(Vector3f* v, float x, float y, float z)
{
    assert(v);
    v->a[0] = x;
    v->a[1] = y;
    v->a[2] = z;
}

void set_vector4f_elements(Vector4f* v, float x, float y, float z, float w)
{
    assert(v);
    v->a[0] = x;
    v->a[1] = y;
    v->a[2] = z;
    v->a[3] = w;
}

void copy_vector3f(const Vector3f* source, Vector3f* destination)
{
    destination->a[0] = source->a[0];
    destination->a[1] = source->a[1];
    destination->a[2] = source->a[2];
}

int equal_vector3f(const Vector3f* v1, const Vector3f* v2)
{
    return (v1->a[0] == v2->a[0]) && (v1->a[1] == v2->a[1]) && (v1->a[2] == v2->a[2]);
}

Vector3 add_vector3(const Vector3* v1, const Vector3* v2)
{
    assert(v1);
    assert(v2);
    Vector3 result = {{v1->a[0] + v2->a[0], v1->a[1] + v2->a[1], v1->a[2] + v2->a[2]}};
    return result;
}

Vector3 subtract_vector3(const Vector3* v1, const Vector3* v2)
{
    assert(v1);
    assert(v2);
    Vector3 result = {{v1->a[0] - v2->a[0], v1->a[1] - v2->a[1], v1->a[2] - v2->a[2]}};
    return result;
}

Vector3 multiply_vector3(const Vector3* v1, const Vector3* v2)
{
    assert(v1);
    assert(v2);
    Vector3 result = {{v1->a[0]*v2->a[0], v1->a[1]*v2->a[1], v1->a[2]*v2->a[2]}};
    return result;
}

Vector3 scale_vector3(const Vector3* v, double scale)
{
    assert(v);
    Vector3 result = {{scale*v->a[0], scale*v->a[1], scale*v->a[2]}};
    return result;
}

Vector3f add_vector3f(const Vector3f* v1, const Vector3f* v2)
{
    assert(v1);
    assert(v2);
    Vector3f result = {{v1->a[0] + v2->a[0], v1->a[1] + v2->a[1], v1->a[2] + v2->a[2]}};
    return result;
}

Vector3f subtract_vector3f(const Vector3f* v1, const Vector3f* v2)
{
    assert(v1);
    assert(v2);
    Vector3f result = {{v1->a[0] - v2->a[0], v1->a[1] - v2->a[1], v1->a[2] - v2->a[2]}};
    return result;
}

Vector3f multiply_vector3f(const Vector3f* v1, const Vector3f* v2)
{
    assert(v1);
    assert(v2);
    Vector3f result = {{v1->a[0]*v2->a[0], v1->a[1]*v2->a[1], v1->a[2]*v2->a[2]}};
    return result;
}

Vector3f scale_vector3f(const Vector3f* v, float scale)
{
    assert(v);
    Vector3f result = {{scale*v->a[0], scale*v->a[1], scale*v->a[2]}};
    return result;
}

double norm3(const Vector3* v)
{
    assert(v);
    return sqrt(dot3(v, v));
}

float norm3f(const Vector3f* v)
{
    assert(v);
    return sqrtf(dot3f(v, v));
}

double dot3(const Vector3* v1, const Vector3* v2)
{
    assert(v1);
    assert(v2);
    return v1->a[0]*v2->a[0] + v1->a[1]*v2->a[1] + v1->a[2]*v2->a[2];
}

float dot3f(const Vector3f* v1, const Vector3f* v2)
{
    assert(v1);
    assert(v2);
    return v1->a[0]*v2->a[0] + v1->a[1]*v2->a[1] + v1->a[2]*v2->a[2];
}

Vector3 cross3(const Vector3* v1, const Vector3* v2)
{
    assert(v1);
    assert(v2);
    Vector3 result;
    result.a[0] = v1->a[1]*v2->a[2] - v1->a[2]*v2->a[1];
    result.a[1] = v1->a[2]*v2->a[0] - v1->a[0]*v2->a[2];
    result.a[2] = v1->a[0]*v2->a[1] - v1->a[1]*v2->a[0];
    return result;
}

Vector3f cross3f(const Vector3f* v1, const Vector3f* v2)
{
    assert(v1);
    assert(v2);
    Vector3f result;
    result.a[0] = v1->a[1]*v2->a[2] - v1->a[2]*v2->a[1];
    result.a[1] = v1->a[2]*v2->a[0] - v1->a[0]*v2->a[2];
    result.a[2] = v1->a[0]*v2->a[1] - v1->a[1]*v2->a[0];
    return result;
}

Matrix4f multiply_matrix4f(const Matrix4f* m1, const Matrix4f* m2)
{
    assert(m1);
    assert(m2);
    Matrix4f result;
    result.a[ 0] = m1->a[ 0]*m2->a[0] + m1->a[ 1]*m2->a[4] + m1->a[ 2]*m2->a[ 8] + m1->a[ 3]*m2->a[12];
    result.a[ 1] = m1->a[ 0]*m2->a[1] + m1->a[ 1]*m2->a[5] + m1->a[ 2]*m2->a[ 9] + m1->a[ 3]*m2->a[13];
    result.a[ 2] = m1->a[ 0]*m2->a[2] + m1->a[ 1]*m2->a[6] + m1->a[ 2]*m2->a[10] + m1->a[ 3]*m2->a[14];
    result.a[ 3] = m1->a[ 0]*m2->a[3] + m1->a[ 1]*m2->a[7] + m1->a[ 2]*m2->a[11] + m1->a[ 3]*m2->a[15];
    result.a[ 4] = m1->a[ 4]*m2->a[0] + m1->a[ 5]*m2->a[4] + m1->a[ 6]*m2->a[ 8] + m1->a[ 7]*m2->a[12];
    result.a[ 5] = m1->a[ 4]*m2->a[1] + m1->a[ 5]*m2->a[5] + m1->a[ 6]*m2->a[ 9] + m1->a[ 7]*m2->a[13];
    result.a[ 6] = m1->a[ 4]*m2->a[2] + m1->a[ 5]*m2->a[6] + m1->a[ 6]*m2->a[10] + m1->a[ 7]*m2->a[14];
    result.a[ 7] = m1->a[ 4]*m2->a[3] + m1->a[ 5]*m2->a[7] + m1->a[ 6]*m2->a[11] + m1->a[ 7]*m2->a[15];
    result.a[ 8] = m1->a[ 8]*m2->a[0] + m1->a[ 9]*m2->a[4] + m1->a[10]*m2->a[ 8] + m1->a[11]*m2->a[12];
    result.a[ 9] = m1->a[ 8]*m2->a[1] + m1->a[ 9]*m2->a[5] + m1->a[10]*m2->a[ 9] + m1->a[11]*m2->a[13];
    result.a[10] = m1->a[ 8]*m2->a[2] + m1->a[ 9]*m2->a[6] + m1->a[10]*m2->a[10] + m1->a[11]*m2->a[14];
    result.a[11] = m1->a[ 8]*m2->a[3] + m1->a[ 9]*m2->a[7] + m1->a[10]*m2->a[11] + m1->a[11]*m2->a[15];
    result.a[12] = m1->a[12]*m2->a[0] + m1->a[13]*m2->a[4] + m1->a[14]*m2->a[ 8] + m1->a[15]*m2->a[12];
    result.a[13] = m1->a[12]*m2->a[1] + m1->a[13]*m2->a[5] + m1->a[14]*m2->a[ 9] + m1->a[15]*m2->a[13];
    result.a[14] = m1->a[12]*m2->a[2] + m1->a[13]*m2->a[6] + m1->a[14]*m2->a[10] + m1->a[15]*m2->a[14];
    result.a[15] = m1->a[12]*m2->a[3] + m1->a[13]*m2->a[7] + m1->a[14]*m2->a[11] + m1->a[15]*m2->a[15];
    return result;
}

Vector4f multiply_matrix4f_vector4f(const Matrix4f* m, const Vector4f* v)
{
    assert(m);
    assert(v);
    Vector4f result;
    result.a[0] = m->a[ 0]*v->a[0] + m->a[ 1]*v->a[1] + m->a[ 2]*v->a[2] + m->a[ 3]*v->a[3];
    result.a[1] = m->a[ 4]*v->a[0] + m->a[ 5]*v->a[1] + m->a[ 6]*v->a[2] + m->a[ 7]*v->a[3];
    result.a[2] = m->a[ 8]*v->a[0] + m->a[ 9]*v->a[1] + m->a[10]*v->a[2] + m->a[11]*v->a[3];
    result.a[3] = m->a[12]*v->a[0] + m->a[13]*v->a[1] + m->a[14]*v->a[2] + m->a[15]*v->a[3];
    return result;
}

Vector3f multiply_matrix4f_vector3f(const Matrix4f* m, const Vector3f* v)
{
    assert(m);
    assert(v);
    Vector3f result;
    result.a[0] = m->a[ 0]*v->a[0] + m->a[ 1]*v->a[1] + m->a[ 2]*v->a[2] + m->a[ 3];
    result.a[1] = m->a[ 4]*v->a[0] + m->a[ 5]*v->a[1] + m->a[ 6]*v->a[2] + m->a[ 7];
    result.a[2] = m->a[ 8]*v->a[0] + m->a[ 9]*v->a[1] + m->a[10]*v->a[2] + m->a[11];
    return result;
}

float multiply_matrix4f_vector3f_z_only(const Matrix4f* m, const Vector3f* v)
{
    assert(m);
    assert(v);
    return m->a[8]*v->a[0] + m->a[9]*v->a[1] + m->a[10]*v->a[2] + m->a[11];
}

void invert_matrix4f(Matrix4f* m)
{
    assert(m);

    const float a11 = m->a[ 0];
    const float a12 = m->a[ 1];
    const float a13 = m->a[ 2];
    const float a14 = m->a[ 3];
    const float a21 = m->a[ 4];
    const float a22 = m->a[ 5];
    const float a23 = m->a[ 6];
    const float a24 = m->a[ 7];
    const float a31 = m->a[ 8];
    const float a32 = m->a[ 9];
    const float a33 = m->a[10];
    const float a34 = m->a[11];
    const float a41 = m->a[12];
    const float a42 = m->a[13];
    const float a43 = m->a[14];
    const float a44 = m->a[15];

    float b11, b12, b13, b14,
          b21, b22, b23, b24,
          b31, b32, b33, b34,
          b41, b42, b43, b44;

    b11 = a22*a33*a44 -
          a22*a34*a43 -
          a32*a23*a44 +
          a32*a24*a43 +
          a42*a23*a34 -
          a42*a24*a33;

    b21 = -a21*a33*a44 +
           a21*a34*a43 +
           a31*a23*a44 -
           a31*a24*a43 -
           a41*a23*a34 +
           a41*a24*a33;

    b31 = a21*a32*a44 -
          a21*a34*a42 -
          a31*a22*a44 +
          a31*a24*a42 +
          a41*a22*a34 -
          a41*a24*a32;

    b41 = -a21*a32*a43 +
           a21*a33*a42 +
           a31*a22*a43 -
           a31*a23*a42 -
           a41*a22*a33 +
           a41*a23*a32;

    b12 = -a12*a33*a44 +
           a12*a34*a43 +
           a32*a13*a44 -
           a32*a14*a43 -
           a42*a13*a34 +
           a42*a14*a33;

    b22 = a11*a33*a44 -
          a11*a34*a43 -
          a31*a13*a44 +
          a31*a14*a43 +
          a41*a13*a34 -
          a41*a14*a33;

    b32 = -a11*a32*a44 +
           a11*a34*a42 +
           a31*a12*a44 -
           a31*a14*a42 -
           a41*a12*a34 +
           a41*a14*a32;

    b42 = a11*a32*a43 -
          a11*a33*a42 -
          a31*a12*a43 +
          a31*a13*a42 +
          a41*a12*a33 -
          a41*a13*a32;

    b13 = a12*a23*a44 -
          a12*a24*a43 -
          a22*a13*a44 +
          a22*a14*a43 +
          a42*a13*a24 -
          a42*a14*a23;

    b23 = -a11*a23*a44 +
           a11*a24*a43 +
           a21*a13*a44 -
           a21*a14*a43 -
           a41*a13*a24 +
           a41*a14*a23;

    b33 = a11*a22*a44 -
          a11*a24*a42 -
          a21*a12*a44 +
          a21*a14*a42 +
          a41*a12*a24 -
          a41*a14*a22;

    b43 = -a11*a22*a43 +
           a11*a23*a42 +
           a21*a12*a43 -
           a21*a13*a42 -
           a41*a12*a23 +
           a41*a13*a22;

    b14 = -a12*a23*a34 +
           a12*a24*a33 +
           a22*a13*a34 -
           a22*a14*a33 -
           a32*a13*a24 +
           a32*a14*a23;

    b24 = a11*a23*a34 -
          a11*a24*a33 -
          a21*a13*a34 +
          a21*a14*a33 +
          a31*a13*a24 -
          a31*a14*a23;

    b34 = -a11*a22*a34 +
           a11*a24*a32 +
           a21*a12*a34 -
           a21*a14*a32 -
           a31*a12*a24 +
           a31*a14*a22;

    b44 = a11*a22*a33 -
          a11*a23*a32 -
          a21*a12*a33 +
          a21*a13*a32 +
          a31*a12*a23 -
          a31*a13*a22;

    float inv_det = a11*b11 + a12*b21 + a13*b31 + a14*b41;

    // Make sure matrix is invertible (non-zero determinant)
    assert(inv_det != 0);

    inv_det = 1.0f/inv_det;

    set_matrix4f_elements(m,
                          b11*inv_det, b12*inv_det, b13*inv_det, b14*inv_det,
                          b21*inv_det, b22*inv_det, b23*inv_det, b24*inv_det,
                          b31*inv_det, b32*inv_det, b33*inv_det, b34*inv_det,
                          b41*inv_det, b42*inv_det, b43*inv_det, b44*inv_det);
}

void invert_matrix4f_3x3_submatrix(Matrix4f* m)
{
    assert(m);

    const float a11 = m->a[ 0];
    const float a12 = m->a[ 1];
    const float a13 = m->a[ 2];
    const float a21 = m->a[ 4];
    const float a22 = m->a[ 5];
    const float a23 = m->a[ 6];
    const float a31 = m->a[ 8];
    const float a32 = m->a[ 9];
    const float a33 = m->a[10];

    const float diff1 = a22*a33 - a23*a32;
    const float diff2 = a23*a31 - a21*a33;
    const float diff3 = a21*a32 - a22*a31;

    // Make sure matrix is invertible (non-zero determinant)
    float inv_det = a11*diff1 + a12*diff2 + a13*diff3;

    assert(inv_det != 0);

    inv_det = 1.0f/inv_det;

    m->a[ 0] =               diff1*inv_det;
    m->a[ 1] = (a13*a32 - a12*a33)*inv_det;
    m->a[ 2] = (a12*a23 - a13*a22)*inv_det;
    m->a[ 4] =               diff2*inv_det;
    m->a[ 5] = (a11*a33 - a13*a31)*inv_det;
    m->a[ 6] = (a13*a21 - a11*a23)*inv_det;
    m->a[ 8] =               diff3*inv_det;
    m->a[ 9] = (a12*a31 - a11*a32)*inv_det;
    m->a[10] = (a11*a22 - a12*a21)*inv_det;
}

void normalize_vector3(Vector3* v)
{
    assert(v);
    const double norm = norm3(v);
    assert(norm > 0);
    const double scale = 1.0/norm;
    v->a[0] *= scale;
    v->a[1] *= scale;
    v->a[2] *= scale;
}

void normalize_vector3f(Vector3f* v)
{
    assert(v);
    const float norm = norm3f(v);
    assert(norm > 0);
    const float scale = 1.0f/norm;
    v->a[0] *= scale;
    v->a[1] *= scale;
    v->a[2] *= scale;
}

void get_matrix4f_first_column_vector3f(const Matrix4f* m, Vector3f* column_vector)
{
    assert(m);
    assert(column_vector);

    column_vector->a[0] = m->a[ 0];
    column_vector->a[1] = m->a[ 4];
    column_vector->a[2] = m->a[ 8];
}

void get_matrix4f_second_column_vector3f(const Matrix4f* m, Vector3f* column_vector)
{
    assert(m);
    assert(column_vector);

    column_vector->a[0] = m->a[ 1];
    column_vector->a[1] = m->a[ 5];
    column_vector->a[2] = m->a[ 9];
}

void get_matrix4f_third_column_vector3f(const Matrix4f* m, Vector3f* column_vector)
{
    assert(m);
    assert(column_vector);

    column_vector->a[0] = m->a[ 2];
    column_vector->a[1] = m->a[ 6];
    column_vector->a[2] = m->a[10];
}

void get_matrix4f_fourth_column_vector3f(const Matrix4f* m, Vector3f* column_vector)
{
    assert(m);
    assert(column_vector);

    column_vector->a[0] = m->a[ 3];
    column_vector->a[1] = m->a[ 7];
    column_vector->a[2] = m->a[11];
}

void get_matrix4f_first_row_vector3f(const Matrix4f* m, Vector3f* row_vector)
{
    assert(m);
    assert(row_vector);

    row_vector->a[0] = m->a[ 0];
    row_vector->a[1] = m->a[ 1];
    row_vector->a[2] = m->a[ 2];
}

void get_matrix4f_second_row_vector3f(const Matrix4f* m, Vector3f* row_vector)
{
    assert(m);
    assert(row_vector);

    row_vector->a[0] = m->a[ 4];
    row_vector->a[1] = m->a[ 5];
    row_vector->a[2] = m->a[ 6];
}

void get_matrix4f_third_row_vector3f(const Matrix4f* m, Vector3f* row_vector)
{
    assert(m);
    assert(row_vector);

    row_vector->a[0] = m->a[ 8];
    row_vector->a[1] = m->a[ 9];
    row_vector->a[2] = m->a[10];
}

void get_matrix4f_fourth_row_vector3f(const Matrix4f* m, Vector3f* row_vector)
{
    assert(m);
    assert(row_vector);

    row_vector->a[0] = m->a[12];
    row_vector->a[1] = m->a[13];
    row_vector->a[2] = m->a[14];
}

Matrix4f create_scaling_transform(float sx, float sy, float sz)
{
    assert(sx > 0);
    assert(sy > 0);
    assert(sz > 0);

    Matrix4f result = IDENTITY_MATRIX4F;
    result.a[ 0] = sx;
    result.a[ 5] = sy;
    result.a[10] = sz;
    return result;
}

Matrix4f create_translation_transform(float dx, float dy, float dz)
{
    Matrix4f result = IDENTITY_MATRIX4F;
    result.a[ 3] = dx;
    result.a[ 7] = dy;
    result.a[11] = dz;
    return result;
}

Matrix4f create_rotation_about_x_transform(float angle)
{
    Matrix4f result = IDENTITY_MATRIX4F;
    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);
    result.a[ 5] =  cos_angle;
    result.a[ 6] = -sin_angle;
    result.a[ 9] =  sin_angle;
    result.a[10] =  cos_angle;
    return result;
}

Matrix4f create_rotation_about_y_transform(float angle)
{
    Matrix4f result = IDENTITY_MATRIX4F;
    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);
    result.a[ 0] =  cos_angle;
    result.a[ 2] =  sin_angle;
    result.a[ 8] = -sin_angle;
    result.a[10] =  cos_angle;
    return result;
}

Matrix4f create_rotation_about_z_transform(float angle)
{
    Matrix4f result = IDENTITY_MATRIX4F;
    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);
    result.a[0] =  cos_angle;
    result.a[1] = -sin_angle;
    result.a[4] =  sin_angle;
    result.a[5] =  cos_angle;
    return result;
}

Matrix4f create_rotation_about_axis_transform(const Vector3f* axis, float angle)
{
    assert(axis);

    Matrix4f result = IDENTITY_MATRIX4F;

    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);
    const float one_minus_cos_angle = 1.0f - cos_angle;
    const float axis_x = axis->a[0];
    const float axis_y = axis->a[1];
    const float axis_z = axis->a[2];

    result.a[ 0] = (axis_x*axis_x*one_minus_cos_angle) + (       cos_angle);
    result.a[ 5] = (axis_y*axis_y*one_minus_cos_angle) + (       cos_angle);
    result.a[10] = (axis_z*axis_z*one_minus_cos_angle) + (       cos_angle);

    result.a[ 1] = (axis_x*axis_y*one_minus_cos_angle) - (axis_z*sin_angle);
    result.a[ 4] = (axis_x*axis_y*one_minus_cos_angle) + (axis_z*sin_angle);

    result.a[ 6] = (axis_y*axis_z*one_minus_cos_angle) - (axis_x*sin_angle);
    result.a[ 9] = (axis_y*axis_z*one_minus_cos_angle) + (axis_x*sin_angle);

    result.a[ 2] = (axis_z*axis_x*one_minus_cos_angle) + (axis_y*sin_angle);
    result.a[ 8] = (axis_z*axis_x*one_minus_cos_angle) - (axis_y*sin_angle);

    return result;
}

Matrix4f create_perspective_transform(float field_of_view, float aspect_ratio, float near_plane_distance, float far_plane_distance)
{
    assert(field_of_view > 0 && field_of_view < 360.0f);
    assert(aspect_ratio > 0);
    assert(near_plane_distance > 0);
    assert(far_plane_distance > near_plane_distance);

    Matrix4f result = {{0}};

    const float x_scale = cotangent(degrees_to_radians(field_of_view/2));
    const float y_scale = x_scale*aspect_ratio;
    const float frustum_length = far_plane_distance - near_plane_distance;

    result.a[ 0] = x_scale;
    result.a[ 5] = y_scale;
    result.a[10] = -(far_plane_distance + near_plane_distance)/frustum_length;
    result.a[11] = -(2*near_plane_distance*far_plane_distance)/frustum_length;
    result.a[14] = -1;

    return result;
}

Matrix4f create_orthographic_transform(float width, float aspect_ratio, float near_plane_distance, float far_plane_distance)
{
    assert(width > 0);
    assert(aspect_ratio > 0);
    assert(near_plane_distance > 0);
    assert(far_plane_distance > near_plane_distance);

    Matrix4f result = {{0}};

    const float height = width*aspect_ratio;
    const float depth = far_plane_distance - near_plane_distance;

    result.a[ 0] = 2.0f/width;
    result.a[ 5] = 2.0f/height;
    result.a[10] = -2.0f/depth;
    result.a[11] = -(far_plane_distance + near_plane_distance)/depth;
    result.a[15] = 1;

    return result;
}

void apply_scaling(Matrix4f* m, float sx, float sy, float sz)
{
    assert(m);
    assert(sx > 0);
    assert(sy > 0);
    assert(sz > 0);

    m->a[ 0] *= sx;
    m->a[ 1] *= sx;
    m->a[ 2] *= sx;
    m->a[ 3] *= sx;

    m->a[ 4] *= sy;
    m->a[ 5] *= sy;
    m->a[ 6] *= sy;
    m->a[ 7] *= sy;

    m->a[ 8] *= sz;
    m->a[ 9] *= sz;
    m->a[10] *= sz;
    m->a[11] *= sz;
}

void apply_translation(Matrix4f* m, float dx, float dy, float dz)
{
    assert(m);

    m->a[ 0] += dx*m->a[12];
    m->a[ 1] += dx*m->a[13];
    m->a[ 2] += dx*m->a[14];
    m->a[ 3] += dx*m->a[15];

    m->a[ 4] += dy*m->a[12];
    m->a[ 5] += dy*m->a[13];
    m->a[ 6] += dy*m->a[14];
    m->a[ 7] += dy*m->a[15];

    m->a[ 8] += dz*m->a[12];
    m->a[ 9] += dz*m->a[13];
    m->a[10] += dz*m->a[14];
    m->a[11] += dz*m->a[15];
}

void apply_rotation_about_x(Matrix4f* m, float angle)
{
    assert(m);

    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);

    float temp;

    temp     = cos_angle*m->a[4] - sin_angle*m->a[ 8];
    m->a[ 8] = sin_angle*m->a[4] + cos_angle*m->a[ 8];
    m->a[ 4] = temp;

    temp     = cos_angle*m->a[5] - sin_angle*m->a[ 9];
    m->a[ 9] = sin_angle*m->a[5] + cos_angle*m->a[ 9];
    m->a[ 5] = temp;

    temp     = cos_angle*m->a[6] - sin_angle*m->a[10];
    m->a[10] = sin_angle*m->a[6] + cos_angle*m->a[10];
    m->a[ 6] = temp;

    temp     = cos_angle*m->a[7] - sin_angle*m->a[11];
    m->a[11] = sin_angle*m->a[7] + cos_angle*m->a[11];
    m->a[ 7] = temp;
}

void apply_rotation_about_y(Matrix4f* m, float angle)
{
    assert(m);

    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);

    float temp;

    temp     = cos_angle*m->a[ 8] - sin_angle*m->a[0];
    m->a[ 0] = sin_angle*m->a[ 8] + cos_angle*m->a[0];
    m->a[ 8] = temp;

    temp     = cos_angle*m->a[ 9] - sin_angle*m->a[1];
    m->a[ 1] = sin_angle*m->a[ 9] + cos_angle*m->a[1];
    m->a[ 9] = temp;

    temp     = cos_angle*m->a[10] - sin_angle*m->a[2];
    m->a[ 2] = sin_angle*m->a[10] + cos_angle*m->a[2];
    m->a[10] = temp;

    temp     = cos_angle*m->a[11] - sin_angle*m->a[3];
    m->a[ 3] = sin_angle*m->a[11] + cos_angle*m->a[3];
    m->a[11] = temp;
}

void apply_rotation_about_z(Matrix4f* m, float angle)
{
    assert(m);

    const float sin_angle = sinf(angle);
    const float cos_angle = cosf(angle);

    float temp;

    temp     = cos_angle*m->a[0] - sin_angle*m->a[4];
    m->a[ 4] = sin_angle*m->a[0] + cos_angle*m->a[4];
    m->a[ 0] = temp;

    temp     = cos_angle*m->a[1] - sin_angle*m->a[5];
    m->a[ 5] = sin_angle*m->a[1] + cos_angle*m->a[5];
    m->a[ 1] = temp;

    temp     = cos_angle*m->a[2] - sin_angle*m->a[6];
    m->a[ 6] = sin_angle*m->a[2] + cos_angle*m->a[6];
    m->a[ 2] = temp;

    temp     = cos_angle*m->a[3] - sin_angle*m->a[7];
    m->a[ 7] = sin_angle*m->a[3] + cos_angle*m->a[7];
    m->a[ 3] = temp;
}

void apply_rotation_about_axis(Matrix4f* m, const Vector3f* axis, float angle)
{
    assert(m);
    assert(axis);

    const Matrix4f rotation = create_rotation_about_axis_transform(axis, angle);
    *m = multiply_matrix4f(&rotation, m);
}

void set_transform_translation(Matrix4f* m, float dx, float dy, float dz)
{
    assert(m);
    m->a[ 3] = dx;
    m->a[ 7] = dy;
    m->a[11] = dz;
}
