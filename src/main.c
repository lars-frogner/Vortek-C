#include "window.h"
#include "renderer.h"


int main(int argc, char const *argv[])
{
    initialize_window();
    initialize_renderer();

    mainloop();

    cleanup_renderer();
    cleanup_window();

    return 0;
}
