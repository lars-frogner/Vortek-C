#include "window.h"

#include "error.h"
#include "renderer.h"
#include "transformation.h"
#include "clip_planes.h"

#include <stdio.h>
#include <time.h>

#include <GLFW/glfw3.h>


#define WINDOW_TITLE "Vortek"
#define DEFAULT_WINDOW_HEIGHT_FRACTION 0.6f
#define DEFAULT_WINDOW_ASPECT_RATIO 1.0f


typedef struct Window
{
    GLFWwindow* handle;
    int width_screen_coords;
    int height_screen_coords;
} Window;

typedef struct FPSCounter
{
    double previous_time;
    unsigned int frame_count;

} FPSCounter;


static void get_screen_resolution(int* width, int* height);
static void update_FPS(GLFWwindow* window_handle);
static void error_callback(int error, const char* description);
static void framebuffer_size_callback(GLFWwindow* window_handle, int width, int height);
static void window_size_callback(GLFWwindow* window_handle, int width, int height);
static void keyboard_callback(GLFWwindow* window_handle, int key, int scancode, int action, int mods);
static void mouse_button_callback(GLFWwindow* window_handle, int button, int action, int mods);
static void cursor_pos_callback(GLFWwindow* window_handle, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window_handle, double xoffset, double yoffset);


static Window window = {NULL, 0, 0};

static FPSCounter fps_counter;

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

    window.handle = glfwCreateWindow((int)(screen_height*DEFAULT_WINDOW_HEIGHT_FRACTION*DEFAULT_WINDOW_ASPECT_RATIO),
                                     (int)(screen_height*DEFAULT_WINDOW_HEIGHT_FRACTION),
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

    int current_window_width, current_window_height;
    glfwGetFramebufferSize(window.handle, &current_window_width, &current_window_height);
    check(current_window_width > 0);
    check(current_window_height > 0);

    update_renderer_window_size_in_pixels(current_window_width, current_window_height);

    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window.handle, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window.handle, window_size_callback);
    glfwSetKeyCallback(window.handle, keyboard_callback);
    glfwSetMouseButtonCallback(window.handle, mouse_button_callback);
    glfwSetCursorPosCallback(window.handle, cursor_pos_callback);
    glfwSetScrollCallback(window.handle, scroll_callback);
}

void mainloop(void)
{
    check(window.handle);

    fps_counter.previous_time = glfwGetTime();
    fps_counter.frame_count = 0;

    while (!glfwWindowShouldClose(window.handle))
    {
        renderer_update_callback();
        update_FPS(window.handle);
        glfwPollEvents();
        glfwSwapBuffers(window.handle);
    }
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
    fps_counter.frame_count++;
    const double current_time = glfwGetTime();
    const double duration = current_time - fps_counter.previous_time;
    if (duration >= 0.2f)
    {
        const double fps = fps_counter.frame_count/duration;
        char title[40];
        sprintf(title, "%s - %.1f FPS", WINDOW_TITLE, fps);
        glfwSetWindowTitle(window_handle, title);

        fps_counter.previous_time = current_time;
        fps_counter.frame_count = 0;
    }
}

static void error_callback(int error, const char* description)
{
    print_error_message(description);
}

static void framebuffer_size_callback(GLFWwindow* window_handle, int width, int height)
{
    renderer_resize_callback(width, height);
}

static void window_size_callback(GLFWwindow* window_handle, int width, int height)
{
    window.width_screen_coords = width;
    window.height_screen_coords = height;
}

static void keyboard_callback(GLFWwindow* window_handle, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_LEFT_SHIFT:
            {
                disable_camera_control();
                enable_clip_plane_control();
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

                break;
            }
            case GLFW_KEY_F:
            {
                if (mods == GLFW_MOD_SHIFT)
                {
                    clip_plane_control_flip_callback();
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
                enable_camera_control();
                disable_clip_plane_control();
                break;
            }
            default:
                break;
        }
    }
}

static void mouse_button_callback(GLFWwindow* window_handle, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            double x_click_pos, y_click_pos;
            glfwGetCursorPos(window_handle, &x_click_pos, &y_click_pos);
            camera_control_drag_start_callback(x_click_pos, y_click_pos, window.width_screen_coords, window.height_screen_coords);
            clip_plane_control_drag_start_callback(x_click_pos, y_click_pos, window.width_screen_coords, window.height_screen_coords);

            mouse_is_pressed = 1;
        }
        else if (action == GLFW_RELEASE)
        {
            mouse_is_pressed = 0;
            camera_control_drag_end_callback();
        }
    }
}

static void cursor_pos_callback(GLFWwindow* window_handle, double xpos, double ypos)
{
    if (mouse_is_pressed)
    {
        camera_control_drag_callback(xpos, ypos, window.width_screen_coords, window.height_screen_coords);
        clip_plane_control_drag_callback(xpos, ypos, window.width_screen_coords, window.height_screen_coords);
    }
}

static void scroll_callback(GLFWwindow* window_handle, double xoffset, double yoffset)
{
    camera_control_scroll_callback(yoffset);
    clip_plane_control_scroll_callback(yoffset);
}
