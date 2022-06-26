// Colton Stiff
// 06/12/2022
// CS-330: 7-1 Final Project
// SNHU

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnOpengl/camera.h> // Camera class

#include "cylinder.h"
#include "sphere.h"


using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Colton Stiff - 7-1 Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao[10];         // Handle for the vertex array object
        GLuint vbo[10];         // Handle for the vertex buffer object
        GLuint nVertices[10];    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Triangle mesh data
    GLMesh gMesh;

    // Texture
    GLuint deskTextureId, laptopTextureId, kbTextureId, mouseTextureId, cupTextureId;
    glm::vec2 gUVScale(1.0f, 1.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // Shader program
    GLuint gProgramId;
    GLuint gLaptopProgramId;
    GLuint gMouseProgramId;


    // camera
    Camera gCamera(glm::vec3(0.0f, 5.0f, 15.0f));
    float gLastX = WINDOW_WIDTH / 5.0f;
    float gLastY = WINDOW_HEIGHT / 5.0f;
    bool gFirstMouse = true;
    bool viewProjection = true;
    float cameraSpeed = 0.01f;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Light color, position, and scale
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 gLightPosition(0.0f, -0.63f, 0.01f);
    glm::vec3 gLightScale(1.9f);
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;


//Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)
    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
    vertexTextureCoordinate = textureCoordinate;
}
);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,

    in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for light color, light position, and camera/view position
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;

void main()
{
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

    //Calculate Ambient lighting*/
    float ambientStrength = 0.7f; // Set ambient or global lighting strength
    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

                                                                         //Calculate Diffuse lighting*/
    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
    float impact = max(dot(norm, lightDirection), 0.2);// Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; // Generate diffuse light color

                                                                //Calculate Specular lighting*/
    float specularIntensity = 0.2f; // Set specular light strength
    float highlightSize = 16.0f; // Set specular highlight size
    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
                                                     //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    // Texture holds the color to be used for all three components
    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

    // Calculate phong result
    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);

/* Laptop Shader Source Code*/
const GLchar* laptopVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

                                           //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

/* Mouse Shader Source Code*/
const GLchar* mouseVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

                                           //Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

// Laptop Fragment Shader Source Code
const GLchar* laptopFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing laptop color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Mouse Fragment Shader Source Code
const GLchar* mouseFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing mouse color (smaller cube) to the GPU

void main()
{
    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;
    if (!UCreateShaderProgram(laptopVertexShaderSource, laptopFragmentShaderSource, gLaptopProgramId))
        return EXIT_FAILURE;
    if (!UCreateShaderProgram(mouseVertexShaderSource, mouseFragmentShaderSource, gMouseProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* deskTexFilename = "resources/textures/desk.jpg";
    const char* laptopTexFilename = "resources/textures/silver.jpg";
    const char* kbTexFilename = "resources/textures/kb.jpg";
    const char* mouseTexFilename = "resources/textures/blue.jpg";
    const char* cupTexFilename = "resources/textures/cup.jpg";

    if (!UCreateTexture(deskTexFilename, deskTextureId))
    {
        cout << "Failed to load Desk texture " << deskTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(laptopTexFilename, laptopTextureId))
    {
        cout << "Failed to load laptop texture " << laptopTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(kbTexFilename, kbTextureId))
    {
        cout << "Failed to load texture " << kbTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(mouseTexFilename, mouseTextureId))
    {
        cout << "Failed to load texture " << mouseTexFilename << endl;
        return EXIT_FAILURE;
    }
    if (!UCreateTexture(cupTexFilename, cupTextureId))
    {
        cout << "Failed to load texture " << cupTexFilename << endl;
        return EXIT_FAILURE;
    }

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0

    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(deskTextureId);
    UDestroyTexture(laptopTextureId);
    UDestroyTexture(kbTextureId);
    UDestroyTexture(mouseTextureId);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLaptopProgramId);
    UDestroyShaderProgram(gMouseProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        viewProjection = !viewProjection;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset > 0 && cameraSpeed < 0.01f)
        cameraSpeed += 0.01f;
    if (yoffset < 0 && cameraSpeed > 0.01f)
        cameraSpeed -= 0.01f;
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button)
    {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
        if (action == GLFW_PRESS)
            cout << "Left mouse button pressed" << endl;
        else
            cout << "Left mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
        if (action == GLFW_PRESS)
            cout << "Middle mouse button pressed" << endl;
        else
            cout << "Middle mouse button released" << endl;
    }
    break;

    case GLFW_MOUSE_BUTTON_RIGHT:
    {
        if (action == GLFW_PRESS)
            cout << "Right mouse button pressed" << endl;
        else
            cout << "Right mouse button released" << endl;
    }
    break;

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}


// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set the shader to be used
    glUseProgram(gProgramId);

    // camera/view transformation
    glm::mat4 view = gCamera.GetViewMatrix();

    // Creates a projection or Ortho view
    glm::mat4 projection;
    if (viewProjection) {
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }
    else {
        float scale = 120;
        projection = glm::ortho((800.0f / scale), -(800.0f / scale), -(600.0f / scale), (600.0f / scale), -2.5f, 6.5f);
    }

#pragma region Laptop Binding / Generation

    float laptopScale = 0.2f;
    glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f));
    glm::mat4 model = translation * rotation * scale;

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint keyLightColorLoc = glGetUniformLocation(gProgramId, "keyLightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[0]);
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, laptopTextureId);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);

#pragma endregion

#pragma region Pencil Holder Binding / Generation

    // Pencil Holder
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cupTextureId);
    glBindVertexArray(gMesh.vao[3]);
    model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first		
    model = glm::translate(model, glm::vec3(-9.0f, -2.0f, 1.0f));
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    static_meshes_3D::Cylinder C1(1, 20, 3, true, true, true);
    C1.render();

#pragma endregion


#pragma region Mouse Ball Binding / Generation

    // Mouse Ball (Sphere)
    Sphere mouseBall(0.4f, 36, 18);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mouseTextureId);
    rotation = glm::rotate(45.0f, glm::vec3(0.0, 1.0f, 0.0f));
    translation = glm::translate(glm::vec3(9.0f, -2.7f, 2.0f));
    model = translation * rotation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    mouseBall.Draw();


#pragma endregion

#pragma region Desk Binding / Generation
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deskTextureId);
    scale = glm::scale(glm::vec3(50.0f, 7.0f, 25.0f));
    translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    model = translation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[1]);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[1]);

#pragma endregion

#pragma region Keyboard Binding / Generation

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, kbTextureId);
    scale = glm::scale(glm::vec3(2.0f, 0.0f, 1.85f));
    translation = glm::translate(glm::vec3(0.0f, -3.28f, 0.45f));
    model = translation * scale;
    modelLoc = glGetUniformLocation(gProgramId, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao[0]);
    // Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);

#pragma endregion

#pragma region Light Binding / Generation

    // Light source
    //----------------
    glUseProgram(gLaptopProgramId);

    //Transform the laptop backlight to used as a visual que for the light source
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the light Shader program
    modelLoc = glGetUniformLocation(gLaptopProgramId, "model");
    viewLoc = glGetUniformLocation(gLaptopProgramId, "view");
    projLoc = glGetUniformLocation(gLaptopProgramId, "projection");

    // Pass matrix data to the light Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(gMesh.vao[0]);
    glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices[0]);

#pragma endregion

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);

    // Adding Laptop
    // Vertex Data
    GLfloat laptopVerts[] = {
        //Positions             Normal Coordinates  Texture Coordinates
        // Top of Laptop / screen
        -3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        -3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,   0.0f, 0.0f,

        -3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

        -3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
        -3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
        -3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

         3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
         3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

         -3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f, -1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

        -3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f,  1.5f, -0.1f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f,  1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

        // Laptop bottom half / keyboaard				    
        -3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

        -3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
        -3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
        -3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

         3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f, -1.5f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
         3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

        -3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f, -1.4f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,

        -3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f,
         3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	1.0f, 0.0f,
         3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
         3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	1.0f, 1.0f,
        -3.0f, -1.4f, -0.2f,    0.0f, 0.0f, 0.0f,	0.0f, 1.0f,
        -3.0f, -1.5f,  3.0f,    0.0f, 0.0f, 0.0f,	0.0f, 0.0f

    };

    GLfloat planeVerts[] = {
        //Position				Normal Coords				Texture Coords
        -0.5f, -0.5f, -0.5f,	0.0f, 1.0f,  0.0f,			0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,	0.0f, 1.0f,  0.0f,			1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,	0.0f, 1.0f,  0.0f,			1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,	0.0f, 1.0f,  0.0f,			1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,	0.0f, 1.0f,  0.0f,			0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,	0.0f, 1.0f,  0.0f,			0.0f, 1.0f,
    };

    mesh.nVertices[0] = sizeof(laptopVerts) / (sizeof(laptopVerts[0]) * (floatsPerVertex + floatsPerUV));
    mesh.nVertices[1] = sizeof(planeVerts) / (sizeof(planeVerts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

#pragma region Laptop Mesh
    glGenVertexArrays(1, &mesh.vao[0]); // we can also generate multiple VAOs or buffers at the same time
    glGenBuffers(1, &mesh.vbo[0]);
    glBindVertexArray(mesh.vao[0]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(laptopVerts), laptopVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Desk Mesh
    glGenVertexArrays(1, &mesh.vao[1]);
    glGenBuffers(1, &mesh.vbo[1]);
    glBindVertexArray(mesh.vao[1]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[1]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVerts), planeVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Keyboard Mesh
    glGenVertexArrays(1, &mesh.vao[8]);
    glGenBuffers(1, &mesh.vbo[8]);
    glBindVertexArray(mesh.vao[8]);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[8]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(laptopVerts), laptopVerts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

                                                                                 // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);
#pragma endregion

#pragma region Mouse Meshes

    glGenVertexArrays(1, &mesh.vao[3]);
    glGenBuffers(1, &mesh.vbo[3]);
    glBindVertexArray(mesh.vao[3]);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo[3]);

#pragma endregion

}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(10, mesh.vao);
    glDeleteBuffers(10, mesh.vbo);
}


/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
