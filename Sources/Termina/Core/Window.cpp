#include "Window.hpp"
#include "Assert.hpp"
#include "Logger.hpp"
#include "glfw3.h"

namespace Termina {
    Window::Window(int width, int height, const char* title)
        : m_Width(width), m_Height(height)
    {
        TN_ASSERT(glfwInit(), "Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

        m_Window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        TN_ASSERT(m_Window, "Failed to create GLFW window");

        glfwGetWindowSize(m_Window, &m_Width, &m_Height);
        glfwGetFramebufferSize(m_Window, &m_PixelWidth, &m_PixelHeight);

        TN_INFO("Created window of size (%d, %d)", width, height);
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    bool Window::IsOpen()
    {
        return !glfwWindowShouldClose(m_Window);
    }

    void Window::Update()
    {
        glfwGetWindowSize(m_Window, &m_Width, &m_Height);
        glfwGetFramebufferSize(m_Window, &m_PixelWidth, &m_PixelHeight);
    }
}
