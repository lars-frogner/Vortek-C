#include "window.h"

#include "error.h"
#include "renderer.h"
#include "transformation.h"
#include "view_aligned_planes.h"
#include "clip_planes.h"

#include <stdio.h>
#include <time.h>

#include <GLFW/glfw3.h>


#define GLFW_MOD_NONE 0

#define WINDOW_TITLE "Vortek"
#define DEFAULT_WINDOW_HEIGHT_FRACTION 0.6f
#define DEFAULT_WINDOW_ASPECT_RATIO 1.0f


typedef struct Window
{
    GLFWwindow* handle;
    int width_screen_coords;
    int height_screen_coords;
    int width_pixels;
    int height_pixels;
} Window;

typedef struct FrameTimer
{
    double previous_time;
    unsigned int frame_count;
    char title[40];
} FrameTimer;


static void get_screen_resolution(int* width, int* height);
static void update_FPS(GLFWwindow* window_handle);
static void error_callback(int error, const char* description);
static void framebuffer_size_callback(GLFWwindow* window_handle, int width, int height);
static void window_size_callback(GLFWwindow* window_handle, int width, int height);
static void keyboard_callback(GLFWwindow* window_handle, int key, int scancode, int action, int mods);
static void mouse_button_callback(GLFWwindow* window_handle, int button, int action, int mods);
static void cursor_pos_callback(GLFWwindow* window_handle, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window_handle, double xoffset, double yoffset);


static Window window = {NULL, 0, 0, 0, 0};

static FrameTimer frame_timer;

static int mouse_is_pressed = 0;


void initialize_window(void)
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        print_severe_message("Could not initialize GLFW.");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int screen_width, screen_height;
    get_screen_resolution(&screen_width, &screen_height);

    window.handle = glfwCreateWindow((int)((float)screen_height*DEFAULT_WINDOW_HEIGHT_FRACTION*DEFAULT_WINDOW_ASPECT_RATIO),
                                     (int)((float)screen_height*DEFAULT_WINDOW_HEIGHT_FRACTION),
                                     WINDOW_TITLE, NULL, NULL);

    if (!window.handle)
    {
        glfwTerminate();
        print_severe_message("Could not create window.");
    }

    glfwMakeContextCurrent(window.handle);

    glfwGetWindowSize(window.handle, &window.width_screen_coords, &window.height_screen_coords);
    check(window.width_screen_coords > 0);
    check(window.height_screen_coords > 0);

    glfwGetFramebufferSize(window.handle, &window.width_pixels, &window.height_pixels);
    check(window.width_pixels > 0);
    check(window.height_pixels > 0);

    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window.handle, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window.handle, window_size_callback);
    glfwSetKeyCallback(window.handle, keyboard_callback);
    glfwSetMouseButtonCallback(window.handle, mouse_button_callback);
    glfwSetCursorPosCallback(window.handle, cursor_pos_callback);
    glfwSetScrollCallback(window.handle, scroll_callback);
}

void initialize_mainloop(void)
{
    frame_timer.previous_time = glfwGetTime();
    frame_timer.frame_count = 0;
}

int step_mainloop(void)
{
    if (glfwWindowShouldClose(window.handle))
        return 0;

    glfwPollEvents();

    const double start_time = glfwGetTime();
    const int was_rendered = perform_rendering();
    const double end_time = glfwGetTime();

    if (was_rendered)
    {
        sprintf(frame_timer.title, "%s - %.2g ms", WINDOW_TITLE, 1000.0*(end_time - start_time));
        glfwSetWindowTitle(window.handle, frame_timer.title);

        glfwSwapBuffers(window.handle);
    }

    return 1;
}

void mainloop(void)
{
    check(window.handle);

    initialize_mainloop();

    while (step_mainloop()) { continue; }
}

void focus_window(void)
{
    glfwFocusWindow(window.handle);
}

void get_window_shape_in_pixels(int* width, int* height)
{
    assert(width);
    assert(height);
    *width = window.width_pixels;
    *height = window.height_pixels;
}

void get_window_shape_in_screen_coordinates(int* width, int* height)
{
    assert(width);
    assert(height);
    *width = window.width_screen_coords;
    *height = window.height_screen_coords;
}

float get_window_aspect_ratio(void)
{
    return (float)window.width_pixels/(float)window.height_pixels;
}

void cleanup_window(void)
{
    check(window.handle);
    glfwDestroyWindow(window.handle);
    glfwTerminate();
}

static void get_screen_resolution(int* width, int* height)
{
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    *width = mode->width;
    *height = mode->height;
}

static void update_FPS(GLFWwindow* window_handle)
{
    frame_timer.frame_count++;
    const double current_time = glfwGetTime();
    const double duration = current_time - frame_timer.previous_time;
    if (duration >= 0.5f)
    {
        const double fps = frame_timer.frame_count/duration;
        sprintf(frame_timer.title, "%s - %.2g FPS", WINDOW_TITLE, fps);
        glfwSetWindowTitle(window_handle, frame_timer.title);

        frame_timer.previous_time = current_time;
        frame_timer.frame_count = 0;
    }
}

static void error_callback(int error, const char* description)
{
    print_error_message(description);
}

static void framebuffer_size_callback(GLFWwindow* window_handle, int width, int height)
{
    window.width_pixels = width;
    window.height_pixels = height;
    renderer_resize_callback(width, height);
    require_rendering();
}

static void window_size_callback(GLFWwindow* window_handle, int width, int height)
{
    window.width_screen_coords = width;
    window.height_screen_coords = height;
}

static void keyboard_callback(GLFWwindow* window_handle, int key, int scancode, int action, int mods)
{
    if (!has_rendering_data())
        return;

    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_LEFT_SHIFT:
            {
                camera_control_drag_end_callback();
                disable_camera_control();
                enable_clip_plane_control();
                require_rendering();
                break;
            }
            case GLFW_KEY_1:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    set_clip_plane_state(0, CLIP_PLANE_ENABLED);
                    set_controllable_clip_plane(0);
                }
                else
                    toggle_clip_plane_enabled_state(0);

                require_rendering();

                break;
            }
            case GLFW_KEY_2:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    set_clip_plane_state(1, CLIP_PLANE_ENABLED);
                    set_controllable_clip_plane(1);
                }
                else
                    toggle_clip_plane_enabled_state(1);

                require_rendering();

                break;
            }
            case GLFW_KEY_3:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    set_clip_plane_state(2, CLIP_PLANE_ENABLED);
                    set_controllable_clip_plane(2);
                }
                else
                    toggle_clip_plane_enabled_state(2);

                require_rendering();

                break;
            }
            case GLFW_KEY_4:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    set_clip_plane_state(3, CLIP_PLANE_ENABLED);
                    set_controllable_clip_plane(3);
                }
                else
                    toggle_clip_plane_enabled_state(3);

                require_rendering();

                break;
            }
            case GLFW_KEY_5:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    set_clip_plane_state(4, CLIP_PLANE_ENABLED);
                    set_controllable_clip_plane(4);
                }
                else
                    toggle_clip_plane_enabled_state(4);

                require_rendering();

                break;
            }
            case GLFW_KEY_6:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    set_clip_plane_state(5, CLIP_PLANE_ENABLED);
                    set_controllable_clip_plane(5);
                }
                else
                    toggle_clip_plane_enabled_state(5);

                require_rendering();

                break;
            }
            case GLFW_KEY_F:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    clip_plane_control_flip_callback();
                    require_rendering();
                }
                break;
            }
            case GLFW_KEY_X:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    clip_plane_control_set_normal_to_x_axis_callback();
                    require_rendering();
                }
                break;
            }
            case GLFW_KEY_Y:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    clip_plane_control_set_normal_to_y_axis_callback();
                    require_rendering();
                }
                break;
            }
            case GLFW_KEY_Z:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    clip_plane_control_set_normal_to_z_axis_callback();
                    require_rendering();
                }
                break;
            }
            case GLFW_KEY_V:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    clip_plane_control_set_normal_to_look_axis_callback();
                    require_rendering();
                }
                break;
            }
            case GLFW_KEY_O:
            {
                if (mods == GLFW_MOD_NONE)
                {
                    toggle_field_outline_drawing();
                    require_rendering();
                }
                else if (mods == GLFW_MOD_SHIFT)
                {
                    clip_plane_control_reset_origin_shift_callback();
                    require_rendering();
                }
                break;
            }
            case GLFW_KEY_B:
            {
                if (mods == GLFW_MOD_NONE)
                {
                    toggle_brick_outline_drawing();
                    require_rendering();
                }
                break;
            }
            case GLFW_KEY_T:
            {
                if (mods == GLFW_MOD_NONE)
                {
                    toggle_sub_brick_outline_drawing();
                    require_rendering();
                }
                break;
            }
            default:
                break;
        }
    }
    else if (action == GLFW_RELEASE)
    {
        switch (key)
        {
            case GLFW_KEY_LEFT_SHIFT:
            {
                clip_plane_control_drag_end_callback();
                disable_clip_plane_control();
                enable_camera_control();
                require_rendering();
                break;
            }
            default:
                break;
        }
    }
}

static void mouse_button_callback(GLFWwindow* window_handle, int button, int action, int mods)
{
    if (!has_rendering_data())
        return;

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            double x_click_pos, y_click_pos;
            glfwGetCursorPos(window_handle, &x_click_pos, &y_click_pos);
            camera_control_drag_start_callback(x_click_pos, y_click_pos);
            clip_plane_control_drag_start_callback(x_click_pos, y_click_pos);
            require_rendering();

            mouse_is_pressed = 1;
        }
        else if (action == GLFW_RELEASE)
        {
            mouse_is_pressed = 0;
            camera_control_drag_end_callback();
            clip_plane_control_drag_end_callback();
            require_rendering();
        }
    }
}

static void cursor_pos_callback(GLFWwindow* window_handle, double xpos, double ypos)
{
    if (mouse_is_pressed)
    {
        camera_control_drag_callback(xpos, ypos);
        clip_plane_control_drag_callback(xpos, ypos);
        require_rendering();
    }
}

static void scroll_callback(GLFWwindow* window_handle, double xoffset, double yoffset)
{
    if (!has_rendering_data())
        return;

    camera_control_scroll_callback(yoffset);
    clip_plane_control_scroll_callback(yoffset);
    require_rendering();
}
