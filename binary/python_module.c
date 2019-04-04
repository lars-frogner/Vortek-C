#include <Python.h>
#include <arrayobject.h>

#include "error.h"
#include "dynamic_string.h"
#include "fields.h"
#include "transformation.h"
#include "bricks.h"
#include "view_aligned_planes.h"
#include "field_textures.h"
#include "transfer_functions.h"
#include "renderer.h"
#include "window.h"


// Function declarations
static PyObject* vt_initialize(PyObject* self, PyObject* args);

static PyObject* vt_set_brick_size_power_of_two(PyObject* self, PyObject* args);
static PyObject* vt_set_minimum_sub_brick_size(PyObject* self, PyObject* args);

static PyObject* vt_add_bifrost_field(PyObject* self, PyObject* args);

static PyObject* vt_step(PyObject* self, PyObject* args);

static PyObject* vt_run(PyObject* self, PyObject* args);

static PyObject* vt_set_transfer_function_lower_limit(PyObject* self, PyObject* args);
static PyObject* vt_set_transfer_function_upper_limit(PyObject* self, PyObject* args);

static PyObject* vt_update_transfer_function_lower_node_red_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_lower_node_green_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_lower_node_blue_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_lower_node_alpha_value(PyObject* self, PyObject* args);

static PyObject* vt_update_transfer_function_upper_node_red_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_upper_node_green_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_upper_node_blue_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_upper_node_alpha_value(PyObject* self, PyObject* args);

static PyObject* vt_update_transfer_function_node_red_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_node_green_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_node_blue_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_node_alpha_value(PyObject* self, PyObject* args);

static PyObject* vt_reset_transfer_function_node_red_value(PyObject* self, PyObject* args);
static PyObject* vt_reset_transfer_function_node_green_value(PyObject* self, PyObject* args);
static PyObject* vt_reset_transfer_function_node_blue_value(PyObject* self, PyObject* args);
static PyObject* vt_reset_transfer_function_node_alpha_value(PyObject* self, PyObject* args);

static PyObject* vt_use_logarithmic_transfer_function_red_component(PyObject* self, PyObject* args);
static PyObject* vt_use_logarithmic_transfer_function_green_component(PyObject* self, PyObject* args);
static PyObject* vt_use_logarithmic_transfer_function_blue_component(PyObject* self, PyObject* args);
static PyObject* vt_use_logarithmic_transfer_function_alpha_component(PyObject* self, PyObject* args);

static PyObject* vt_reset_transfer_function_red_component(PyObject* self, PyObject* args);
static PyObject* vt_reset_transfer_function_green_component(PyObject* self, PyObject* args);
static PyObject* vt_reset_transfer_function_blue_component(PyObject* self, PyObject* args);
static PyObject* vt_reset_transfer_function_alpha_component(PyObject* self, PyObject* args);

static PyObject* vt_set_camera_field_of_view(PyObject* self, PyObject* args);
static PyObject* vt_set_clip_plane_distances(PyObject* self, PyObject* args);
static PyObject* vt_use_perspective_camera_projection(PyObject* self, PyObject* args);
static PyObject* vt_use_orthographic_camera_projection(PyObject* self, PyObject* args);

static PyObject* vt_set_lower_visibility_threshold(PyObject* self, PyObject* args);
static PyObject* vt_set_upper_visibility_threshold(PyObject* self, PyObject* args);

static PyObject* vt_set_field_boundary_indicator_creation(PyObject* self, PyObject* args);
static PyObject* vt_set_brick_boundary_indicator_creation(PyObject* self, PyObject* args);
static PyObject* vt_set_sub_brick_boundary_indicator_creation(PyObject* self, PyObject* args);

static PyObject* vt_bring_window_to_front(PyObject* self, PyObject* args);

static PyObject* vt_cleanup(PyObject* self, PyObject* args);


/*
Method definition objects for the extension. The entries are:
ml_name:  The name of the method.
ml_meth:  Function pointer to the method implementation.
ml_flags: Flags indicating special features of this method, such as
          accepting arguments, accepting keyword arguments, being a
          class method, or being a static method of a class.
ml_doc:   Contents of this method's docstring.
*/
static PyMethodDef vortek_method_definitions[] =
{
    {"initialize",                                        vt_initialize,                                        METH_VARARGS, NULL},
    {"set_brick_size_power_of_two",                       vt_set_brick_size_power_of_two,                       METH_VARARGS, NULL},
    {"set_minimum_sub_brick_size",                        vt_set_minimum_sub_brick_size,                        METH_VARARGS, NULL},
    {"add_bifrost_field",                                 vt_add_bifrost_field,                                 METH_VARARGS, NULL},
    {"step",                                              vt_step,                                              METH_VARARGS, NULL},
    {"run",                                               vt_run,                                               METH_VARARGS, NULL},
    {"set_transfer_function_lower_limit",                 vt_set_transfer_function_lower_limit,                 METH_VARARGS, NULL},
    {"set_transfer_function_upper_limit",                 vt_set_transfer_function_upper_limit,                 METH_VARARGS, NULL},
    {"update_transfer_function_lower_node_red_value",     vt_update_transfer_function_lower_node_red_value,     METH_VARARGS, NULL},
    {"update_transfer_function_lower_node_green_value",   vt_update_transfer_function_lower_node_green_value,   METH_VARARGS, NULL},
    {"update_transfer_function_lower_node_blue_value",    vt_update_transfer_function_lower_node_blue_value,    METH_VARARGS, NULL},
    {"update_transfer_function_lower_node_alpha_value",   vt_update_transfer_function_lower_node_alpha_value,   METH_VARARGS, NULL},
    {"update_transfer_function_upper_node_red_value",     vt_update_transfer_function_upper_node_red_value,     METH_VARARGS, NULL},
    {"update_transfer_function_upper_node_green_value",   vt_update_transfer_function_upper_node_green_value,   METH_VARARGS, NULL},
    {"update_transfer_function_upper_node_blue_value",    vt_update_transfer_function_upper_node_blue_value,    METH_VARARGS, NULL},
    {"update_transfer_function_upper_node_alpha_value",   vt_update_transfer_function_upper_node_alpha_value,   METH_VARARGS, NULL},
    {"update_transfer_function_node_red_value",           vt_update_transfer_function_node_red_value,           METH_VARARGS, NULL},
    {"update_transfer_function_node_green_value",         vt_update_transfer_function_node_green_value,         METH_VARARGS, NULL},
    {"update_transfer_function_node_blue_value",          vt_update_transfer_function_node_blue_value,          METH_VARARGS, NULL},
    {"update_transfer_function_node_alpha_value",         vt_update_transfer_function_node_alpha_value,         METH_VARARGS, NULL},
    {"reset_transfer_function_node_red_value",            vt_reset_transfer_function_node_red_value,            METH_VARARGS, NULL},
    {"reset_transfer_function_node_green_value",          vt_reset_transfer_function_node_green_value,          METH_VARARGS, NULL},
    {"reset_transfer_function_node_blue_value",           vt_reset_transfer_function_node_blue_value,           METH_VARARGS, NULL},
    {"reset_transfer_function_node_alpha_value",          vt_reset_transfer_function_node_alpha_value,          METH_VARARGS, NULL},
    {"use_logarithmic_transfer_function_red_component",   vt_use_logarithmic_transfer_function_red_component,   METH_VARARGS, NULL},
    {"use_logarithmic_transfer_function_green_component", vt_use_logarithmic_transfer_function_green_component, METH_VARARGS, NULL},
    {"use_logarithmic_transfer_function_blue_component",  vt_use_logarithmic_transfer_function_blue_component,  METH_VARARGS, NULL},
    {"use_logarithmic_transfer_function_alpha_component", vt_use_logarithmic_transfer_function_alpha_component, METH_VARARGS, NULL},
    {"reset_transfer_function_red_component",             vt_reset_transfer_function_red_component,             METH_VARARGS, NULL},
    {"reset_transfer_function_green_component",           vt_reset_transfer_function_green_component,           METH_VARARGS, NULL},
    {"reset_transfer_function_blue_component",            vt_reset_transfer_function_blue_component,            METH_VARARGS, NULL},
    {"reset_transfer_function_alpha_component",           vt_reset_transfer_function_alpha_component,           METH_VARARGS, NULL},
    {"set_camera_field_of_view",                          vt_set_camera_field_of_view,                          METH_VARARGS, NULL},
    {"set_clip_plane_distances",                          vt_set_clip_plane_distances,                          METH_VARARGS, NULL},
    {"use_perspective_camera_projection",                 vt_use_perspective_camera_projection,                 METH_VARARGS, NULL},
    {"use_orthographic_camera_projection",                vt_use_orthographic_camera_projection,                METH_VARARGS, NULL},
    {"set_lower_visibility_threshold",                    vt_set_lower_visibility_threshold,                    METH_VARARGS, NULL},
    {"set_upper_visibility_threshold",                    vt_set_upper_visibility_threshold,                    METH_VARARGS, NULL},
    {"set_field_boundary_indicator_creation",             vt_set_field_boundary_indicator_creation,             METH_VARARGS, NULL},
    {"set_brick_boundary_indicator_creation",             vt_set_brick_boundary_indicator_creation,             METH_VARARGS, NULL},
    {"set_sub_brick_boundary_indicator_creation",         vt_set_sub_brick_boundary_indicator_creation,         METH_VARARGS, NULL},
    {"bring_window_to_front",                             vt_bring_window_to_front,                             METH_VARARGS, NULL},
    {"cleanup",                                           vt_cleanup,                                           METH_VARARGS, NULL},
    {NULL,                                                NULL,                                                 0,            NULL} // Marks the end of the array
};

/*
Module definition:
The arguments of this structure tell Python what to call the extension,
what it's methods are and where to look for it's method definitions.
*/
static struct PyModuleDef vortek_module_definition =
{
    PyModuleDef_HEAD_INIT,     // m_base
    "vortek",                  // m_name
    NULL,                      // m_doc
    -1,                        // m_size
    vortek_method_definitions, // m_methods
    NULL,                      // m_slots
    NULL,                      // m_traverse
    NULL,                      // m_clear
    NULL                       // m_free
};

/*
Module initialization:
Python calls this function when importing the extension. It is important
that this function is named PyInit_<module name> exactly, and matches
the name keyword argument in setup.py's setup() call.
*/
PyMODINIT_FUNC PyInit_vortek(void)
{
    Py_Initialize();
    import_array();
    return PyModule_Create(&vortek_module_definition);
}

static PyObject* vt_initialize(PyObject* self, PyObject* args)
{
    // void vt_initialize(void);

    initialize_window();
    initialize_renderer();
    initialize_mainloop();
    Py_RETURN_NONE;
}

static PyObject* vt_set_brick_size_power_of_two(PyObject* self, PyObject* args)
{
    // void vt_set_brick_size_power_of_two(int brick_size_exponent);

    int brick_size_exponent;

    if (!PyArg_ParseTuple(args, "i", &brick_size_exponent))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_brick_size_power_of_two");

    if (brick_size_exponent < 0)
        print_severe_message("Brick size power of two must be non-negative.");

    set_brick_size_exponent((unsigned int)brick_size_exponent);

    Py_RETURN_NONE;
}

static PyObject* vt_set_minimum_sub_brick_size(PyObject* self, PyObject* args)
{
    // void vt_set_minimum_sub_brick_size(int min_sub_brick_size);

    int min_sub_brick_size;

    if (!PyArg_ParseTuple(args, "i", &min_sub_brick_size))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_minimum_sub_brick_size");

    if (min_sub_brick_size < 0)
        print_severe_message("Minimum sub brick size must be non-negative.");

    set_min_sub_brick_size((unsigned int)min_sub_brick_size);

    Py_RETURN_NONE;
}

static PyObject* vt_add_bifrost_field(PyObject* self, PyObject* args)
{
    // void vt_add_bifrost_field(char* field_name, char* file_base_name);

    char* field_name;
    char* file_base_name;

    if (!PyArg_ParseTuple(args, "ss", &field_name, &file_base_name))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_add_bifrost_field");

    DynamicString data_path = create_string("%s.raw", file_base_name);
    DynamicString header_path = create_string("%s.dat", file_base_name);

    set_single_field_rendering_field(create_field_from_bifrost_file(field_name, data_path.chars, header_path.chars));

    clear_string(&data_path);
    clear_string(&header_path);

    Py_RETURN_NONE;
}

static PyObject* vt_step(PyObject* self, PyObject* args)
{
    // void vt_step(void);

    if (step_mainloop())
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject* vt_run(PyObject* self, PyObject* args)
{
    // void vt_run(void);

    initialize_window();
    initialize_renderer();

    set_single_field_rendering_field(create_field_from_bifrost_file("temperature_field",
                                                                    "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg.raw",
                                                                    "/Users/larsfrog/Code/output_visualization/no_ebeam/en024031_emer3.0sml_orig_631_tg.dat"));

    mainloop();

    cleanup_renderer();
    cleanup_window();

    Py_RETURN_NONE;
}

static PyObject* vt_set_transfer_function_lower_limit(PyObject* self, PyObject* args)
{
    // void vt_set_transfer_function_lower_limit(float lower_limit);

    float lower_limit;

    if (!PyArg_ParseTuple(args, "f", &lower_limit))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_transfer_function_lower_limit");

    set_transfer_function_lower_limit(get_single_field_rendering_TF_name(),
                                      field_value_to_texture_value(get_single_field_rendering_texture_name(), lower_limit));

    Py_RETURN_NONE;
}

static PyObject* vt_set_transfer_function_upper_limit(PyObject* self, PyObject* args)
{
    // void vt_set_transfer_function_upper_limit(int node);

    float upper_limit;

    if (!PyArg_ParseTuple(args, "f", &upper_limit))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_transfer_function_upper_limit");

    set_transfer_function_upper_limit(get_single_field_rendering_TF_name(),
                                      field_value_to_texture_value(get_single_field_rendering_texture_name(), upper_limit));

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_lower_node_red_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_lower_node_red_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_lower_node_red_value");

    set_transfer_function_lower_node_value(get_single_field_rendering_TF_name(), TF_RED, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_lower_node_green_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_lower_node_green_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_lower_node_green_value");

    set_transfer_function_lower_node_value(get_single_field_rendering_TF_name(), TF_GREEN, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_lower_node_blue_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_lower_node_blue_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_lower_node_blue_value");

    set_transfer_function_lower_node_value(get_single_field_rendering_TF_name(), TF_BLUE, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_lower_node_alpha_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_lower_node_alpha_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_lower_node_alpha_value");

    set_transfer_function_lower_node_value(get_single_field_rendering_TF_name(), TF_ALPHA, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_upper_node_red_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_upper_node_red_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_upper_node_red_value");

    set_transfer_function_upper_node_value(get_single_field_rendering_TF_name(), TF_RED, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_upper_node_green_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_upper_node_green_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_upper_node_green_value");

    set_transfer_function_upper_node_value(get_single_field_rendering_TF_name(), TF_GREEN, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_upper_node_blue_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_upper_node_blue_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_upper_node_blue_value");

    set_transfer_function_upper_node_value(get_single_field_rendering_TF_name(), TF_BLUE, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_upper_node_alpha_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_upper_node_alpha_value(float value);

    float value;

    if (!PyArg_ParseTuple(args, "f", &value))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_update_transfer_function_upper_node_alpha_value");

    set_transfer_function_upper_node_value(get_single_field_rendering_TF_name(), TF_ALPHA, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_node_red_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_node_red_value(int node, float value);

    int node;
    float value;

    if (!PyArg_ParseTuple(args, "if", &node, &value))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_update_transfer_function_node_red_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    set_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                TF_RED, (unsigned int)node, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_node_green_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_node_green_value(int node, float value);

    int node;
    float value;

    if (!PyArg_ParseTuple(args, "if", &node, &value))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_update_transfer_function_node_green_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    set_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                TF_GREEN, (unsigned int)node, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_node_blue_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_node_blue_value(int node, float value);

    int node;
    float value;

    if (!PyArg_ParseTuple(args, "if", &node, &value))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_update_transfer_function_node_blue_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    set_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                TF_BLUE, (unsigned int)node, value);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_node_alpha_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_node_alpha_value(int node, float value);

    int node;
    float value;

    if (!PyArg_ParseTuple(args, "if", &node, &value))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_update_transfer_function_node_alpha_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    set_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                TF_ALPHA, (unsigned int)node, value);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_node_red_value(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_node_red_value(int node);

    int node;

    if (!PyArg_ParseTuple(args, "i", &node))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_reset_transfer_function_node_red_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    remove_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                   TF_RED, (unsigned int)node);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_node_green_value(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_node_green_value(int node);

    int node;

    if (!PyArg_ParseTuple(args, "i", &node))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_reset_transfer_function_node_green_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    remove_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                   TF_GREEN, (unsigned int)node);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_node_blue_value(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_node_blue_value(int node);

    int node;

    if (!PyArg_ParseTuple(args, "i", &node))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_reset_transfer_function_node_blue_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    remove_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                   TF_BLUE, (unsigned int)node);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_node_alpha_value(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_node_alpha_value(int node);

    int node;

    if (!PyArg_ParseTuple(args, "i", &node))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_reset_transfer_function_node_alpha_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    remove_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                   TF_ALPHA, (unsigned int)node);

    Py_RETURN_NONE;
}

static PyObject* vt_use_logarithmic_transfer_function_red_component(PyObject* self, PyObject* args)
{
    // void vt_use_logarithmic_transfer_function_red_component(void);

    set_logarithmic_transfer_function(get_single_field_rendering_TF_name(), TF_RED, 0.0f, 1.0f);

    Py_RETURN_NONE;
}

static PyObject* vt_use_logarithmic_transfer_function_green_component(PyObject* self, PyObject* args)
{
    // void vt_use_logarithmic_transfer_function_green_component(void);

    set_logarithmic_transfer_function(get_single_field_rendering_TF_name(), TF_GREEN, 0.0f, 1.0f);

    Py_RETURN_NONE;
}

static PyObject* vt_use_logarithmic_transfer_function_blue_component(PyObject* self, PyObject* args)
{
    // void vt_use_logarithmic_transfer_function_blue_component(void);

    set_logarithmic_transfer_function(get_single_field_rendering_TF_name(), TF_BLUE, 0.0f, 1.0f);

    Py_RETURN_NONE;
}

static PyObject* vt_use_logarithmic_transfer_function_alpha_component(PyObject* self, PyObject* args)
{
    // void vt_use_logarithmic_transfer_function_alpha_component(void);

    set_logarithmic_transfer_function(get_single_field_rendering_TF_name(), TF_ALPHA, 0.0f, 1.0f);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_red_component(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_red_component(void);

    reset_transfer_function(get_single_field_rendering_TF_name(), TF_RED);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_green_component(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_green_component(void);

    reset_transfer_function(get_single_field_rendering_TF_name(), TF_GREEN);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_blue_component(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_blue_component(void);

    reset_transfer_function(get_single_field_rendering_TF_name(), TF_BLUE);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_alpha_component(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_alpha_component(void);

    reset_transfer_function(get_single_field_rendering_TF_name(), TF_ALPHA);

    Py_RETURN_NONE;
}

static PyObject* vt_set_camera_field_of_view(PyObject* self, PyObject* args)
{
    // void vt_set_camera_field_of_view(float value);

    float field_of_view;

    if (!PyArg_ParseTuple(args, "f", &field_of_view))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_camera_field_of_view");

    update_camera_field_of_view(field_of_view);

    Py_RETURN_NONE;
}

static PyObject* vt_set_clip_plane_distances(PyObject* self, PyObject* args)
{
    // void vt_set_clip_plane_distances(float value);

    float near_plane_distance;
    float far_plane_distance;

    if (!PyArg_ParseTuple(args, "ff", &near_plane_distance, &far_plane_distance))
        print_severe_message("Could not parse arguments to function \"%s\".", "vt_set_clip_plane_distances");

    update_camera_clip_plane_distances(near_plane_distance, far_plane_distance);

    Py_RETURN_NONE;
}

static PyObject* vt_use_perspective_camera_projection(PyObject* self, PyObject* args)
{
    // void vt_use_perspective_camera_projection(void);

    update_camera_projection_type(PERSPECTIVE_PROJECTION);

    Py_RETURN_NONE;
}

static PyObject* vt_use_orthographic_camera_projection(PyObject* self, PyObject* args)
{
    // void vt_use_orthographic_camera_projection(void);

    update_camera_projection_type(ORTHOGRAPHIC_PROJECTION);

    Py_RETURN_NONE;
}

static PyObject* vt_set_lower_visibility_threshold(PyObject* self, PyObject* args)
{
    // void vt_set_lower_visibility_threshold(float threshold);

    float threshold;

    if (!PyArg_ParseTuple(args, "f", &threshold))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_lower_visibility_threshold");

    set_lower_visibility_threshold(threshold);

    Py_RETURN_NONE;
}

static PyObject* vt_set_upper_visibility_threshold(PyObject* self, PyObject* args)
{
    // void vt_set_upper_visibility_threshold(float threshold);

    float threshold;

    if (!PyArg_ParseTuple(args, "f", &threshold))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_upper_visibility_threshold");

    set_upper_visibility_threshold(threshold);

    Py_RETURN_NONE;
}

static PyObject* vt_set_field_boundary_indicator_creation(PyObject* self, PyObject* args)
{
    // void vt_set_field_boundary_indicator_creation(int state);

    int state;

    if (!PyArg_ParseTuple(args, "i", &state))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_field_boundary_indicator_creation");

    if (state != 0 || state != 1)
        print_severe_message("Argument to function \"%s\" must be either 0 or 1.", "vt_set_field_boundary_indicator_creation");

    set_field_boundary_indicator_creation(state);

    Py_RETURN_NONE;
}

static PyObject* vt_set_brick_boundary_indicator_creation(PyObject* self, PyObject* args)
{
    // void vt_set_brick_boundary_indicator_creation(int state);

    int state;

    if (!PyArg_ParseTuple(args, "i", &state))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_brick_boundary_indicator_creation");

    if (state != 0 || state != 1)
        print_severe_message("Argument to function \"%s\" must be either 0 or 1.", "vt_set_brick_boundary_indicator_creation");

    set_brick_boundary_indicator_creation(state);

    Py_RETURN_NONE;
}

static PyObject* vt_set_sub_brick_boundary_indicator_creation(PyObject* self, PyObject* args)
{
    // void vt_set_sub_brick_boundary_indicator_creation(int state);

    int state;

    if (!PyArg_ParseTuple(args, "i", &state))
        print_severe_message("Could not parse argument to function \"%s\".", "vt_set_sub_brick_boundary_indicator_creation");

    if (state != 0 || state != 1)
        print_severe_message("Argument to function \"%s\" must be either 0 or 1.", "vt_set_sub_brick_boundary_indicator_creation");

    set_sub_brick_boundary_indicator_creation(state);

    Py_RETURN_NONE;
}

static PyObject* vt_bring_window_to_front(PyObject* self, PyObject* args)
{
    // void vt_bring_window_to_front(void);

    focus_window();
    Py_RETURN_NONE;
}

static PyObject* vt_cleanup(PyObject* self, PyObject* args)
{
    // void vt_cleanup(void);

    cleanup_renderer();
    cleanup_window();
    Py_RETURN_NONE;
}
