#include "transform.h"

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

Matrix4f matmul4f(const Matrix4f* m1, const Matrix4f* m2)
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

Vector4f matvecmul4f(const Matrix4f* m, const Vector4f* v)
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

void get_matrix4f_x_y_z_basis_vectors(const Matrix4f* m, Vector3f* x_basis_vector, Vector3f* y_basis_vector, Vector3f* z_basis_vector)
{
    assert(m);
    assert(x_basis_vector);
    assert(y_basis_vector);
    assert(z_basis_vector);

    x_basis_vector->a[0] = m->a[ 0];
    x_basis_vector->a[1] = m->a[ 4];
    x_basis_vector->a[2] = m->a[ 8];

    y_basis_vector->a[0] = m->a[ 1];
    y_basis_vector->a[1] = m->a[ 5];
    y_basis_vector->a[2] = m->a[ 9];

    z_basis_vector->a[0] = m->a[ 2];
    z_basis_vector->a[1] = m->a[ 6];
    z_basis_vector->a[2] = m->a[10];
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

Matrix4f create_perspective_transform(float field_of_view_y, float aspect_ratio, float near_plane, float far_plane)
{
    assert(field_of_view_y > 0 && field_of_view_y < 360.0f);
    assert(aspect_ratio > 0);
    assert(near_plane > 0);
    assert(far_plane > near_plane);

    Matrix4f result = {{0}};

    const float y_scale = cotangent(degrees_to_radians(field_of_view_y/2));
    const float x_scale = y_scale/aspect_ratio;
    const float frustum_length = far_plane - near_plane;

    result.a[ 0] = x_scale;
    result.a[ 5] = y_scale;
    result.a[10] = -(far_plane + near_plane)/frustum_length;
    result.a[11] = -(2*near_plane*far_plane)/frustum_length;
    result.a[14] = -1;

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
    *m = matmul4f(&rotation, m);
}
