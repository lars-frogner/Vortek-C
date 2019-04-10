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
static PyObject* vt_is_initialized(PyObject* self, PyObject* args);

static PyObject* vt_initialize(PyObject* self, PyObject* args);

static PyObject* vt_set_brick_size_power_of_two(PyObject* self, PyObject* args);
static PyObject* vt_set_minimum_sub_brick_size(PyObject* self, PyObject* args);

static PyObject* vt_set_field_from_bifrost_file(PyObject* self, PyObject* args);

static PyObject* vt_step(PyObject* self, PyObject* args);

static PyObject* vt_refresh_visibility(PyObject* self, PyObject* args);
static PyObject* vt_refresh_frame(PyObject* self, PyObject* args);

static PyObject* vt_enable_autorefresh(PyObject* self, PyObject* args);
static PyObject* vt_disable_autorefresh(PyObject* self, PyObject* args);

static PyObject* vt_set_transfer_function_lower_limit(PyObject* self, PyObject* args);
static PyObject* vt_set_transfer_function_upper_limit(PyObject* self, PyObject* args);

static PyObject* vt_update_transfer_function_lower_node_value(PyObject* self, PyObject* args);
static PyObject* vt_update_transfer_function_upper_node_value(PyObject* self, PyObject* args);

static PyObject* vt_update_transfer_function_node_value(PyObject* self, PyObject* args);

static PyObject* vt_remove_transfer_function_node(PyObject* self, PyObject* args);

static PyObject* vt_use_logarithmic_transfer_function_component(PyObject* self, PyObject* args);

static PyObject* vt_set_custom_transfer_function_component(PyObject* self, PyObject* args);

static PyObject* vt_reset_transfer_function_component(PyObject* self, PyObject* args);

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


static void maybe_refresh(int include_visibility);


static int is_initialized = 0;
static int autorefresh = 1;


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
    {"is_initialized",                                    vt_is_initialized,                               METH_VARARGS, NULL},
    {"initialize",                                        vt_initialize,                                   METH_VARARGS, NULL},
    {"set_brick_size_power_of_two",                       vt_set_brick_size_power_of_two,                  METH_VARARGS, NULL},
    {"set_minimum_sub_brick_size",                        vt_set_minimum_sub_brick_size,                   METH_VARARGS, NULL},
    {"set_field_from_bifrost_file",                       vt_set_field_from_bifrost_file,                  METH_VARARGS, NULL},
    {"step",                                              vt_step,                                         METH_VARARGS, NULL},
    {"refresh_visibility",                                vt_refresh_visibility,                           METH_VARARGS, NULL},
    {"refresh_frame",                                     vt_refresh_frame,                                METH_VARARGS, NULL},
    {"enable_autorefresh",                                vt_enable_autorefresh,                           METH_VARARGS, NULL},
    {"disable_autorefresh",                               vt_disable_autorefresh,                          METH_VARARGS, NULL},
    {"set_transfer_function_lower_limit",                 vt_set_transfer_function_lower_limit,            METH_VARARGS, NULL},
    {"set_transfer_function_upper_limit",                 vt_set_transfer_function_upper_limit,            METH_VARARGS, NULL},
    {"update_transfer_function_lower_node_value",         vt_update_transfer_function_lower_node_value,    METH_VARARGS, NULL},
    {"update_transfer_function_upper_node_value",         vt_update_transfer_function_upper_node_value,    METH_VARARGS, NULL},
    {"update_transfer_function_node_value",               vt_update_transfer_function_node_value,          METH_VARARGS, NULL},
    {"remove_transfer_function_node",                     vt_remove_transfer_function_node,                METH_VARARGS, NULL},
    {"use_logarithmic_transfer_function_component",       vt_use_logarithmic_transfer_function_component,  METH_VARARGS, NULL},
    {"set_custom_transfer_function_component",            vt_set_custom_transfer_function_component,       METH_VARARGS, NULL},
    {"reset_transfer_function_component",                 vt_reset_transfer_function_component,            METH_VARARGS, NULL},
    {"set_camera_field_of_view",                          vt_set_camera_field_of_view,                     METH_VARARGS, NULL},
    {"set_clip_plane_distances",                          vt_set_clip_plane_distances,                     METH_VARARGS, NULL},
    {"use_perspective_camera_projection",                 vt_use_perspective_camera_projection,            METH_VARARGS, NULL},
    {"use_orthographic_camera_projection",                vt_use_orthographic_camera_projection,           METH_VARARGS, NULL},
    {"set_lower_visibility_threshold",                    vt_set_lower_visibility_threshold,               METH_VARARGS, NULL},
    {"set_upper_visibility_threshold",                    vt_set_upper_visibility_threshold,               METH_VARARGS, NULL},
    {"set_field_boundary_indicator_creation",             vt_set_field_boundary_indicator_creation,        METH_VARARGS, NULL},
    {"set_brick_boundary_indicator_creation",             vt_set_brick_boundary_indicator_creation,        METH_VARARGS, NULL},
    {"set_sub_brick_boundary_indicator_creation",         vt_set_sub_brick_boundary_indicator_creation,    METH_VARARGS, NULL},
    {"bring_window_to_front",                             vt_bring_window_to_front,                        METH_VARARGS, NULL},
    {"cleanup",                                           vt_cleanup,                                      METH_VARARGS, NULL},
    {NULL,                                                NULL,                                            0,            NULL} // Marks the end of the array
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

static PyObject* vt_is_initialized(PyObject* self, PyObject* args)
{
    // void vt_is_initialized(void);

    return Py_BuildValue("i", is_initialized);
}

static PyObject* vt_initialize(PyObject* self, PyObject* args)
{
    // void vt_initialize(void);

    initialize_window();
    initialize_renderer();
    initialize_mainloop();
    is_initialized = 1;
    Py_RETURN_NONE;
}

static PyObject* vt_set_brick_size_power_of_two(PyObject* self, PyObject* args)
{
    // void vt_set_brick_size_power_of_two(int brick_size_exponent);

    int brick_size_exponent;

    if (!PyArg_ParseTuple(args, "i", &brick_size_exponent))
        print_severe_message("Could not parse argument to function \"%s\".", "set_brick_size_power_of_two");

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
        print_severe_message("Could not parse argument to function \"%s\".", "set_minimum_sub_brick_size");

    if (min_sub_brick_size < 0)
        print_severe_message("Minimum sub brick size must be non-negative.");

    set_min_sub_brick_size((unsigned int)min_sub_brick_size);

    Py_RETURN_NONE;
}

static PyObject* vt_set_field_from_bifrost_file(PyObject* self, PyObject* args)
{
    // void vt_set_field_from_bifrost_file(char* field_name, char* file_base_name);

    char* field_name;
    char* file_base_name;

    if (!PyArg_ParseTuple(args, "ss", &field_name, &file_base_name))
        print_severe_message("Could not parse arguments to function \"%s\".", "add_bifrost_field");

    DynamicString data_path = create_string("%s.raw", file_base_name);
    DynamicString header_path = create_string("%s.dat", file_base_name);

    Field* const existing_field = get_field_texture_field(get_single_field_rendering_texture_name());
    if (existing_field)
        destroy_field(existing_field->name.chars);

    set_single_field_rendering_field(create_field_from_bifrost_file(field_name, data_path.chars, header_path.chars));

    clear_string(&data_path);
    clear_string(&header_path);

    update_visibility_ratios(get_single_field_rendering_TF_name(),
                             get_field_texture_bricked_field(get_single_field_rendering_texture_name()));
    require_rendering();

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

static PyObject* vt_refresh_visibility(PyObject* self, PyObject* args)
{
    // void vt_refresh_visibility(void);

    update_visibility_ratios(get_single_field_rendering_TF_name(),
                             get_field_texture_bricked_field(get_single_field_rendering_texture_name()));
    Py_RETURN_NONE;
}

static PyObject* vt_refresh_frame(PyObject* self, PyObject* args)
{
    // void vt_refresh_frame(void);

    require_rendering();
    Py_RETURN_NONE;
}

static PyObject* vt_enable_autorefresh(PyObject* self, PyObject* args)
{
    // void vt_enable_autorefresh(void);

    autorefresh = 1;
    Py_RETURN_NONE;
}

static PyObject* vt_disable_autorefresh(PyObject* self, PyObject* args)
{
    // void vt_disable_autorefresh(void);

    autorefresh = 0;
    Py_RETURN_NONE;
}

static PyObject* vt_set_transfer_function_lower_limit(PyObject* self, PyObject* args)
{
    // void vt_set_transfer_function_lower_limit(float lower_limit);

    float lower_limit;

    if (!PyArg_ParseTuple(args, "f", &lower_limit))
        print_severe_message("Could not parse argument to function \"%s\".", "set_transfer_function_lower_limit");

    set_transfer_function_lower_limit(get_single_field_rendering_TF_name(),
                                      field_value_to_texture_value(get_single_field_rendering_texture_name(), lower_limit));

    maybe_refresh(1);

    Py_RETURN_NONE;
}

static PyObject* vt_set_transfer_function_upper_limit(PyObject* self, PyObject* args)
{
    // void vt_set_transfer_function_upper_limit(float upper_limit);

    float upper_limit;

    if (!PyArg_ParseTuple(args, "f", &upper_limit))
        print_severe_message("Could not parse argument to function \"%s\".", "set_transfer_function_upper_limit");

    set_transfer_function_upper_limit(get_single_field_rendering_TF_name(),
                                      field_value_to_texture_value(get_single_field_rendering_texture_name(), upper_limit));

    maybe_refresh(1);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_lower_node_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_lower_node_value(int component, float value);

    int component;
    float value;

    if (!PyArg_ParseTuple(args, "if", &component, &value))
        print_severe_message("Could not parse arguments to function \"%s\".", "update_transfer_function_lower_node_value");

    if (component < 0 || component > 3)
        print_severe_message("Transfer function component specifier must be an integer in the range [0, 3].");

    set_transfer_function_lower_node_value(get_single_field_rendering_TF_name(), (enum transfer_function_component)component, value);

    maybe_refresh(component == 3);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_upper_node_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_upper_node_value(int component, float value);

    int component;
    float value;

    if (!PyArg_ParseTuple(args, "if", &component, &value))
        print_severe_message("Could not parse arguments to function \"%s\".", "update_transfer_function_upper_node_value");

    if (component < 0 || component > 3)
        print_severe_message("Transfer function component specifier must be an integer in the range [0, 3].");

    set_transfer_function_upper_node_value(get_single_field_rendering_TF_name(), (enum transfer_function_component)component, value);

    maybe_refresh(component == 3);

    Py_RETURN_NONE;
}

static PyObject* vt_update_transfer_function_node_value(PyObject* self, PyObject* args)
{
    // void vt_update_transfer_function_node_value(int component, int node, float value);

    int component;
    int node;
    float value;

    if (!PyArg_ParseTuple(args, "iif", &component, &node, &value))
        print_severe_message("Could not parse arguments to function \"%s\".", "update_transfer_function_node_value");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    if (component < 0 || component > 3)
        print_severe_message("Transfer function component specifier must be an integer in the range [0, 3].");

    set_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                (enum transfer_function_component)component,
                                                (unsigned int)node, value);

    maybe_refresh(component == 3);

    Py_RETURN_NONE;
}

static PyObject* vt_remove_transfer_function_node(PyObject* self, PyObject* args)
{
    // void vt_remove_transfer_function_node(int component, int node);

    int component;
    int node;

    if (!PyArg_ParseTuple(args, "ii", &component, &node))
        print_severe_message("Could not parse arguments to function \"%s\".", "remove_transfer_function_node");

    if (node < TF_START_NODE || node > TF_END_NODE)
        print_severe_message("Transfer function node specifier must be an integer in the range [%d, %d].", TF_START_NODE, TF_END_NODE);

    if (component < 0 || component > 3)
        print_severe_message("Transfer function component specifier must be an integer in the range [0, 3].");

    remove_piecewise_linear_transfer_function_node(get_single_field_rendering_TF_name(),
                                                   (enum transfer_function_component)component,
                                                   (unsigned int)node);

    maybe_refresh(component == 3);

    Py_RETURN_NONE;
}

static PyObject* vt_use_logarithmic_transfer_function_component(PyObject* self, PyObject* args)
{
    // void vt_use_logarithmic_transfer_function_component(int component);

    int component;

    if (!PyArg_ParseTuple(args, "i", &component))
        print_severe_message("Could not parse argument to function \"%s\".", "use_logarithmic_transfer_function_component");

    if (component < 0 || component > 3)
        print_severe_message("Transfer function component specifier must be an integer in the range [0, 3].");

    set_logarithmic_transfer_function(get_single_field_rendering_TF_name(),
                                      (enum transfer_function_component)component,
                                      0.0f, 1.0f);

    maybe_refresh(component == 3);

    Py_RETURN_NONE;
}

static PyObject* vt_set_custom_transfer_function_component(PyObject* self, PyObject* args)
{
    // void vt_set_custom_transfer_function_component(int component, ndarray values);

    int component;
    PyArrayObject* values;

    if (!PyArg_ParseTuple(args, "iO!", &component, &PyArray_Type, &values))
        print_severe_message("Could not parse arguments to function \"%s\".", "set_custom_transfer_function_component");

    if (component < 0 || component > 3)
        print_severe_message("Transfer function component specifier must be an integer in the range [0, 3].");

    if (values->dimensions[0] != TF_NUMBER_OF_INTERIOR_NODES)
        print_severe_message("Transfer function custom values array must have length %d.", TF_NUMBER_OF_INTERIOR_NODES);

    set_custom_transfer_function(get_single_field_rendering_TF_name(),
                                 (enum transfer_function_component)component,
                                 (const float*)values->data);

    maybe_refresh(component == 3);

    Py_RETURN_NONE;
}

static PyObject* vt_reset_transfer_function_component(PyObject* self, PyObject* args)
{
    // void vt_reset_transfer_function_component(int component);

    int component;

    if (!PyArg_ParseTuple(args, "i", &component))
        print_severe_message("Could not parse argument to function \"%s\".", "reset_transfer_function_component");

    if (component < 0 || component > 3)
        print_severe_message("Transfer function component specifier must be an integer in the range [0, 3].");

    reset_transfer_function(get_single_field_rendering_TF_name(),
                            (enum transfer_function_component)component);

    maybe_refresh(component == 3);

    Py_RETURN_NONE;
}

static PyObject* vt_set_camera_field_of_view(PyObject* self, PyObject* args)
{
    // void vt_set_camera_field_of_view(float field_of_view);

    float field_of_view;

    if (!PyArg_ParseTuple(args, "f", &field_of_view))
        print_severe_message("Could not parse argument to function \"%s\".", "set_camera_field_of_view");

    update_camera_field_of_view(field_of_view);

    maybe_refresh(0);

    Py_RETURN_NONE;
}

static PyObject* vt_set_clip_plane_distances(PyObject* self, PyObject* args)
{
    // void vt_set_clip_plane_distances(float near_plane_distance, float far_plane_distance);

    float near_plane_distance;
    float far_plane_distance;

    if (!PyArg_ParseTuple(args, "ff", &near_plane_distance, &far_plane_distance))
        print_severe_message("Could not parse arguments to function \"%s\".", "set_clip_plane_distances");

    update_camera_clip_plane_distances(near_plane_distance, far_plane_distance);

    maybe_refresh(0);

    Py_RETURN_NONE;
}

static PyObject* vt_use_perspective_camera_projection(PyObject* self, PyObject* args)
{
    // void vt_use_perspective_camera_projection(void);

    update_camera_projection_type(PERSPECTIVE_PROJECTION);

    maybe_refresh(0);

    Py_RETURN_NONE;
}

static PyObject* vt_use_orthographic_camera_projection(PyObject* self, PyObject* args)
{
    // void vt_use_orthographic_camera_projection(void);

    update_camera_projection_type(ORTHOGRAPHIC_PROJECTION);

    maybe_refresh(0);

    Py_RETURN_NONE;
}

static PyObject* vt_set_lower_visibility_threshold(PyObject* self, PyObject* args)
{
    // void vt_set_lower_visibility_threshold(float threshold);

    float threshold;

    if (!PyArg_ParseTuple(args, "f", &threshold))
        print_severe_message("Could not parse argument to function \"%s\".", "set_lower_visibility_threshold");

    set_lower_visibility_threshold(threshold);

    maybe_refresh(0);

    Py_RETURN_NONE;
}

static PyObject* vt_set_upper_visibility_threshold(PyObject* self, PyObject* args)
{
    // void vt_set_upper_visibility_threshold(float threshold);

    float threshold;

    if (!PyArg_ParseTuple(args, "f", &threshold))
        print_severe_message("Could not parse argument to function \"%s\".", "set_upper_visibility_threshold");

    set_upper_visibility_threshold(threshold);

    maybe_refresh(0);

    Py_RETURN_NONE;
}

static PyObject* vt_set_field_boundary_indicator_creation(PyObject* self, PyObject* args)
{
    // void vt_set_field_boundary_indicator_creation(int state);

    int state;

    if (!PyArg_ParseTuple(args, "i", &state))
        print_severe_message("Could not parse argument to function \"%s\".", "set_field_boundary_indicator_creation");

    if (state != 0 || state != 1)
        print_severe_message("Argument to function \"%s\" must be either 0 or 1.", "set_field_boundary_indicator_creation");

    set_field_boundary_indicator_creation(state);

    Py_RETURN_NONE;
}

static PyObject* vt_set_brick_boundary_indicator_creation(PyObject* self, PyObject* args)
{
    // void vt_set_brick_boundary_indicator_creation(int state);

    int state;

    if (!PyArg_ParseTuple(args, "i", &state))
        print_severe_message("Could not parse argument to function \"%s\".", "set_brick_boundary_indicator_creation");

    if (state != 0 || state != 1)
        print_severe_message("Argument to function \"%s\" must be either 0 or 1.", "set_brick_boundary_indicator_creation");

    set_brick_boundary_indicator_creation(state);

    Py_RETURN_NONE;
}

static PyObject* vt_set_sub_brick_boundary_indicator_creation(PyObject* self, PyObject* args)
{
    // void vt_set_sub_brick_boundary_indicator_creation(int state);

    int state;

    if (!PyArg_ParseTuple(args, "i", &state))
        print_severe_message("Could not parse argument to function \"%s\".", "set_sub_brick_boundary_indicator_creation");

    if (state != 0 || state != 1)
        print_severe_message("Argument to function \"%s\" must be either 0 or 1.", "set_sub_brick_boundary_indicator_creation");

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
    is_initialized = 0;
    Py_RETURN_NONE;
}

static void maybe_refresh(int include_visibility)
{
    if (autorefresh)
    {
        if (include_visibility)
            update_visibility_ratios(get_single_field_rendering_TF_name(),
                                     get_field_texture_bricked_field(get_single_field_rendering_texture_name()));

        require_rendering();
    }
}
