
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <thread>
#include "shader.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "texture.hpp"

 /**
  * @brief Returns x value inside range
  *
  * @param x - Value to constrict to range
  * @param min - minimum allowed value for x
  * @param max - maximum allowed value for x
  * 
  * @returns x value between min and max
  */
float
Clamp(float x, float min, float max) {
    return x < min ? min : x > max ? max : x;
}

int WindowWidth = 800;
int WindowHeight = 800;
const float TargetFPS = 60.0f;
const std::string WindowTitle = "Okean";

struct Input {
    bool MoveLeft;
    bool MoveRight;
    bool MoveUp;
    bool MoveDown;
    bool LookLeft;
    bool LookRight;
    bool LookUp;
    bool LookDown;
};

struct EngineState {
    Input* mInput;
    Camera* mCamera;
    unsigned mShadingMode;
    bool mDrawDebugLines;
    float mDT;
};

/**
 * @brief Error callback function for GLFW. See GLFW docs for details
 *
 * @param error Error code
 * @param description Error message
 */
static void
ErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error: " << description << std::endl;
}

/**
 * @brief Keyboard callback function for GLFW. See GLFW docs for details
 *
 * @param window GLFW window context object
 * @param key Triggered key GLFW code
 * @param scancode Triggered key scan code
 * @param action Triggered key action: pressed, released or repeated
 * @param mode Modifiers
 */
static void
KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    EngineState* State = (EngineState*)glfwGetWindowUserPointer(window);
    Input* UserInput = State->mInput;
    bool IsDown = action == GLFW_PRESS || action == GLFW_REPEAT;
    switch (key) {
    case GLFW_KEY_A: UserInput->MoveLeft = IsDown; break;
    case GLFW_KEY_D: UserInput->MoveRight = IsDown; break;
    case GLFW_KEY_W: UserInput->MoveUp = IsDown; break;
    case GLFW_KEY_S: UserInput->MoveDown = IsDown; break;

    case GLFW_KEY_RIGHT: UserInput->LookLeft = IsDown; break;
    case GLFW_KEY_LEFT: UserInput->LookRight = IsDown; break;
    case GLFW_KEY_UP: UserInput->LookUp = IsDown; break;
    case GLFW_KEY_DOWN: UserInput->LookDown = IsDown; break;

  

    case GLFW_KEY_L: {
        if (IsDown) {
            State->mDrawDebugLines ^= true; break;
        }
    } break;

    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    }
}

static void
FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    WindowWidth = width;
    WindowHeight = height;
    glViewport(0, 0, width, height);
}

/**
 * @brief Updates engine state based on input
 * 
 * @param state EngineState
 */
static void
HandleInput(EngineState* state) {
    Input* UserInput = state->mInput;
    Camera* FPSCamera = state->mCamera;
    if (UserInput->MoveLeft) FPSCamera->Move(-1.0f, 0.0f, state->mDT);
    if (UserInput->MoveRight) FPSCamera->Move(1.0f, 0.0f, state->mDT);
    if (UserInput->MoveDown) FPSCamera->Move(0.0f, -1.0f, state->mDT);
    if (UserInput->MoveUp) FPSCamera->Move(0.0f, 1.0f, state->mDT);

    if (UserInput->LookLeft) FPSCamera->Rotate(1.0f, 0.0f, state->mDT);
    if (UserInput->LookRight) FPSCamera->Rotate(-1.0f, 0.0f, state->mDT);
    if (UserInput->LookDown) FPSCamera->Rotate(0.0f, -1.0f, state->mDT);
    if (UserInput->LookUp) FPSCamera->Rotate(0.0f, 1.0f, state->mDT);
}


int main() {
    GLFWwindow* Window = 0;
    if (!glfwInit()) {
        std::cerr << "Failed to init glfw" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(WindowWidth, WindowHeight, WindowTitle.c_str(), 0, 0);
    if (!Window) {
        std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(Window);

    GLenum GlewError = glewInit();
    if (GlewError != GLEW_OK) {
        std::cerr << "Failed to init glew: " << glewGetErrorString(GlewError) << std::endl;
        glfwTerminate();
        return -1;
    }

    EngineState State = { 0 };
    Camera FPSCamera;
    Input UserInput = { 0 };
    State.mCamera = &FPSCamera;
    State.mInput = &UserInput;
    glfwSetWindowUserPointer(Window, &State);

    glfwSetErrorCallback(ErrorCallback);
    glfwSetFramebufferSizeCallback(Window, FramebufferSizeCallback);
    glfwSetKeyCallback(Window, KeyCallback);

    glViewport(0.0f, 0.0f, WindowWidth, WindowHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
  
    unsigned WaterDiffuseTexture = Texture::LoadImageToTexture("res/voda.jpg");
    unsigned WaterSpecularTexture = Texture::LoadImageToTexture("res/WaterSpec.jpg");
    unsigned IceDiffuseTexture = Texture::LoadImageToTexture("res/Ice.jfif");
    unsigned IceSpecularTexture = Texture::LoadImageToTexture("res/icescratched.jpg");

    std::vector<float> CubeVertices = {
        // X     Y     Z     NX    NY    NZ    U     V    FRONT SIDE
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // R U
        -0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // L U
                                                        // LEFT SIDE
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
        -0.5f, -0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f,  0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.5f,  0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // RIGHT SIDE
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // R U
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // BOTTOM SIDE
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.5f, -0.5f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // TOP SIDE
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // L D
         0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
         0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // R U
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // L U
                                                        // BACK SIDE
         0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // L D
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
         0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // R D
        -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // R U
         0.5f,  0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // L U
    };

    unsigned CubeVAO;
    glGenVertexArrays(1, &CubeVAO);
    glBindVertexArray(CubeVAO);
    unsigned CubeVBO;
    glGenBuffers(1, &CubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, CubeVertices.size() * sizeof(float), CubeVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

   

    Model Penguin("res/penguin/penguinObj.obj");
    if (!Penguin.Load()) {
        std::cerr << "Failed to load peguin\n";
        glfwTerminate();
        return -1;
    }

    // NOTE: Used to only define color
    Shader ColorShader("shaders/color.vert", "shaders/color.frag");

 
    // NOTE: Phong shader with material and texture support
    Shader PhongShaderMaterialTexture("shaders/basic.vert", "shaders/phong_material_texture.frag");
    glUseProgram(PhongShaderMaterialTexture.GetId());
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Position", glm::vec3(0.0f, 4.5f, -2.0f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Direction", glm::vec3(1.0f, -1.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ka", glm::vec3(0.6f, 0.6f, 0.6f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Kd", glm::vec3(0.6f, 0.6f, 0.6f));
    PhongShaderMaterialTexture.SetUniform3f("uDirLight.Ks", glm::vec3(1.0f));

    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ka", glm::vec3(1.0f, 0.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Kd", glm::vec3(1.0f, 0.0f, 0.0f));
    PhongShaderMaterialTexture.SetUniform3f("uPointLight.Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kl", 0.50f);
    PhongShaderMaterialTexture.SetUniform1f("uPointLight.Kq", 0.20f);
    
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Position", glm::vec3(0.0f, 4.5f, -2.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Direction", glm::vec3(0.0f, -4.5f, 2.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ka", glm::vec3(0.0f, 1.0f, 1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Kd", glm::vec3(0.0f, 1.0f, 1.0f));
    PhongShaderMaterialTexture.SetUniform3f("uSpotlight.Ks", glm::vec3(1.0f));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kc", 1.0f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kl", 0.092f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.Kq", 0.032f);
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.InnerCutOff", glm::cos(glm::radians(12.5f)));
    PhongShaderMaterialTexture.SetUniform1f("uSpotlight.OuterCutOff", glm::cos(glm::radians(17.5f)));
    // NOTE: Diminishes the light's diffuse component by half, tinting it slightly red
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Kd", 0);
    // NOTE: Makes the object really shiny
    PhongShaderMaterialTexture.SetUniform1i("uMaterial.Ks", 1);
    PhongShaderMaterialTexture.SetUniform1f("uMaterial.Shininess", 128.0f);
    glUseProgram(0);

    glm::mat4 Projection = glm::perspective(45.0f, WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
    glm::mat4 View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
    glm::mat4 ModelMatrix(1.0f);
    
    // NOTE: Current angle around Y axis, with regards to XZ plane at which the point light is situated at
    float Angle = 0.0f;
    // NOTE: Distance of point light from center of rotation
    float Distance = 4.0f;
    float TargetFrameTime = 1.0f / TargetFPS;
    float StartTime = glfwGetTime();
    float EndTime = glfwGetTime();
    glClearColor(0.1f, 0.1f, 0.2f, 0.0f);

    // NOTE: Currently used shader
    Shader* CurrentShader = &PhongShaderMaterialTexture;
    while (!glfwWindowShouldClose(Window)) {
        glfwPollEvents();
        HandleInput(&State);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // NOTE: In case of window resize, update projection. Bit bad for performance to do it every iteration.
        // If laggy, remove this line
        Projection = glm::perspective(45.0f, WindowWidth / (float)WindowHeight, 0.1f, 100.0f);
        View = glm::lookAt(FPSCamera.GetPosition(), FPSCamera.GetTarget(), FPSCamera.GetUp());
        StartTime = glfwGetTime();
        glUseProgram(CurrentShader->GetId());
        CurrentShader->SetProjection(Projection);
        CurrentShader->SetView(View);
        CurrentShader->SetUniform3f("uViewPos", FPSCamera.GetPosition());

        
         glm::vec3 SpotLightPosition(Distance * 0.1, 2.0f, 2.0f + Distance * sin(Angle));
        CurrentShader->SetUniform3f("uSpotlight.Position", SpotLightPosition);
        Angle += State.mDT;


        glm::vec3 IslandPosition(Distance * 0.1, -1.9, 4.0f + Distance * sin(Angle));
        
        //Crtnje veceg ostrva
        glm::mat4 ModelMatrix(1.0f);
        ModelMatrix = glm::translate(glm::mat4(1.0f), IslandPosition);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(8.0, 0.4, 3.2));
        CurrentShader->SetModel(ModelMatrix);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, IceDiffuseTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, IceSpecularTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        glm::vec3 IslandPosition1(Distance * cos(Angle), -1.9, -4.0f + Distance * 0.2);
        
        //Crtnje manjeg ostrva
        glm::mat4 ModelMatrix1(1.0f);
        ModelMatrix1 = glm::translate(glm::mat4(1.0f), IslandPosition1);
        ModelMatrix1 = glm::scale(ModelMatrix1, glm::vec3(7.0, 0.4, 2.8));
        CurrentShader->SetModel(ModelMatrix1);
        glActiveTexture(GL_TEXTURE0);
        glActiveTexture(GL_TEXTURE1);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);
        
        //Okean
        glm::mat4 ModelMatrix2(1.0f);
        ModelMatrix2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.2, -2.8, 4.0));
        ModelMatrix2 = glm::scale(ModelMatrix2, glm::vec3(20.0, 1.2, 18.0));
        CurrentShader->SetModel(ModelMatrix2);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, WaterDiffuseTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, WaterSpecularTexture);
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, CubeVertices.size() / 8);

        glm::vec3 PenguinMove(Distance * 0.1, -1.5, 4.0f + Distance * sin(Angle));
        
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, PenguinMove);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.020, 0.030, 0.025));
        ModelMatrix = glm::rotate(ModelMatrix, glm::radians(270.0f), glm::vec3(1.0, 0.0, 0.0));
        CurrentShader->SetModel(ModelMatrix);
        Penguin.Render();

      
        glUseProgram(ColorShader.GetId());
        ColorShader.SetProjection(Projection);
        ColorShader.SetView(View);
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 1.0f, -2.0f));
        ColorShader.SetModel(ModelMatrix);


        glm::vec3 PointLightPosition(0.0, 5.5, 2.0f);
        CurrentShader->SetUniform3f("uPointLight.Position", PointLightPosition);
        
        // NOTE: Draw point light
        ModelMatrix = glm::translate(ModelMatrix, PointLightPosition);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.5f));
        ColorShader.SetModel(ModelMatrix);
        glBindVertexArray(CubeVAO);
        ColorShader.SetUniform3f("uColor", glm::vec3(0.0f, 1.0f, 0.0f));
        glDrawArrays(GL_TRIANGLES, 0, 36);

       
        //Crtnje MESECA
       ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 4.5f, -2.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.0f));
        ColorShader.SetModel(ModelMatrix);
        ColorShader.SetUniform3f("uColor", glm::vec3(1.0f, 1.0f+cos(Angle), 0.5f+sin(Angle)));
        glBindVertexArray(CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);
        glUseProgram(0);
        glfwSwapBuffers(Window);

        // NOTE: Time management
        EndTime = glfwGetTime();
        float WorkTime = EndTime - StartTime;
        if (WorkTime < TargetFrameTime) {
            int DeltaMS = (int)((TargetFrameTime - WorkTime) * 1000.0f);
            std::this_thread::sleep_for(std::chrono::milliseconds(DeltaMS));
            EndTime = glfwGetTime();
        }
        State.mDT = EndTime - StartTime;
    }

    glfwTerminate();
    return 0;
}
