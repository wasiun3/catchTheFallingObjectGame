#include "glad.h"
#include "glfw3.h"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Game Variables
float basketX = 0.0f;
float objectX = 0.0f;
float objectY = 1.0f;
float objectSpeed = 1.5f;
int score = 0;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int currentStarType = 0; // 0 = Golden Animation, 1 = Rainbow Color

// --- Shaders ---
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "out vec3 vertexColor;\n"
    "uniform mat4 transform;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = transform * vec4(aPos, 1.0);\n"
    "   vertexColor = aColor;\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 vertexColor;\n"
    "uniform int renderMode;\n" 
    "uniform float time;\n"
    "void main()\n"
    "{\n"
    "   if(renderMode == 0) {\n"
    "       // Uses exact vertex colors (Basket Oval, Basket Body, Rainbow Star, Ground)\n"
    "       FragColor = vec4(vertexColor, 1.0f);\n"
    "   }\n"
    "   else if(renderMode == 1) {\n"
    "       // Golden Animation for Star\n"
    "       float greenValue = (sin(time * 6.0) / 2.0) + 0.45f;\n"
    "       FragColor = vec4(1.0f, greenValue, 0.0f, 1.0f);\n" // Mix of Red and animating Green gives glowing Gold
    "   }\n"
    "}\n\0";

// --- Shape Generation Functions ---

// 1. Generate Bresenham Line (For Ground)
std::vector<float> BresenhamLine(float x0_ndc, float y0_ndc, float x1_ndc, float y1_ndc)
{
    std::vector<float> points;
    int x0 = (x0_ndc + 1.0f) * 400; int y0 = (y0_ndc + 1.0f) * 300;
    int x1 = (x1_ndc + 1.0f) * 400; int y1 = (y1_ndc + 1.0f) * 300;

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    while (true) {
        points.push_back((x0 / 400.0f) - 1.0f); points.push_back((y0 / 300.0f) - 1.0f); points.push_back(0.0f);
        points.push_back(0.4f); points.push_back(0.4f); points.push_back(0.4f); // Gray Ground

        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
    return points;
}

// 2. Generate Beautiful Basket (Bright Pink Oval Top + Purple Body)
std::vector<float> GenerateBeautifulBasket() {
    std::vector<float> points;
    
    // Helper to draw horizontal fill lines with specific colors
    auto addHorizontalLine = [&](float xStart, float xEnd, float yPos, float r, float g, float b) {
        points.push_back(xStart); points.push_back(yPos); points.push_back(0.0f);
        points.push_back(r); points.push_back(g); points.push_back(b);
        points.push_back(xEnd); points.push_back(yPos); points.push_back(0.0f);
        points.push_back(r); points.push_back(g); points.push_back(b);
    };

    // A. Oval Top (Bresenham Circle Algorithm stretched on X-axis)
    int r = 12; // Y radius
    float scaleX = 5.0f; // Stretch X to make it a wide Oval
    int x = 0, y = r;
    int d = 3 - 2 * r;
    
    // Bright Pink Color
    float ovalR = 1.0f, ovalG = 0.1f, ovalB = 0.6f; 

    while (y >= x) {
        // Fill the oval using horizontal scanlines
        addHorizontalLine(-x * scaleX / 400.0f, x * scaleX / 400.0f, y / 300.0f, ovalR, ovalG, ovalB);
        addHorizontalLine(-x * scaleX / 400.0f, x * scaleX / 400.0f, -y / 300.0f, ovalR, ovalG, ovalB);
        addHorizontalLine(-y * scaleX / 400.0f, y * scaleX / 400.0f, x / 300.0f, ovalR, ovalG, ovalB);
        addHorizontalLine(-y * scaleX / 400.0f, y * scaleX / 400.0f, -x / 300.0f, ovalR, ovalG, ovalB);
        
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }

    // B. Bottom Body (Bounded by Left Line, Right Line, Bottom Line)
    // Purple Color
    float bodyR = 0.6f, bodyG = 0.1f, bodyB = 0.9f; 
    
    int topWidth = r * scaleX; // Match width with the oval
    int bottomWidth = topWidth - 20; // Tapered shape (narrower at the bottom)
    int depth = 50; // How deep the basket is

    // Start filling from the middle of the oval downwards
    for (int py = 0; py <= depth; py++) {
        float t = (float)py / depth; 
        int currentWidth = topWidth * (1.0f - t) + bottomWidth * t; // Interpolate width
        addHorizontalLine(-currentWidth / 400.0f, currentWidth / 400.0f, -py / 300.0f, bodyR, bodyG, bodyB);
    }

    return points;
}

// 3. Generate Star Geometry (Rainbow layout)
std::vector<float> GenerateStar() {
    std::vector<float> starVertices;
    float outerRadius = 0.12f;
    float innerRadius = 0.05f;
    
    // Center point
    starVertices.push_back(0.0f); starVertices.push_back(0.0f); starVertices.push_back(0.0f);
    starVertices.push_back(1.0f); starVertices.push_back(1.0f); starVertices.push_back(1.0f);

    for(int i = 0; i <= 10; ++i) {
        float angle = glm::radians(i * 36.0f + 90.0f);
        float radius = (i % 2 == 0) ? outerRadius : innerRadius;
        
        starVertices.push_back(cos(angle) * radius);
        starVertices.push_back(sin(angle) * radius);
        starVertices.push_back(0.0f);
        
        // Rainbow colors
        float r = (sin(angle) + 1.0f) / 2.0f;
        float g = (sin(angle + 2.0f) + 1.0f) / 2.0f;
        float b = (sin(angle + 4.0f) + 1.0f) / 2.0f;
        starVertices.push_back(r); starVertices.push_back(g); starVertices.push_back(b);
    }
    return starVertices;
}

// --- Game Logic ---
void resetObject() {
    objectY = 1.0f;
    objectX = (rand() % 160 / 100.0f) - 0.8f;
    objectSpeed += 0.1f; 
    currentStarType = rand() % 2; // Randomly choose Golden (0) or Rainbow (1)
}

int main()
{
    srand(static_cast<unsigned int>(time(0)));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Pink & Purple Basket Catcher", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- Build VAOs / VBOs ---

    // 1. Beautiful Basket (Oval + Body)
    std::vector<float> basketData = GenerateBeautifulBasket();
    unsigned int basketVAO, basketVBO;
    glGenVertexArrays(1, &basketVAO);
    glGenBuffers(1, &basketVBO);
    glBindVertexArray(basketVAO);
    glBindBuffer(GL_ARRAY_BUFFER, basketVBO);
    glBufferData(GL_ARRAY_BUFFER, basketData.size() * sizeof(float), basketData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 2. Star
    std::vector<float> starData = GenerateStar();
    unsigned int starVAO, starVBO;
    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);
    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, starData.size() * sizeof(float), starData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 3. Ground Line
    std::vector<float> lineData = BresenhamLine(-1.0f, -0.9f, 1.0f, -0.9f);
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    unsigned int renderModeLoc = glGetUniformLocation(shaderProgram, "renderMode");
    unsigned int timeLoc = glGetUniformLocation(shaderProgram, "time");

    resetObject();

    // Game Loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        objectY -= objectSpeed * deltaTime; 

        // Collision Check
        float basketY = -0.7f;
        if (objectY <= basketY + 0.1f && objectY >= basketY - 0.2f) { 
            if (objectX >= basketX - 0.18f && objectX <= basketX + 0.18f) { 
                score++;
                std::cout << "STAR CAUGHT! Score: " << score << std::endl;
                resetObject();
            }
        }

        // Miss Check
        if (objectY < -1.0f) {
            std::cout << "Missed! Game Over. Final Score: " << score << std::endl;
            score = 0; 
            objectSpeed = 1.0f; 
            resetObject();
        }

        // Background (Deep Navy Blue)
        glClearColor(0.0f, 0.0f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform1f(timeLoc, currentFrame);

        // 1. Draw Ground Line
        glUniform1i(renderModeLoc, 0); 
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(model));
        glPointSize(2.0f);
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_POINTS, 0, lineData.size() / 6);

        // 2. Draw Beautiful Basket
        glUniform1i(renderModeLoc, 0); // RenderMode 0 respects the multiple Vertex Colors (Pink Oval + Purple Body)
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(basketX, basketY, 0.0f)); 
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(basketVAO);
        // Using GL_LINES to fill the shape with horizontal scanlines
        glDrawArrays(GL_LINES, 0, basketData.size() / 6);

        // 3. Draw Star
        if (currentStarType == 0) {
            glUniform1i(renderModeLoc, 1); // Golden Animation Mode
        } else {
            glUniform1i(renderModeLoc, 0); // Rainbow Color Mode
        }
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(objectX, objectY, 0.0f)); 
        model = glm::rotate(model, currentFrame * 3.0f, glm::vec3(0.0f, 0.0f, 1.0f)); 
        
        if (currentStarType == 0) {
            // Golden star pulses (scales) up and down
            float scale = sin(currentFrame * 8.0f) * 0.2f + 1.0f; 
            model = glm::scale(model, glm::vec3(scale, scale, 1.0f));
        }

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindVertexArray(starVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 12); 

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &basketVAO); glDeleteBuffers(1, &basketVBO);
    glDeleteVertexArrays(1, &starVAO);   glDeleteBuffers(1, &starVBO);
    glDeleteVertexArrays(1, &lineVAO);   glDeleteBuffers(1, &lineVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
        
    float velocity = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        if (basketX > -0.8f) basketX -= velocity;
        
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        if (basketX < 0.8f) basketX += velocity;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}