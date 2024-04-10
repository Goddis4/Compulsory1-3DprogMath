#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iomanip>
#include <functional>
#include <glm/glm.hpp>

using namespace std;

// Vertex Shader
const char* vertexShaderSource =
"#version 330 core\n"
"layout(location = 0) in vec3 aPosition;\n"
"uniform float pitchValue;\n"
"void main() {\n"
"    gl_Position = vec4(aPosition, 1.0);\n"
"}\0";

// Fragment Shader
const char* fragmentShaderSource =
"#version 330 core\n"
"uniform float pitchValue;\n"
"out vec4 FragColor;\n"
"void main() {\n"
"   if (pitchValue <= 0) {\n"
"       FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n" // Red for negative pitch
"   } else {\n"
"       FragColor = vec4(0.0, 1.0, 0.0, 1.0);\n" // Green for positive pitch
"   }\n"
"}\0";

// 2D-GRAPH AND SPIRAL
// Generate and write textfiles for 2D-graph/spiral
void generateDataTextFile(const char* fnVertex, const char* fnData, const char* fnRaw, int pointsCount, GLfloat* vertices, 
                          float* initialValuesY, float* initialValuesZ, float minX, float maxX) 
{
    ofstream fileVertex(fnVertex);
    ofstream fileRaw(fnRaw);
    ofstream fileData(fnData);

    // Make sure the files are opened correctly
    if (!fileVertex.is_open() || !fileData.is_open() || !fileRaw.is_open()) {
        cerr << "Error trying to open files for writing." << endl;
        return;
    }

    // Write the number of points as the first line
    fileVertex << "Number of Points: " << pointsCount << endl;
    fileRaw << "Number of Points: " << pointsCount << endl;
    fileData << "Number of Points: " << pointsCount << endl;

    float xInc = (maxX - minX) / (pointsCount - 1);

    fileVertex << fixed << setprecision(6);
    fileData << fixed << setprecision(6);

    // Write each subsequent line with original y-values and vertex data
    for (int i = 0; i < pointsCount; i++) 
    {
        float x1 = vertices[i * 3];
        float y1 = vertices[i * 3 + 1];
        float z1 = vertices[i * 3 + 2];

        float x2 = vertices[(i + 1) * 3];
        float y2 = vertices[(i + 1) * 3 + 1];
        float z2 = vertices[(i + 1) * 3 + 2];

        // Calculate Newtonian quotient (pitch)
        float pitchValue = (y2 - y1) / (x2 - x1);

        // Writing each line of each file
        fileVertex << i + 1 << ":\t x: " << x1 << "\ty: " << y1 << "\tz: " << z1 << endl;
        fileRaw << x1 << ", " << y1 << ", " << z1 << endl;
        fileData << "x: " << minX + (i * xInc) << "\ty: " << initialValuesY[i] << "\tPitch: " << pitchValue << endl;
    }

    fileVertex.close();
    fileData.close();
}

// Draw method for 2D-lines
// Draws the 2D-graph/spiral
void draw2DGraphSpiralLines(GLuint VAO, int pointsCount, GLuint shaderProgram, GLint pitchValueLocation, GLfloat* vertices) 
{
    // Use the shader program
    glUseProgram(shaderProgram);

    // Draw each section separately
    for (int i = 0; i < pointsCount - 1; ++i) 
    {
        float x1 = vertices[i * 3];
        float y1 = vertices[i * 3 + 1];
        float z1 = vertices[i * 3 + 2];

        float x2 = vertices[(i + 1) * 3];
        float y2 = vertices[(i + 1) * 3 + 1];
        float z2 = vertices[(i + 1) * 3 + 2];

        // Calculate Newtonian quotient (pitch)
        float pitchValue = (y2 - y1) / (x2 - x1);

        // Set the uniform pitch value for the current section
        glUniform1f(pitchValueLocation, pitchValue);

        // Draw the current section
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINE_STRIP, i, 2); // Draw two vertices to form a line section
        glBindVertexArray(0);
    }
}

// 2D-GRAPH 
// Calculation of 2D-graph (z-value is set to 0)
void calculate2DGraph(GLfloat* vertices, float minX, float maxX, int pointsCount, function<float(float)> equation) 
{
    float xSpan = maxX - minX;
    float xOffset = (maxX + minX) / 2.0f;

    float minY = numeric_limits<float>::infinity();
    float maxY = -numeric_limits<float>::infinity();

    // Create a vector to save original y-values
    vector<float> initialValuesY(pointsCount);

    // Calculate ySpan and find minY and maxY
    for (int i = 0; i < pointsCount; i++) 
    {
        float x = minX + static_cast<float>(i) / static_cast<float>(pointsCount - 1) * xSpan;
        float y = equation(x);  // Use the given equation
        
        initialValuesY[i] = y;  // Save original y-values

        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    float ySpan = maxY - minY;


    // Calculation of vertices
    for (int i = 0; i < pointsCount; i++) 
    {
        float x = minX + static_cast<float>(i) / static_cast<float>(pointsCount - 1) * xSpan;
        float y = equation(x);      // Uses the given equation
        float z = 0.0f;             // Left as 0 since graph is 2D

        vertices[i * 3] = (x - xOffset) / (xSpan / 2);             // Normalize x within (-1, 1)
        vertices[i * 3 + 1] = 2.0f * ((y - minY) / ySpan) - 1.0f;  // Normalize y within (-1, 1)
        vertices[i * 3 + 2] = z;
    }

    // Call generateDataTextFile
    generateDataTextFile("vertexData.txt", "functionData.txt", "vertexRaw.txt", pointsCount, vertices, 
                          initialValuesY.data(), 0, minX, maxX);
}

// 3D-SPIRAL
// Calculation of vertex coordinatess for the 3D-spiral
void calculateSpiral(GLfloat* vertices, int pointsCount) 
{
    float radius = 8.0f * atan(1);    // radius
    float dt = radius / (pointsCount / 2);

    // Create a vector to save original y-values
    vector<float> initialValuesY(pointsCount);
    vector<float> initialValuesZ(pointsCount);

    // Calculation of vertices for a 3D spiral
    for (int i = 0; i < pointsCount; ++i) 
    {
        float t = i * dt;

        float x = cos(t);
        float y = sin(t);
        float z = t / radius - 1;

        initialValuesY[i] = y;
        initialValuesZ[i] = z;

        // Assign x, y and z coordinates to vertices
        // By switching z and y, we can see the spiral from a different angle
        vertices[i * 3] = x;
        vertices[i * 3 + 1] = z;
        vertices[i * 3 + 2] = y;
    }

    // Run generateDataTextFile
    generateDataTextFile("vertexData.txt", "functionData.txt", "vertexRaw.txt", pointsCount, vertices, 
                          initialValuesY.data(), initialValuesZ.data(), -1, 1);
}

// 3D GRAPH
// Generate and write textfiles for 3D graph
void generateDataTextFile3D(const char* fnVertex, const char* fnRaw, int pointsCount, GLfloat* vertices, 
                            float* initialValuesY, float* initialValuesZ, float minX, float maxX) 
{
    ofstream fileVertex(fnVertex);
    ofstream fileRaw(fnRaw);

    // Make sure the files are opened correctly
    if (!fileVertex.is_open() || !fileRaw.is_open()) {
        cerr << "Error trying to open files for writing." << endl;
        return;
    }

    // Write the number of points as the first line
    // Because it is 2 variables, and there is a for-loop inside another for-loop,
    // the number of points is equal to pointsCount^2
    fileVertex << "Number of Points: " << pointsCount * pointsCount << endl;
    fileRaw << "Number of Points: " << pointsCount * pointsCount << endl;

    fileVertex << fixed << setprecision(6);

    // Write each subsequent line with original y-values and vertex data
    for (int i = 0; i < pointsCount * pointsCount; i++) 
    {
        float x1 = vertices[i * 3];
        float y1 = vertices[i * 3 + 1];
        float z1 = vertices[i * 3 + 2];

        float x2 = vertices[(i + 1) * 3];
        float y2 = vertices[(i + 1) * 3 + 1];
        float z2 = vertices[(i + 1) * 3 + 2];


        fileVertex << i + 1 << ":\t x: " << x1 << "\ty: " << y1 << "\tz: " << z1 << endl;
        fileRaw << x1 << ", " << y1 << ", " << z1 << endl;
    }

    fileVertex.close();
}

// Draw method for lines in a 3D-space
// Works for 3D-graphs (functions of 2 variables)
void draw3DGraphLines(GLuint VAO, int pointsCount, GLuint shaderProgram, GLint pitchValueLocation, GLfloat* vertices) 
{
    // Use the shader program
    glUseProgram(shaderProgram);

    // Draw each section separately
    for (int i = 0; i < pointsCount - 2; ++i) 
    {
        float x1 = vertices[i * 3];
        float y1 = vertices[i * 3 + 1];
        float z1 = vertices[i * 3 + 2];

        float x2 = vertices[(i + 1) * 3];
        float y2 = vertices[(i + 1) * 3 + 1];
        float z2 = vertices[(i + 1) * 3 + 2];

        // Calculate Newtonian quotient (pitch)
        float pitchValue = (y2 - y1) / (x2 - x1);

        // Set the uniform pitch value for the current section
        glUniform1f(pitchValueLocation, pitchValue);

        // Draw the current section
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINE_STRIP, i, 2); // Draw two vertices to form a line section
        glBindVertexArray(0);
    }
}

// 3D-GRAPH
// Calculation of 3D-graph
void calculate3DGraph(GLfloat* vertices, int pointsCount, function<float(float, float)> equation) 
{
    float minX = -1.0f;
    float maxX = 1.0f;
    float minY = -1.0f;
    float maxY = 1.0f;

    // Calculation of vertices
    int index = 0;
    for (int i = 0; i < pointsCount; ++i) 
    {
        for (int j = 0; j < pointsCount; ++j) 
        {
            float x = minX + static_cast<float>(i) / static_cast<float>(pointsCount - 1) * (maxX - minX);
            float y = minY + static_cast<float>(j) / static_cast<float>(pointsCount - 1) * (maxY - minY);
            float z = equation(x, y);  // Use the given equation

            // Assign x, y and z coordinates to vertices
            // By switching up x, y and z, we can see the 3D-graph from a different angle
            vertices[index * 3] = x;
            vertices[index * 3 + 1] = z;
            vertices[index * 3 + 2] = y;

            ++index;
        }
    }

    // Call generateDataTextFile
    generateDataTextFile3D("vertexData.txt", "vertexRaw.txt", pointsCount, vertices, 0, 0, -1, 1);

}

int main()
{
    // Initializing GLFW
    glfwInit();

    // Tell GLFW OpenGl Version (OpenGL 3.3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Tell GLFW OpenGL Profile (CORE)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window with dimensions 800x800, window title, fullscreen yes/no, last option NULL because its not relevant here
    GLFWwindow* window = glfwCreateWindow(800, 800, "Compulsory 1", NULL, NULL);

    // Terminate GLFW if last code fails
    if (window == NULL) 
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    // Tell GLFW to use the window
    glfwMakeContextCurrent(window);

    // Load GLAD so it configures OpenGL
    gladLoadGL();

    // Specify dimensions of viewport for OpenGL
    glViewport(0, 0, 800, 800);


    // Graph values
    const int pointsCount = 50;
    GLfloat vertices[3 * (pointsCount * pointsCount)];
    float minX = -10.0f;
    float maxX = 10.0f;

    // Function for 2D-space
    auto equation2D = [](float x) { return (x * x); }; // x^2

    // Function for 3D-space
    auto equation3D = [](float x, float y) { return (x * y); }; // x * y


    // WHEN DECIDING BETWEEN 2D-GRAPH, SPIRAL AND 3D-GRAPH, ALSO SWITCH DRAWING METHOD IN THE MAIN WHILE LOOP FURTHER DOWN
    // Uncomment the calculation-function you wish to see
    
    // Calculate vertex coordinatess for 2D-graph
    calculate2DGraph(vertices, minX, maxX, pointsCount, equation2D);

    // Calculate vertex coordinates for 3D-spiral
    //calculateSpiral(vertices, pointsCount);

    // Calculate vertex coordinates for 3D-graph
    //calculate3DGraph(vertices, pointsCount, equation3D);
    // ____________________________________________________________________________________________________________________


    // Create Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind VAO
    glBindVertexArray(VAO);

    // Bind VBO and copy data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Unbind VAO and VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Create shader program, attach shaders, and link
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check the pitchValue variable location
    GLint pitchValueLocation = glGetUniformLocation(shaderProgram, "pitchValue");
    if (pitchValueLocation == -1) 
    {
        cout << "Unable to get the pitchValue variable location." << endl;
    }

    // Delete shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Background color
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    // Clear the back buffer and give it the new color
    glClear(GL_COLOR_BUFFER_BIT);
    // Swap the back buffer with the front buffer
    glfwSwapBuffers(window);

    // Swap the back buffer with the front buffer
    glfwSwapBuffers(window);


    // Main while loop
    while (!glfwWindowShouldClose(window)) 
    {
        // Clear the back buffer and give it the new color
        glClear(GL_COLOR_BUFFER_BIT);

        glLineWidth(2);

        // DECIDE BETWEEN DRAWING METHOS FOR 2D-/SPIRAL AND 3D-GRAPHS
        // Uncomment the drawing method you want to see
        
        // Drawing method for 2D-graph and spiral
        draw2DGraphSpiralLines(VAO, pointsCount, shaderProgram, pitchValueLocation, vertices);

        // Drawing method for 3D-graph
        //draw3DGraphLines(VAO, pointsCount*pointsCount, shaderProgram, pitchValueLocation, vertices);
        // __________________________________________________________

        // Swap the back buffer with the front buffer
        glfwSwapBuffers(window);

        // Take care of all GLFW events
        glfwPollEvents();
    }

    // Delete window and terminate GLFW before closing the program
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}