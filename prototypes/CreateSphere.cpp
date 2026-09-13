#include <iostream>
#include <cmath>
#include <variant>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <OpenGL/gl3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstring>

// Simple orbit camera controlled by the mouse. It hooks into:
// - glfwSetFramebufferSizeCallback to install mouse callbacks when the window is created
// - glGetUniformLocation to remember the "view" uniform location
// - glUniformMatrix4fv to override the "view" matrix with our orbit camera each frame

struct OrbitCamera {
    double lastX = 0.0, lastY = 0.0;
    bool rotating = false;
    bool panning = false;

    float yaw = 0.0f;    // radians
    float pitch = 0.0f;  // radians
    float distance = 2.0f;
    glm::vec3 target = glm::vec3(0.0f);

    float rotateSpeed = 0.005f;
    float panSpeed = 0.0015f;
    float zoomSpeed = 0.1f;
} static gCam;

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static glm::vec3 OrbitCamPos()
{
    float cp = cosf(gCam.pitch);
    float sp = sinf(gCam.pitch);
    float sy = sinf(gCam.yaw);
    float cy = cosf(gCam.yaw);

    glm::vec3 dir(cp * sy, sp, cp * cy); // forward from target
    return gCam.target + dir * gCam.distance;
}

static glm::mat4 GetCurrentViewMatrix()
{
    glm::vec3 pos = OrbitCamPos();
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    return glm::lookAt(pos, gCam.target, up);
}

// ---------- Mouse callbacks ----------
static void CursorPosCB(GLFWwindow* window, double x, double y)
{
    (void)window;
    if (!gCam.rotating && !gCam.panning) {
        gCam.lastX = x; gCam.lastY = y;
        return;
    }

    double dx = x - gCam.lastX;
    double dy = y - gCam.lastY;
    gCam.lastX = x; gCam.lastY = y;

    if (gCam.rotating) {
        gCam.yaw   -= static_cast<float>(dx) * gCam.rotateSpeed;
        gCam.pitch -= static_cast<float>(dy) * gCam.rotateSpeed;
        float lim = 1.55334303f; // ~89 deg
        gCam.pitch = clampf(gCam.pitch, -lim, lim);
    } else if (gCam.panning) {
        // Pan in camera plane
        glm::vec3 pos = OrbitCamPos();
        glm::vec3 fwd = glm::normalize(gCam.target - pos);
        glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0,1,0)));
        glm::vec3 up = glm::normalize(glm::cross(right, fwd));

        float scale = gCam.distance * gCam.panSpeed;
        gCam.target -= right * static_cast<float>(dx) * scale;
        gCam.target += up    * static_cast<float>(dy) * scale;
    }
}

static void MouseButtonCB(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        gCam.rotating = (action == GLFW_PRESS);
        gCam.lastX = x; gCam.lastY = y;
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE || button == GLFW_MOUSE_BUTTON_RIGHT) {
        gCam.panning = (action == GLFW_PRESS);
        gCam.lastX = x; gCam.lastY = y;
    }
}

static void ScrollCB(GLFWwindow* window, double xoff, double yoff)
{
    (void)window; (void)xoff;
    float factor = 1.0f - static_cast<float>(yoff) * gCam.zoomSpeed;
    if (factor < 0.1f) factor = 0.1f;
    gCam.distance = clampf(gCam.distance * factor, 0.2f, 100.0f);
}

static void InstallInput(GLFWwindow* window)
{
    static bool installed = false;
    if (installed || !window) return;
    installed = true;

    glfwSetCursorPosCallback(window, CursorPosCB);
    glfwSetMouseButtonCallback(window, MouseButtonCB);
    glfwSetScrollCallback(window, ScrollCB);
}

// ---------- Hook uniforms to override "view" matrix ----------
static auto p_glGetUniformLocation = glGetUniformLocation;
static auto p_glUniformMatrix4fv = glUniformMatrix4fv;
static GLint g_uViewLoc = -1;

static GLint TrackUniformLocation(GLuint program, const char* name)
{
    GLint loc = p_glGetUniformLocation(program, name);
    if (name && std::strcmp(name, "view") == 0) g_uViewLoc = loc;
    return loc;
}

static void OverrideUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    if (location == g_uViewLoc && count == 1) {
        glm::mat4 view = GetCurrentViewMatrix();
        p_glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(view));
    } else {
        p_glUniformMatrix4fv(location, count, transpose, value);
    }
}

// Replace these functions after capturing originals
#define glGetUniformLocation(prog, name) TrackUniformLocation((prog), (name))
#define glUniformMatrix4fv(loc, count, transpose, value) OverrideUniformMatrix4fv((loc), (count), (transpose), (value))

// Install input automatically when the app sets the framebuffer size callback (right after window creation)
static auto p_glfwSetFramebufferSizeCallback = glfwSetFramebufferSizeCallback;
#define glfwSetFramebufferSizeCallback(win, cb) (InstallInput((win)), p_glfwSetFramebufferSizeCallback((win), (cb)))

// Minimal OpenGL + GLFW + GLM program that renders a lit sphere

// --------- Shaders ---------
static const char* kVertexShader = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out float lightIntensity;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Approximate normal from position (sphere centered at origin)
    vec3 normal = normalize(aPos);

    // Simple directional light
    vec3 lightDir = normalize(vec3(0.6, 0.8, 0.4));
    float NdotL = max(dot(normal, lightDir), 0.0);

    // Add a small ambient term
    lightIntensity = max(NdotL, 0.15);
}
)glsl";

static const char* kFragmentShader = R"glsl(
#version 330 core
in float lightIntensity;
out vec4 FragColor;

uniform vec4 objectColor;
uniform bool isGrid;
uniform bool GLOW;

void main()
{
    if (isGrid) {
        FragColor = objectColor;
    } else if (GLOW) {
        FragColor = vec4(objectColor.rgb * 10.0, objectColor.a);
    } else {
        FragColor = vec4(objectColor.rgb * lightIntensity, objectColor.a);
    }
}
)glsl";

// --------- Helpers ---------
/*
static void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                                     GLsizei length, const GLchar* message, const void* userParam)
{
    (void)source; (void)type; (void)id; (void)severity; (void)length; (void)userParam;
    fprintf(stderr, "GL Debug: %s\n", message);
}
*/

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    glViewport(0, 0, width, height);
}


static GLuint CompileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        fprintf(stderr, "Shader compile error: %s\n", log.data());
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint CreateProgram(const char* vs, const char* fs)
{
    GLuint v = CompileShader(GL_VERTEX_SHADER, vs);
    if (!v) return 0;
    GLuint f = CompileShader(GL_FRAGMENT_SHADER, fs);
    if (!f) { glDeleteShader(v); return 0; }

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(p, len, nullptr, log.data());
        fprintf(stderr, "Program link error: %s\n", log.data());
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

static glm::vec3 sphericalToCartesian(float r, float theta, float phi)
{
    float x = r * sinf(theta) * cosf(phi);
    float y = r * cosf(theta);
    float z = r * sinf(theta) * sinf(phi);
    return glm::vec3(x, y, z);
}

static std::vector<float> GenerateSphere(float radius, int stacks, int sectors)
{
    std::vector<float> vertices;
    vertices.reserve(stacks * sectors * 6 * 3); // 2 triangles per quad, 3 verts each, 3 comps

    for (int i = 0; i < stacks; ++i) {
        float theta1 = (float)i / (float)stacks * glm::pi<float>();
        float theta2 = (float)(i + 1) / (float)stacks * glm::pi<float>();

        for (int j = 0; j < sectors; ++j) {
            float phi1 = (float)j / (float)sectors * glm::two_pi<float>();
            float phi2 = (float)(j + 1) / (float)sectors * glm::two_pi<float>();

            glm::vec3 v1 = sphericalToCartesian(radius, theta1, phi1);
            glm::vec3 v2 = sphericalToCartesian(radius, theta1, phi2);
            glm::vec3 v3 = sphericalToCartesian(radius, theta2, phi1);
            glm::vec3 v4 = sphericalToCartesian(radius, theta2, phi2);

            // Triangle 1: v1, v2, v3
            vertices.insert(vertices.end(), { v1.x, v1.y, v1.z,
                                              v2.x, v2.y, v2.z,
                                              v3.x, v3.y, v3.z });

            // Triangle 2: v2, v4, v3
            vertices.insert(vertices.end(), { v2.x, v2.y, v2.z,
                                              v4.x, v4.y, v4.z,
                                              v3.x, v3.y, v3.z });
        }
    }
    return vertices;
}

// --------- Main ---------
int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return 1;
    }

    // macOS friendly core context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenGL Sphere", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);



    // Build program
    GLuint program = CreateProgram(kVertexShader, kFragmentShader);
    if (!program) {
        glfwTerminate();
        return 1;
    }

    // Generate sphere geometry
    float sphereRadius = 0.5f;
    int stacks = 64;
    int sectors = 64;
    std::vector<float> sphere = GenerateSphere(sphereRadius, stacks, sectors);
    GLsizei sphereVertexCount = static_cast<GLsizei>(sphere.size() / 3);

    // Create VAO/VBO
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sphere.size() * sizeof(float), sphere.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    // GL state
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);

    // Uniform locations
    glUseProgram(program);
    GLint uModel = glGetUniformLocation(program, "model");
    GLint uView = glGetUniformLocation(program, "view");
    GLint uProj = glGetUniformLocation(program, "projection");
    GLint uObjColor = glGetUniformLocation(program, "objectColor");
    GLint uIsGrid = glGetUniformLocation(program, "isGrid");
    GLint uGlow = glGetUniformLocation(program, "GLOW");

    // Camera
    glm::vec3 camPos(0.0f, 0.0f, 2.0f);
    glm::mat4 model(1.0f);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Basic input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, 1);
        }

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /*
        // Animate slow rotation
        float t = (float)glfwGetTime();
        model = glm::rotate(glm::mat4(1.0f), t * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));
        */

        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.01f, 100.0f);

        glUseProgram(program);
        glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(proj));
        glUniform4f(uObjColor, 0.1f, 0.6f, 1.0f, 1.0f);
        glUniform1i(uIsGrid, GL_FALSE);
        glUniform1i(uGlow, GL_FALSE);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
