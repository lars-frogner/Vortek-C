#include "shader_generator.h"

#include "error.h"

#include <stdio.h>
#include <string.h>

#define MAX_GLOBAL_VARIABLE_NAME_SIZE 20


typedef struct Variable
{
    size_t number;
    DynamicString expression;
    LinkedList dependencies;
    int is_written;
    int is_deleted;
} Variable;

typedef struct Dependency
{
    size_t variable_number;
    char global_variable_name[MAX_GLOBAL_VARIABLE_NAME_SIZE];
    int is_global;
} Dependency;


static void find_directly_dependent_variables(LinkedList* directly_dependent_variables, ShaderSource* source, size_t variable_number);
static void find_required_global_variables(HashMap* required_global_variables, ShaderSource* source);
static void clear_source_code(ShaderSource* source);
static void write_required_global_variable_expressions(ShaderSource* source);
static void write_required_variable_expressions(ShaderSource* source, size_t variable_number);

static void add_sampler3D_uniform(ShaderSource* source, const char* name);
static void add_sampler1D_uniform(ShaderSource* source, const char* name);
static void add_output(ShaderSource* source, const char* type, const char* name);

static Variable* create_variable(ShaderSource* source);
static void delete_variable(ShaderSource* source, size_t variable_number);

static void add_global_dependency(Variable* variable, const char* global_variable_name);
static void add_variable_dependency(Variable* variable, size_t variable_number);

static Variable* get_variable(LinkedList* variables, size_t variable_number);
static Variable* get_current_variable(LinkedList* variables);
static Dependency* get_current_dependency(LinkedList* dependencies);


void initialize_shader_source(ShaderSource* source)
{
    check(source);
    source->code = create_empty_string();
    source->global_variable_expressions = create_map();
    source->variables = create_list();
    source->deleted_variables = create_list();
    source->output_variables = create_list();
    source->next_undeleted_unused_variable_number = 0;
}

void add_input_in_shader(ShaderSource* source, const char* type, const char* name)
{
    check(source);
    check(type);
    check(name);
    insert_string_in_map(&source->global_variable_expressions, name, "in %s %s;\n", type, name);
}

void add_vertex_input_in_shader(ShaderSource* source, const char* type, const char* name, unsigned int layout_location)
{
    check(source);
    check(type);
    check(name);
    insert_string_in_map(&source->global_variable_expressions, name, "layout(location=%d) in %s %s;\n", layout_location, type, name);
}

void add_uniform_in_shader(ShaderSource* source, const char* type, const char* name)
{
    check(source);
    check(type);
    check(name);
    insert_string_in_map(&source->global_variable_expressions, name, "uniform %s %s;\n", type, name);
}

void add_array_uniform_in_shader(ShaderSource* source, const char* type, const char* name, size_t length)
{
    check(source);
    check(type);
    check(name);
    insert_string_in_map(&source->global_variable_expressions, name, "uniform %s %s[%d];\n", type, name, length);
}

size_t transform_input_in_shader(ShaderSource* source, const char* matrix_name, const char* input_name)
{
    check(source);
    check(input_name);
    check(matrix_name);

    Variable* const variable = create_variable(source);

    set_string(&variable->expression, "    vec4 variable_%d = %s*%s;\n", variable->number, matrix_name, input_name);

    add_global_dependency(variable, input_name);
    add_global_dependency(variable, matrix_name);

    return variable->number;
}

void add_field_texture_in_shader(ShaderSource* source, const char* texture_name)
{
    add_sampler3D_uniform(source, texture_name);
}

size_t apply_scalar_field_texture_sampling_in_shader(ShaderSource* source, const char* texture_name, const char* texture_coordinates_name)
{
    check(source);
    check(texture_name);

    Variable* const variable = create_variable(source);

    set_string(&variable->expression, "    float variable_%d = texture(%s, %s).r;\n", variable->number, texture_name, texture_coordinates_name);

    add_global_dependency(variable, texture_name);
    add_global_dependency(variable, texture_coordinates_name);

    return variable->number;
}

void add_transfer_function_in_shader(ShaderSource* source, const char* transfer_function_name)
{
    add_sampler1D_uniform(source, transfer_function_name);
}

size_t apply_transfer_function_in_shader(ShaderSource* source, const char* transfer_function_name, size_t input_variable_number)
{
    check(source);
    check(transfer_function_name);
    check(input_variable_number < source->variables.length);

    Variable* const variable = create_variable(source);

    set_string(&variable->expression, "    vec4 variable_%d = texture(%s, variable_%d);\n", variable->number, transfer_function_name, input_variable_number);

    add_global_dependency(variable, transfer_function_name);
    add_variable_dependency(variable, input_variable_number);

    return variable->number;
}

size_t add_snippet_in_shader(ShaderSource* source, const char* output_type, const char* output_name, const char* snippet, LinkedList* global_dependencies, LinkedList* variable_dependencies)
{
    check(source);
    check(output_type);
    check(output_name);
    check(snippet);

    Variable* const variable = create_variable(source);

    set_string(&variable->expression, "%s\n    %s variable_%d = %s;\n", snippet, output_type, variable->number, output_name);

    if (global_dependencies)
    {
        for (reset_list_iterator(global_dependencies); valid_list_iterator(global_dependencies); advance_list_iterator(global_dependencies))
        {
            add_global_dependency(variable, get_current_string_from_list(global_dependencies));
        }
    }

    if (variable_dependencies)
    {
        for (reset_list_iterator(variable_dependencies); valid_list_iterator(variable_dependencies); advance_list_iterator(variable_dependencies))
        {
            add_variable_dependency(variable, get_current_size_t_from_list(variable_dependencies));
        }
    }

    return variable->number;
}

void assign_variable_to_output_in_shader(ShaderSource* source, size_t variable_number, const char* output_name)
{
    check(source);
    check(output_name);
    check(variable_number < source->variables.length);

    Variable* const variable = get_variable(&source->variables, variable_number);
    check(!variable->is_deleted);

    extend_string(&variable->expression, "    %s = variable_%d;\n", output_name, variable_number);

    append_size_t_to_list(&source->output_variables, variable_number);
}

void assign_transformed_variable_to_output_in_shader(ShaderSource* source, const char* matrix_name, size_t variable_number, const char* output_name)
{
    check(source);
    check(matrix_name);
    check(output_name);
    check(variable_number < source->variables.length);

    Variable* const variable = get_variable(&source->variables, variable_number);
    check(!variable->is_deleted);

    extend_string(&variable->expression, "    %s = %s*variable_%d;\n", output_name, matrix_name, variable_number);

    add_global_dependency(variable, matrix_name);

    append_size_t_to_list(&source->output_variables, variable_number);
}

void assign_variable_to_new_output_in_shader(ShaderSource* source, const char* type, size_t variable_number, const char* output_name)
{
    check(source);
    check(type);
    check(output_name);
    check(variable_number < source->variables.length);

    add_output(source, type, output_name);

    Variable* const variable = get_variable(&source->variables, variable_number);
    check(!variable->is_deleted);

    extend_string(&variable->expression, "    %s = variable_%d;\n", output_name, variable_number);

    add_global_dependency(variable, output_name);

    append_size_t_to_list(&source->output_variables, variable_number);
}

void assign_input_to_new_output_in_shader(ShaderSource* source, const char* type, const char* input_name, const char* output_name)
{
    check(source);
    check(type);
    check(input_name);
    check(output_name);

    add_output(source, type, output_name);

    Variable* const variable = create_variable(source);

    set_string(&variable->expression, "    %s = %s;\n", output_name, input_name);

    add_global_dependency(variable, output_name);
    add_global_dependency(variable, input_name);

    append_size_t_to_list(&source->output_variables, variable->number);
}

const char* generate_shader_code(ShaderSource* source)
{
    check(source);

    if (source->output_variables.length == 0)
        print_severe_message("Shader code has no output.");

    clear_source_code(source);

    set_string(&source->code, "#version 400\n\n");

    write_required_global_variable_expressions(source);

    extend_string(&source->code, "\nvoid main(void)\n{\n");

    for (reset_list_iterator(&source->output_variables); valid_list_iterator(&source->output_variables); advance_list_iterator(&source->output_variables))
    {
        write_required_variable_expressions(source, get_current_size_t_from_list(&source->output_variables));
    }

    extend_string(&source->code, "}\n");

    return source->code.chars;
}

void remove_variable_in_shader(ShaderSource* source, size_t variable_number)
{
    check(source);

    LinkedList directly_dependent_variables = create_list();
    find_directly_dependent_variables(&directly_dependent_variables, source, variable_number);

    delete_variable(source, variable_number);

    for (reset_list_iterator(&directly_dependent_variables); valid_list_iterator(&directly_dependent_variables); advance_list_iterator(&directly_dependent_variables))
    {
        remove_variable_in_shader(source, get_current_size_t_from_list(&directly_dependent_variables));
    }

    clear_list(&directly_dependent_variables);
}

void destroy_shader_source(ShaderSource* source)
{
    check(source);

    clear_string(&source->code);
    destroy_map(&source->global_variable_expressions);

    size_t i;
    for (i = 0; i < source->variables.length; i++)
    {
        delete_variable(source, i);
    }

    clear_list(&source->variables);
    clear_list(&source->deleted_variables);
    clear_list(&source->output_variables);
    source->next_undeleted_unused_variable_number = 0;
}

static void find_directly_dependent_variables(LinkedList* directly_dependent_variables, ShaderSource* source, size_t variable_number)
{
    assert(directly_dependent_variables);
    assert(source);
    assert(variable_number < source->variables.length);

    for (reset_list_iterator(&source->variables); valid_list_iterator(&source->variables); advance_list_iterator(&source->variables))
    {
        Variable* const variable = get_current_variable(&source->variables);

        if (!variable->is_deleted && variable->number != variable_number)
        {
            for (reset_list_iterator(&variable->dependencies); valid_list_iterator(&variable->dependencies); advance_list_iterator(&variable->dependencies))
            {
                Dependency* const dependency = get_current_dependency(&variable->dependencies);

                if (!dependency->is_global && dependency->variable_number == variable_number)
                    append_size_t_to_list(directly_dependent_variables, variable->number);
            }
        }
    }
}

static void find_required_global_variables(HashMap* required_global_variables, ShaderSource* source)
{
    assert(required_global_variables);
    assert(source);

    for (reset_list_iterator(&source->variables); valid_list_iterator(&source->variables); advance_list_iterator(&source->variables))
    {
        Variable* const variable = get_current_variable(&source->variables);

        if (!variable->is_deleted)
        {
            for (reset_list_iterator(&variable->dependencies); valid_list_iterator(&variable->dependencies); advance_list_iterator(&variable->dependencies))
            {
                Dependency* const dependency = get_current_dependency(&variable->dependencies);

                if (dependency->is_global)
                    insert_new_map_item(required_global_variables, dependency->global_variable_name, 1);
            }
        }
    }
}

static void clear_source_code(ShaderSource* source)
{
    assert(source);

    clear_string(&source->code);

    for (reset_list_iterator(&source->variables); valid_list_iterator(&source->variables); advance_list_iterator(&source->variables))
    {
        Variable* const variable = get_current_variable(&source->variables);
        variable->is_written = 0;
    }
}

static void write_required_global_variable_expressions(ShaderSource* source)
{
    assert(source);

    HashMap required_global_variables = create_map();
    find_required_global_variables(&required_global_variables, source);

    for (reset_map_iterator(&required_global_variables); valid_map_iterator(&required_global_variables); advance_map_iterator(&required_global_variables))
    {
        const char* global_variable_name = get_current_map_key(&required_global_variables);
        const char* expression = get_string_from_map(&source->global_variable_expressions, global_variable_name);

        if (!expression)
            print_severe_message("Could not find required global shader variable %s.", global_variable_name);

        extend_string(&source->code, expression);
    }

    destroy_map(&required_global_variables);
}

static void write_required_variable_expressions(ShaderSource* source, size_t variable_number)
{
    assert(source);
    check(variable_number < source->variables.length);

    Variable* const variable = get_variable(&source->variables, variable_number);

    if (variable->is_deleted || !variable->expression.chars)
        print_severe_message("Could not find required shader variable with number %d.", variable_number);

    for (reset_list_iterator(&variable->dependencies); valid_list_iterator(&variable->dependencies); advance_list_iterator(&variable->dependencies))
    {
        Dependency* const dependency = get_current_dependency(&variable->dependencies);

        if (!dependency->is_global)
        {
            Variable* const dependent_variable = get_variable(&source->variables, dependency->variable_number);

            if (!dependent_variable->is_written)
                write_required_variable_expressions(source, dependency->variable_number);
        }
    }

    extend_string(&source->code, variable->expression.chars);
    variable->is_written = 1;
}

static void add_sampler3D_uniform(ShaderSource* source, const char* name)
{
    check(source);
    check(name);
    insert_string_in_map(&source->global_variable_expressions, name, "uniform sampler3D %s;\n", name);
}

static void add_sampler1D_uniform(ShaderSource* source, const char* name)
{
    check(source);
    check(name);
    insert_string_in_map(&source->global_variable_expressions, name, "uniform sampler1D %s;\n", name);
}

static void add_output(ShaderSource* source, const char* type, const char* name)
{
    check(source);
    check(type);
    check(name);
    insert_string_in_map(&source->global_variable_expressions, name, "out %s %s;\n", type, name);
}

static Variable* create_variable(ShaderSource* source)
{
    assert(source);

    size_t variable_number;
    Variable* variable = NULL;

    if (source->deleted_variables.length > 0)
    {
        variable_number = get_size_t_from_list(&source->deleted_variables, 0);
        remove_first_list_item(&source->deleted_variables);

        variable = get_variable(&source->variables, variable_number);

        clear_string(&variable->expression);
        clear_list(&variable->dependencies);
    }
    else
    {
        variable_number = source->next_undeleted_unused_variable_number++;

        ListItem item = insert_new_list_item(&source->variables, variable_number, sizeof(Variable));
        variable = (Variable*)item.data;

        variable->expression = create_empty_string();
        variable->dependencies = create_list();
    }

    variable->number = variable_number;
    variable->is_written = 0;
    variable->is_deleted = 0;

    return variable;
}

static void delete_variable(ShaderSource* source, size_t variable_number)
{
    assert(source);
    assert(variable_number < source->variables.length);

    Variable* const variable = get_variable(&source->variables, variable_number);

    if (variable->is_deleted)
        return;

    variable->number = 0;
    clear_string(&variable->expression);
    clear_list(&variable->dependencies);
    variable->is_written = 0;
    variable->is_deleted = 1;

    append_size_t_to_list(&source->deleted_variables, variable_number);
}

static void add_global_dependency(Variable* variable, const char* global_variable_name)
{
    assert(sizeof(char)*(strlen(global_variable_name) + 1) <= MAX_GLOBAL_VARIABLE_NAME_SIZE);

    ListItem item = append_new_list_item(&variable->dependencies, sizeof(Dependency));
    Dependency* const dependency = (Dependency*)item.data;

    dependency->variable_number = 0;
    strcpy(dependency->global_variable_name, global_variable_name);
    dependency->is_global = 1;
}

static void add_variable_dependency(Variable* variable, size_t variable_number)
{
    ListItem item = append_new_list_item(&variable->dependencies, sizeof(Dependency));
    Dependency* const dependency = (Dependency*)item.data;

    dependency->variable_number = variable_number;
    dependency->global_variable_name[0] = '\0';
    dependency->is_global = 0;
}

static Variable* get_variable(LinkedList* variables, size_t variable_number)
{
    assert(variables);
    assert(variable_number < variables->length);

    ListItem item = get_list_item(variables, variable_number);
    assert(item.size == sizeof(Variable));
    check(item.data);
    return (Variable*)item.data;
}

static Variable* get_current_variable(LinkedList* variables)
{
    assert(variables);

    ListItem item = get_current_list_item(variables);
    assert(item.size == sizeof(Variable));
    check(item.data);
    return (Variable*)item.data;
}

static Dependency* get_current_dependency(LinkedList* dependencies)
{
    ListItem item = get_current_list_item(dependencies);
    assert(item.size == sizeof(Dependency));
    check(item.data);
    return (Dependency*)item.data;
}
