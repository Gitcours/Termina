#include <Termina/Core/Window.hpp>
#include <Termina/Renderer/Renderer.hpp>

int main()
{
    Termina::Window window(1280, 720, "Hi");
    Termina::Renderer renderer(&window);
    while (window.IsOpen()) {
        window.Update();

        renderer.Render();
    }
    return 0;
}
