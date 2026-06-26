# 🌟 Catch the Falling Object

A 2D interactive arcade-style game built using **C++** and **OpenGL**. This project was developed as part of a Computer Graphics coursework to practically implement core computer graphics algorithms, including Bresenham's line and circle drawing algorithms, custom polygon filling, 2D transformations, and shader-based animations.

## 🎮 Gameplay Overview
The objective of the game is simple: control the neon basket at the bottom of the screen to catch the falling stars.

* **Golden Stars:** These stars glow dynamically and pulse (scale up and down) as they fall.
* **Rainbow Stars:** These stars feature a smooth, interpolated multi-color gradient and continuously spin.
* **Progressive Difficulty:** Every time you successfully catch a star, your score increases by 1, and the falling speed of the next star increases slightly, testing your reflexes.
* **Game Over:** If a star misses the basket and hits the ground, the game ends. The final score is printed in the console, and the game resets for a new round.

## 🕹️ Controls
| Action | Key Binding |
| :--- | :--- |
| **Move Left** | `Left Arrow` or `A` |
| **Move Right** | `Right Arrow` or `D` |
| **Quit Game** | `ESC` |

## 🛠️ Technical Concepts Implemented
Instead of relying on built-in OpenGL shape primitives, this project utilizes fundamental computer graphics algorithms to construct the game assets:

1.  **Bresenham's Line Algorithm:** * Used to mathematically plot and draw the boundary line (the ground) at the bottom of the screen.
2.  **Bresenham's Circle Algorithm (Modified):** * The top opening of the basket (Bright Pink) is generated using a Bresenham circle algorithm that is mathematically scaled along the X-axis to form a perfect oval.
3.  **Scanline Polygon Filling:** * The basket's solid body (Purple) and the top oval are filled completely using horizontal scanlines calculated from the outer boundary points.
4.  **2D Transformations (`glm`):** * **Translation:** Used to move the basket horizontally across the screen and to drop the stars vertically.
    * **Rotation:** Applied to the Rainbow stars to make them spin continuously based on `glfwGetTime()`.
    * **Scaling:** Applied to the Golden stars using a sine wave function to create a smooth, continuous pulsing animation.
5.  **Shader Programming & Interpolation:**
    * **Vertex Color Interpolation:** The Rainbow stars define specific RGB values at each vertex, allowing the fragment shader to smoothly interpolate a color gradient across the shape.
    * **Time-based Color Animation:** The Golden stars shift their color values dynamically over time using uniform variables linked to trigonometric functions in the fragment shader.

## 📂 Project Dependencies
To compile and run this project, the following libraries are required (and should be included in your project directory):
* **GLFW:** For window creation, context management, and input handling.
* **GLAD:** For loading OpenGL function pointers.
* **GLM (OpenGL Mathematics):** For handling matrix and vector math operations.

## 🚀 How to Compile and Run
1.  Ensure you have a C++ compiler installed (e.g., GCC, MinGW, or MSVC).
2.  Place your `glad.c`, `glad.h`, `glfw3.h`, and the `glm` folder in the same directory as this `main.cpp` file.
3.  Link the necessary OpenGL and GLFW libraries during compilation. 
    *(Example for MinGW on Windows):*
    ```bash
    g++ main.cpp glad.c -o CatchTheObject.exe -lglfw3 -lgdi32 -lopengl32
    ```
4.  Run the generated executable:
    ```bash
    ./CatchTheObject.exe
    ```
## Demo Video

[▶ Watch Demo Video](https://drive.google.com/file/d/14DuJ0AApNipXFbUbbcFq-Kb16AfwDCKM/view)

## 👨‍💻 Author
Developed for Computer Graphics Coursework.
