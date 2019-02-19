#include "window.h"

#include "error.h"
#include "renderer.h"
#include "trackball.h"

#include <stdio.h>
#include <time.h>

#include <GLFW/glfw3.h>


#define WINDOW_TITLE "Vortek"
#define DEFAULT_WINDOW_WIDTH 600
#define DEFAULT_WINDOW_HEIGHT 600


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

    window.handle = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

    if (!window.handle)
    {
        glfwTerminate();
        print_severe_message("Could not create window.");
    }

    glfwSetFramebufferSizeCallback(window.handle, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window.handle, window_size_callback);
    glfwSetKeyCallback(window.handle, keyboard_callback);
    glfwSetMouseButtonCallback(window.handle, mouse_button_callback);
    glfwSetCursorPosCallback(window.handle, cursor_pos_callback);
    glfwSetScrollCallback(window.handle, scroll_callback);

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
            case GLFW_KEY_T:
            {
                renderer_key_action_callback();
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
            trackball_leftclick_callback(x_click_pos, y_click_pos, window.height_screen_coords);

            mouse_is_pressed = 1;
        }
        else if (action == GLFW_RELEASE)
        {
            mouse_is_pressed = 0;
        }
    }
}

static void cursor_pos_callback(GLFWwindow* window_handle, double xpos, double ypos)
{
    if (mouse_is_pressed)
    {
        trackball_mouse_drag_callback(xpos, ypos, window.height_screen_coords);
    }
}

static void scroll_callback(GLFWwindow* window_handle, double xoffset, double yoffset)
{
    trackball_zoom_callback(yoffset);
}
