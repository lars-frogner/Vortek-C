#include "bricks.h"

#include "error.h"
#include "extra_math.h"

#include <stdlib.h>


#define MIN_BRICK_SIZE 8


static void copy_subarray_with_cycled_layout(const float* full_input_array,
                                             size_t full_input_size_y, size_t full_input_size_z,
                                             size_t input_offset_x, size_t input_offset_y, size_t input_offset_z,
                                             float* full_output_array,
                                             size_t full_output_size_x, size_t full_output_size_y, size_t full_output_size_z,
                                             size_t output_offset_x, size_t output_offset_y, size_t output_offset_z,
                                             size_t subsize_x, size_t subsize_y, size_t subsize_z,
                                             unsigned int cycle);


void create_bricked_field(BrickedField* bricked_field, const Field* field, unsigned int brick_size_exponent, unsigned int kernel_size)
{
    check(bricked_field);
    check(field);
    check(field->data);
    check(kernel_size > 0);

    if (field->type != SCALAR_FIELD)
        print_severe_message("Bricking is only supported for scalar fields.");

    float* const original_data = field->data;
    const size_t field_size_x = field->size_x;
    const size_t field_size_y = field->size_y;
    const size_t field_size_z = field->size_z;

    const size_t pad = kernel_size - 1;

    const size_t brick_size = max_size(pow2_size_t(brick_size_exponent), MIN_BRICK_SIZE);
    const size_t padded_brick_size = brick_size + 2*pad;
    const size_t padded_brick_length = padded_brick_size*padded_brick_size*padded_brick_size;

    if (brick_size > field_size_x || brick_size > field_size_y || brick_size > field_size_z)
        print_severe_message("Brick dimensions (%d, %d, %d) exceed field dimensions of (%d, %d, %d).",
                             brick_size, brick_size, brick_size, field_size_x, field_size_y, field_size_z);

    const size_t n_full_bricks_x = field_size_x/brick_size;
    const size_t n_full_bricks_y = field_size_y/brick_size;
    const size_t n_full_bricks_z = field_size_z/brick_size;

    const size_t has_remaining_x = (field_size_x - n_full_bricks_x*brick_size) > 0;
    const size_t has_remaining_y = (field_size_y - n_full_bricks_y*brick_size) > 0;
    const size_t has_remaining_z = (field_size_z - n_full_bricks_z*brick_size) > 0;

    const size_t n_bricks_x = n_full_bricks_x + has_remaining_x;
    const size_t n_bricks_y = n_full_bricks_y + has_remaining_y;
    const size_t n_bricks_z = n_full_bricks_z + has_remaining_z;

    const size_t n_bricks = n_bricks_x*n_bricks_y*n_bricks_z;

    printf("%d, %d, %d\n", field_size_x, field_size_y, field_size_z);
    printf("%d\n", pad);
    printf("%d, %d, %d\n", brick_size, padded_brick_size, padded_brick_length);
    printf("%d, %d, %d\n", n_full_bricks_x, n_full_bricks_y, n_full_bricks_z);
    printf("%d, %d, %d\n", has_remaining_x, has_remaining_y, has_remaining_z);
    printf("%d, %d, %d, %d\n", n_bricks_x, n_bricks_y, n_bricks_z, n_bricks);

    Brick* const bricks = (Brick*)malloc(sizeof(Brick)*n_bricks);
    check(bricks);

    const size_t new_data_size_x = n_bricks_x*padded_brick_size;
    const size_t new_data_size_y = n_bricks_y*padded_brick_size;
    const size_t new_data_size_z = n_bricks_z*padded_brick_size;

    const size_t new_data_length = n_bricks*padded_brick_length;
    float* const new_data = (float*)calloc(new_data_length, sizeof(float));
    check(new_data);

    size_t i, j, k;
    unsigned int cycle;
    size_t brick_idx;
    size_t data_offset = 0;
    size_t original_data_offset_x;
    size_t original_data_offset_y;
    size_t original_data_offset_z;
    size_t new_data_offset_x;
    size_t new_data_offset_y;
    size_t new_data_offset_z;
    size_t effective_brick_size_x;
    size_t effective_brick_size_y;
    size_t effective_brick_size_z;

    for (i = 0; i < n_bricks_x; i++)
    {
        for (j = 0; j < n_bricks_y; j++)
        {
            for (k = 0; k < n_bricks_z; k++)
            {
                printf("%d, %d, %d\n", i, j, k);

                original_data_offset_x = i*brick_size - (i > 0)*pad;
                original_data_offset_y = j*brick_size - (j > 0)*pad;
                original_data_offset_z = k*brick_size - (k > 0)*pad;

                new_data_offset_x = i*padded_brick_size + (i == 0)*pad;
                new_data_offset_y = j*padded_brick_size + (j == 0)*pad;
                new_data_offset_z = k*padded_brick_size + (k == 0)*pad;

                printf("%d, %d\n", padded_brick_size - (k == 0)*pad, field_size_z - original_data_offset_z);

                effective_brick_size_x = min_size(padded_brick_size - (i == 0)*pad, field_size_x - original_data_offset_x);
                effective_brick_size_y = min_size(padded_brick_size - (j == 0)*pad, field_size_y - original_data_offset_y);
                effective_brick_size_z = min_size(padded_brick_size - (k == 0)*pad, field_size_z - original_data_offset_z);

                cycle = (i + j + k) % 3;

                copy_subarray_with_cycled_layout(original_data,
                                                 field_size_y, field_size_z,
                                                 original_data_offset_x, original_data_offset_y, original_data_offset_z,
                                                 new_data,
                                                 new_data_size_x, new_data_size_y, new_data_size_z,
                                                 new_data_offset_x, new_data_offset_y, new_data_offset_z,
                                                 effective_brick_size_x, effective_brick_size_y, effective_brick_size_z,
                                                 cycle);

                brick_idx = (i*n_bricks_y + j)*n_bricks_z + k;
                Brick* const brick = bricks + brick_idx;

                brick->data = new_data + data_offset;
                brick->orientation = (enum brick_orientation)cycle;

                data_offset += padded_brick_length;
            }
        }
    }

    bricked_field->field = field;
    bricked_field->bricks = bricks;
    bricked_field->n_bricks_x = n_bricks_x;
    bricked_field->n_bricks_y = n_bricks_y;
    bricked_field->n_bricks_z = n_bricks_z;
    bricked_field->n_bricks = n_bricks;
    bricked_field->brick_size = brick_size;
    bricked_field->padded_brick_size = padded_brick_size;
}

void destroy_bricked_field(BrickedField* bricked_field)
{
    check(bricked_field);

    bricked_field->field = NULL;
    bricked_field->n_bricks = 0;

    if (!bricked_field->bricks)
        return;

    if (bricked_field->bricks[0].data)
        free(bricked_field->bricks[0].data);

    free(bricked_field->bricks);
    bricked_field->bricks = NULL;
}

static void copy_subarray_with_cycled_layout(const float* full_input_array,
                                             size_t full_input_size_y, size_t full_input_size_z,
                                             size_t input_offset_x, size_t input_offset_y, size_t input_offset_z,
                                             float* full_output_array,
                                             size_t full_output_size_x, size_t full_output_size_y, size_t full_output_size_z,
                                             size_t output_offset_x, size_t output_offset_y, size_t output_offset_z,
                                             size_t subsize_x, size_t subsize_y, size_t subsize_z,
                                             unsigned int cycle)
{
    check(full_input_array);
    check(full_output_array);
    check(full_output_array != full_input_array);
    check(cycle < 3);

    printf("\n\n%d, %d\n", full_input_size_y, full_input_size_z);
    printf("%d, %d, %d\n", input_offset_x, input_offset_y, input_offset_z);
    printf("%d, %d, %d\n", full_output_size_x, full_output_size_y, full_output_size_z);
    printf("%d, %d, %d\n", output_offset_x, output_offset_y, output_offset_z);
    printf("%d, %d, %d\n", subsize_x, subsize_y, subsize_z);
    printf("%d\n", cycle);

    const size_t input_offset = (input_offset_x*full_input_size_y + input_offset_y)*full_input_size_z + input_offset_z;
    const size_t output_offset = (output_offset_x*full_output_size_y + output_offset_y)*full_output_size_z + output_offset_z;

    size_t i, j, k;
    size_t input_idx, output_idx;

    if (cycle == 0)
    {
        for (i = 0; i < subsize_x; i++)
            for (j = 0; j < subsize_y; j++)
                for (k = 0; k < subsize_z; k++)
                {
                    input_idx = input_offset + (i*full_input_size_y + j)*full_input_size_z + k;
                    output_idx = output_offset + (i*full_output_size_y + j)*full_output_size_z + k;

                    full_output_array[output_idx] = full_input_array[input_idx];
                }
    }
    else if (cycle == 1)
    {
        for (i = 0; i < subsize_x; i++)
            for (j = 0; j < subsize_y; j++)
                for (k = 0; k < subsize_z; k++)
                {
                    input_idx = input_offset + (i*full_input_size_y + j)*full_input_size_z + k;
                    output_idx = output_offset + (k*full_output_size_x + i)*full_output_size_y + j;

                    full_output_array[output_idx] = full_input_array[input_idx];
                }
    }
    else
    {
        for (i = 0; i < subsize_x; i++)
            for (j = 0; j < subsize_y; j++)
                for (k = 0; k < subsize_z; k++)
                {
                    input_idx = input_offset + (i*full_input_size_y + j)*full_input_size_z + k;
                    output_idx = output_offset + (j*full_output_size_z + k)*full_output_size_x + i;

                    full_output_array[output_idx] = full_input_array[input_idx];
                }
    }
}
