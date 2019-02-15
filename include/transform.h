#ifndef TRANSFORM_H
#define TRANSFORM_H

typedef struct Vector2f
{
    float a[2];
} Vector2f;

typedef struct Vector3
{
    double a[3];
} Vector3;

typedef struct Vector3f
{
    float a[3];
} Vector3f;

typedef struct Vector4f
{
    float a[4];
} Vector4f;

typedef struct Matrix4f
{
    float a[16];
} Matrix4f;

extern const Matrix4f IDENTITY_MATRIX4F;

Vector3f vector3_to_vector3f(const Vector3* vector3);

void print_vector2f(const Vector2f* v);
void print_vector3(const Vector3* v);
void print_vector3f(const Vector3f* v);
void print_vector4f(const Vector4f* v);
void print_matrix4f(const Matrix4f* m);

Vector2f create_vector2f(float x, float y);
Vector3 create_vector3(double x, double y, double z);
Vector3f create_vector3f(float x, float y, float z);
Vector4f create_vector4f(float x, float y, float z, float w);

void set_vector2f_elements(Vector2f* v, float x, float y);
void set_vector3_elements(Vector3* v, double x, double y, double z);
void set_vector3f_elements(Vector3f* v, float x, float y, float z);
void set_vector4f_elements(Vector4f* v, float x, float y, float z, float w);

Vector3 add_vector3(const Vector3* v1, const Vector3* v2);
Vector3 subtract_vector3(const Vector3* v1, const Vector3* v2);
Vector3 scale_vector3(const Vector3* v, double scale);

Vector3f add_vector3f(const Vector3f* v1, const Vector3f* v2);
Vector3f subtract_vector3f(const Vector3f* v1, const Vector3f* v2);
Vector3f scale_vector3f(const Vector3f* v, float scale);

double norm3(const Vector3* v);
float norm3f(const Vector3f* v);

double dot3(const Vector3* v1, const Vector3* v2);
float dot3f(const Vector3f* v1, const Vector3f* v2);

Vector3 cross3(const Vector3* v1, const Vector3* v2);
Vector3f cross3f(const Vector3f* v1, const Vector3f* v2);

void normalize_vector3(Vector3* v);
void normalize_vector3f(Vector3f* v);

void get_matrix4f_x_y_z_basis_vectors(const Matrix4f* m, Vector3f* x_basis_vector, Vector3f* y_basis_vector, Vector3f* z_basis_vector);

Matrix4f matmul4f(const Matrix4f* m1, const Matrix4f* m2);
Vector4f matvecmul4f(const Matrix4f* m, const Vector4f* v);

Matrix4f create_scaling_transform(float sx, float sy, float sz);
Matrix4f create_translation_transform(float dx, float dy, float dz);
Matrix4f create_rotation_about_x_transform(float angle);
Matrix4f create_rotation_about_y_transform(float angle);
Matrix4f create_rotation_about_z_transform(float angle);
Matrix4f create_rotation_about_axis_transform(const Vector3f* axis, float angle);
Matrix4f create_perspective_transform(float field_of_view_y, float aspect_ratio, float near_plane, float far_plane);

void apply_scaling(Matrix4f* m, float sx, float sy, float sz);
void apply_translation(Matrix4f* m, float dx, float dy, float dz);
void apply_rotation_about_x(Matrix4f* m, float angle);
void apply_rotation_about_y(Matrix4f* m, float angle);
void apply_rotation_about_z(Matrix4f* m, float angle);
void apply_rotation_about_axis(Matrix4f* m, const Vector3f* axis, float angle);

#endif
