#include <iostream>
#include <cmath>
#include <variant>
#include <OpenGL/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


int main(){

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 800, "Circle", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Render here
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_TRIANGLE_FAN);
        glColor3f(1.0f, 0.5f, 1.0f); // Red color
        glVertex2f(0.0f, 0.0f); // Center of circle
        int numSegments = 100;
        float radius = 0.5f;
        for (int i = 0; i <= numSegments; i++) {
            float angle = i * 2.0f * M_PI / numSegments;
            float x = radius * cosf(angle);
            float y = radius * sinf(angle);
            glVertex2f(x, y);
        }
        glEnd();

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}