#include "window.h"

#include "renderer.h"
#include "trackball.h"
#include "utils.h"

#include <stdlib.h>

#include <GLFW/glfw3.h>


#define WINDOW_TITLE "Window"
#define DEFAULT_WINDOW_WIDTH 600
#define DEFAULT_WINDOW_HEIGHT 600


typedef struct Window
{
    GLFWwindow* handle;
    int width_screen_coords;
    int height_screen_coords;
} Window;


static void error_callback(int error, const char* description);
static void framebuffer_size_callback(GLFWwindow* window_handle, int width, int height);
static void window_size_callback(GLFWwindow* window_handle, int width, int height);
static void keyboard_callback(GLFWwindow* window_handle, int key, int scancode, int action, int mods);
static void mouse_button_callback(GLFWwindow* window_handle, int button, int action, int mods);
static void cursor_pos_callback(GLFWwindow* window_handle, double xpos, double ypos);
static void scroll_callback(GLFWwindow* window_handle, double xoffset, double yoffset);


static Window window = {NULL, 0, 0};

static int mouse_is_pressed = 0;


void initialize_window()
{
    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
    {
        print_error("Could not initialize GLFW.");
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window.handle = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

    if (!window.handle)
    {
        print_error("Could not create window.");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetFramebufferSizeCallback(window.handle, framebuffer_size_callback);
    glfwSetWindowSizeCallback(window.handle, window_size_callback);
    glfwSetKeyCallback(window.handle, keyboard_callback);
    glfwSetMouseButtonCallback(window.handle, mouse_button_callback);
    glfwSetCursorPosCallback(window.handle, cursor_pos_callback);
    glfwSetScrollCallback(window.handle, scroll_callback);

    glfwMakeContextCurrent(window.handle);

    glfwGetWindowSize(window.handle, &window.width_screen_coords, &window.height_screen_coords);

    int current_window_width, current_window_height;
    glfwGetFramebufferSize(window.handle, &current_window_width, &current_window_height);
    update_renderer_window_size_in_pixels(current_window_width, current_window_height);

    glfwSwapInterval(1);
}

void cleanup_window()
{
    glfwDestroyWindow(window.handle);
    glfwTerminate();
}

void mainloop()
{
    while (!glfwWindowShouldClose(window.handle))
    {
        renderer_update_callback();
        glfwSwapBuffers(window.handle);
        glfwPollEvents();
    }
}

static void error_callback(int error, const char* description)
{
    print_error(description);
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
