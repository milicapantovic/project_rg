#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

unsigned int loadCubemap(vector<std::string> faces);


void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};


struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 treePosition = glm::vec3(0.0f);
    glm::vec3 treePosition1 = glm::vec3(10.0f, 0.0f, 0.0f);
    glm::vec3 treePosition2 = glm::vec3(20.0f, 0.0f, 0.0f);

    float backpackScale = 1.0f;
    PointLight pointLight;
    DirLight dirLight;
    SpotLight spotLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    // load models
    // -----------
    Model ourModel("resources/objects/Tree/Tree.obj");
    Model ourModel1("resources/objects/mountain/avatar_mountain.obj");
    Model ourModel2("resources/objects/rocks/rocks.obj");
    Model ourModel3("resources/objects/grass/10450_Rectangular_Grass_Patch_v1_iterations-2.obj");
//    Model ourModel4("resources/objects/");
//    Model ourModel5("resources/objects/");
//    Model ourModel6("resources/objects/");

    ourModel.SetShaderTextureNamePrefix("material.");
    ourModel1.SetShaderTextureNamePrefix("material.");
    ourModel2.SetShaderTextureNamePrefix("material.");
    ourModel3.SetShaderTextureNamePrefix("material.");

    DirLight& dirLight = programState->dirLight;
    dirLight.direction = glm::vec3( -0.2f, -1.0f, -0.3f);
    dirLight.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    dirLight.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.8, 0.8, 0.8);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;



    SpotLight& spotLight = programState->spotLight;
    spotLight.position = programState->camera.Position;
    spotLight.direction = programState->camera.Front;
    spotLight.ambient = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;
    spotLight.cutOff = glm::cos(glm::radians(12.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(15.0f));


            float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    stbi_set_flip_vertically_on_load(false);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox1/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox1/back.jpg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();

        ourShader.setVec3("dirLight.direction", dirLight.direction);
        ourShader.setVec3("dirLight.ambient", dirLight.ambient);
        ourShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        ourShader.setVec3("dirLight.specular", dirLight.specular);

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        ourShader.setVec3("spotLight.position", spotLight.position);
        ourShader.setVec3("spotLight.direction", spotLight.direction);
        ourShader.setVec3("spotLight.ambient", spotLight.ambient);
        ourShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        ourShader.setVec3("spotLight.specular", spotLight.specular);
        ourShader.setFloat("spotLight.constant", spotLight.constant);
        ourShader.setFloat("spotLight.linear", spotLight.linear);
        ourShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        ourShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        ourShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        float time = glfwGetTime();
        // render the loaded model


        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               glm::vec3(-35.0f, -0.4f, 0.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(3.0f));    // it's a bit too big for our scene, so scale it down
        model = glm::rotate(model, (float)(time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        glm::mat4 model_1 = glm::mat4(1.0f);
        model_1 = glm::translate(model_1,
                                 glm::vec3(-35.0f, -0.4f, 30.0f)); // translate it down so it's at the center of the scene
        model_1 = glm::rotate(model_1, (float)(-time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        model_1 = glm::scale(model_1, glm::vec3(3.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model_1);
        ourModel.Draw(ourShader);

        glm::mat4 model1 = glm::mat4(1.0f);
        model1 = glm::translate(model1,
                               glm::vec3(35.0f, -0.4f, 0.0f)); // translate it down so it's at the center of the scene
        model1 = glm::scale(model1, glm::vec3(3.0f));    // it's a bit too big for our scene, so scale it down
        model1 = glm::rotate(model1, (float)(time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model1);
        ourModel.Draw(ourShader);

        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2,
                                glm::vec3 (35.0f, -0.4f, 30.0f)); // translate it down so it's at the center of the scene
        model2 = glm::rotate(model2, (float)(-time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        model2 = glm::scale(model2, glm::vec3(3.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model2);
        ourModel.Draw(ourShader);


        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4,
                                glm::vec3 (35.0f, -0.8f, 0.0f)); // translate it down so it's at the center of the scene
        model4 = glm::rotate(model4, (float)(time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        model4 = glm::scale(model4, glm::vec3(2.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model4);
        ourModel1.Draw(ourShader);



        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3,
                                glm::vec3 (35.0f, -0.8f, 30.0f)); // translate it down so it's at the center of the scene
        model3 = glm::rotate(model3, (float)(-time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        model3 = glm::scale(model3, glm::vec3(2.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model3);
        ourModel1.Draw(ourShader);



        glm::mat4 model_4 = glm::mat4(1.0f);
        model_4 = glm::translate(model_4,
                                glm::vec3 (-35.0f, -0.8f, 0.0f)); // translate it down so it's at the center of the scene
        model_4 = glm::rotate(model_4, (float)(time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        model_4 = glm::scale(model_4, glm::vec3(2.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model_4);
        ourModel1.Draw(ourShader);

        glm::mat4 model5 = glm::mat4(1.0f);
        model5 = glm::translate(model5,
                                glm::vec3 (-35.0f, -0.8f, 30.0f)); // translate it down so it's at the center of the scene
        model5 = glm::rotate(model5, (float)(-time / 0.7) + 2, glm::vec3(0.0f, 1.0f, 0.0f));
        model5 = glm::scale(model5, glm::vec3(2.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model5);
        ourModel1.Draw(ourShader);

        glm::mat4 model6 = glm::mat4(1.0f);
        model6 = glm::translate(model6,
                                glm::vec3 (0.0f, 0.0f, 15.0f)); // translate it down so it's at the center of the scene
//        model6 = glm::rotate(model6, 1.6f, glm::vec3(0.0f, 1.0f, 0.0f));
//        model6 = glm::rotate(model6, -1.6f, glm::vec3(1.0f, 0.0f, 0.0f));
        model6 = glm::scale(model6, glm::vec3(10.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model6);
        ourModel.Draw(ourShader);

        glm::mat4 model7 = glm::mat4(1.0f);
        model7 = glm::translate(model7,
                                glm::vec3 (0.0f, -10.0f, 10.0f)); // translate it down so it's at the center of the scene
//        model6 = glm::rotate(model7, 1.6f, glm::vec3(0.0f, 1.0f, 0.0f));
        model7 = glm::rotate(model7, -1.6f, glm::vec3(1.0f, 0.0f, 0.0f));
        model7 = glm::scale(model7, glm::vec3(0.3f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model7);
        ourModel3.Draw(ourShader);

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height,
                                        &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap failed to load at path: " << faces[i]
                      << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T,
                    GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R,
                    GL_CLAMP_TO_EDGE);
    return textureID;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->treePosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

}
