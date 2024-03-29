#ifndef GEOMETRY_H
#define GEOMETRY_H

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
Vector4f extend_vector3f_to_vector4f(const Vector3f* v, float w);
Vector3f extract_vector3f_from_vector4f(const Vector4f* v);

Vector3f homogenize_vector4f(const Vector4f* v);

void set_matrix4f_elements(Matrix4f* m,
                           float a11, float a12, float a13, float a14,
                           float a21, float a22, float a23, float a24,
                           float a31, float a32, float a33, float a34,
                           float a41, float a42, float a43, float a44);
void set_vector2f_elements(Vector2f* v, float x, float y);
void set_vector3_elements(Vector3* v, double x, double y, double z);
void set_vector3f_elements(Vector3f* v, float x, float y, float z);
void set_vector4f_elements(Vector4f* v, float x, float y, float z, float w);

void copy_vector3_to_vector3f(const Vector3* source, Vector3f* destination);
void copy_vector3f_to_vector4f(const Vector3f* source, Vector4f* destination);
void copy_vector4f_to_vector3f(const Vector4f* source, Vector3f* destination);

int equal_vector3f(const Vector3f* v1, const Vector3f* v2);

Vector3 added_vector3(const Vector3* v1, const Vector3* v2);
Vector3 subtracted_vector3(const Vector3* v1, const Vector3* v2);
Vector3 multiplied_vector3(const Vector3* v1, const Vector3* v2);
Vector3 scaled_vector3(const Vector3* v, double scale);

void add_vector3(const Vector3* v1, Vector3* v2);
void subtract_vector3(const Vector3* v1, Vector3* v2);
void multiply_vector3(const Vector3* v1, Vector3* v2);
void scale_vector3(Vector3* v, double scale);

Vector3f added_vector3f(const Vector3f* v1, const Vector3f* v2);
Vector3f subtracted_vector3f(const Vector3f* v1, const Vector3f* v2);
Vector3f multiplied_vector3f(const Vector3f* v1, const Vector3f* v2);
Vector3f scaled_vector3f(const Vector3f* v, float scale);

void add_vector3f(const Vector3f* v1, Vector3f* v2);
void subtract_vector3f(const Vector3f* v1, Vector3f* v2);
void multiply_vector3f(const Vector3f* v1, Vector3f* v2);
void scale_vector3f(Vector3f* v, float scale);

double norm3(const Vector3* v);
float norm3f(const Vector3f* v);

double dot3(const Vector3* v1, const Vector3* v2);
float dot3f(const Vector3f* v1, const Vector3f* v2);

Vector3 cross3(const Vector3* v1, const Vector3* v2);
Vector3f cross3f(const Vector3f* v1, const Vector3f* v2);

void normalize_vector3(Vector3* v);
void normalize_vector3f(Vector3f* v);

void invert_vector3f(Vector3f* v);

void get_matrix4f_first_column_vector3f(const Matrix4f* m, Vector3f* column_vector);
void get_matrix4f_second_column_vector3f(const Matrix4f* m, Vector3f* column_vector);
void get_matrix4f_third_column_vector3f(const Matrix4f* m, Vector3f* column_vector);
void get_matrix4f_fourth_column_vector3f(const Matrix4f* m, Vector3f* column_vector);
void get_matrix4f_first_row_vector3f(const Matrix4f* m, Vector3f* row_vector);
void get_matrix4f_second_row_vector3f(const Matrix4f* m, Vector3f* row_vector);
void get_matrix4f_third_row_vector3f(const Matrix4f* m, Vector3f* row_vector);
void get_matrix4f_fourth_row_vector3f(const Matrix4f* m, Vector3f* row_vector);

Matrix4f multiply_matrix4f(const Matrix4f* m1, const Matrix4f* m2);
Vector4f multiply_matrix4f_vector4f(const Matrix4f* m, const Vector4f* v);
Vector3f multiply_matrix4f_vector3f(const Matrix4f* m, const Vector3f* v);
Vector3f multiply_matrix4f_sub_3x3_vector3f(const Matrix4f* m, const Vector3f* v);
Vector3 multiply_matrix4f_vector3(const Matrix4f* m, const Vector3* v);
Vector3 multiply_matrix4f_sub_3x3_vector3(const Matrix4f* m, const Vector3* v);

void invert_matrix4f(Matrix4f* m);
void invert_matrix4f_3x3_submatrix(Matrix4f* m);
void transpose_matrix4f(Matrix4f* m);

Matrix4f create_scaling_transform(float sx, float sy, float sz);
Matrix4f create_translation_transform(float dx, float dy, float dz);
Matrix4f create_rotation_about_x_transform(float angle);
Matrix4f create_rotation_about_y_transform(float angle);
Matrix4f create_rotation_about_z_transform(float angle);
Matrix4f create_rotation_about_axis_transform(const Vector3f* axis, float angle);
Matrix4f create_perspective_transform(float field_of_view, float aspect_ratio, float near_plane_distance, float far_plane_distance);
Matrix4f create_orthographic_transform(float width, float aspect_ratio, float near_plane_distance, float far_plane_distance);

void apply_scaling(Matrix4f* m, float sx, float sy, float sz);
void apply_translation(Matrix4f* m, float dx, float dy, float dz);
void apply_rotation_about_x(Matrix4f* m, float angle);
void apply_rotation_about_y(Matrix4f* m, float angle);
void apply_rotation_about_z(Matrix4f* m, float angle);
void apply_rotation_about_axis(Matrix4f* m, const Vector3f* axis, float angle);

void set_transform_translation(Matrix4f* m, float dx, float dy, float dz);

void rotate_vector3f_about_axis(Vector3f* vector, const Vector3f* axis, float angle);
void rotate_normal3f_about_axis(Vector3f* vector, const Vector3f* axis, float angle);

#endif
