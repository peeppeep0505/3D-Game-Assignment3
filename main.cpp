#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <learnopengl/filesystem.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

// ----------------------------
// Window settings
// ----------------------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// ----------------------------
// Timing
// ----------------------------
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ----------------------------
// Mouse
// ----------------------------
float lastX = SCR_WIDTH * 0.5f;
float lastY = SCR_HEIGHT * 0.5f;
bool firstMouse = true;

// ----------------------------
// Camera
// ----------------------------
Camera camera(glm::vec3(0.0f, 6.0f, 10.0f));
float cameraPitch = -20.0f;
float cameraDistance = 10.0f;

// ----------------------------
// Player
// ----------------------------
glm::vec3 playerPosition(0.0f, 0.3f, 0.0f);
float playerYaw = 180.0f; 
float playerSpeed = 6.0f;
float turnSpeed = 120.0f; 
float playerRadius = 1.0f;

// ----------------------------
// Game
// ----------------------------
int score = 0;
const int WIN_SCORE = 5;
bool gameWon = false;
bool gameOver = false;

struct Crystal
{
    glm::vec3 position;
    bool collected = false;
    float radius = 2.0f;
};

struct Enemy
{
    glm::vec3 startPosition;
    glm::vec3 position;
    float radius = 1.5f;
    float moveRange = 6.0f;
    float speed = 1.5f;
    int moveAxis = 0; // 0 = X, 1 = Z
    float phase = 0.0f;
};

std::vector<Crystal> crystals;
std::vector<Enemy> enemies;

// ----------------------------
// Function declarations
// ----------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void processInput(GLFWwindow* window);
void updateCamera();
bool checkSphereCollision(const glm::vec3& a, float ra, const glm::vec3& b, float rb);

void initCrystals();
void initEnemies();
void updateEnemies(float time);
void updateWindowTitle(GLFWwindow* window);
void resetGame(GLFWwindow* window);

// ----------------------------
// Main
// ----------------------------
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Crystal Collector", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader modelShader("1.model_loading.vs", "1.model_loading.fs");

    Model playerModel(FileSystem::getPath("resources/objects/chiikawa/chiikawa.obj"));
    Model sceneModel(FileSystem::getPath("resources/objects/SnowTerrain/SnowTerrain.obj"));
    Model crystalModel(FileSystem::getPath("resources/objects/crystal/stylized_crystal_SM.obj"));
    Model enemyModel(FileSystem::getPath("resources/objects/snowman/11581_Snowman_V2_l3.obj"));

    initCrystals();
    initEnemies();
    updateWindowTitle(window);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        if (!gameWon && !gameOver)
        {
            updateEnemies(currentFrame);

            for (auto& crystal : crystals)
            {
                if (!crystal.collected &&
                    checkSphereCollision(playerPosition, playerRadius, crystal.position, crystal.radius))
                {
                    crystal.collected = true;
                    score++;

                    if (score >= WIN_SCORE)
                        gameWon = true;

                    updateWindowTitle(window);
                }
            }

            for (const auto& enemy : enemies)
            {
                if (checkSphereCollision(playerPosition, playerRadius, enemy.position, enemy.radius))
                {
                    gameOver = true;
                    updateWindowTitle(window);
                    break;
                }
            }
        }

        updateCamera();

        glClearColor(0.75f, 0.85f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        modelShader.use();

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            1000.0f
        );
        glm::mat4 view = camera.GetViewMatrix();

        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);

        // ----------------------------
        // Draw scene
        // ----------------------------
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -1.8f, 0.0f));
            model = glm::scale(model, glm::vec3(3.0f));

            modelShader.setMat4("model", model);
            sceneModel.Draw(modelShader);
        }

        // ----------------------------
        // Draw player
        // ----------------------------
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, playerPosition);
            model = glm::rotate(model, glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // เผื่อโมเดลหันกลับด้าน
            model = glm::scale(model, glm::vec3(0.8f));

            modelShader.setMat4("model", model);
            playerModel.Draw(modelShader);
        }

        // ----------------------------
        // Draw crystals
        // ----------------------------
        for (size_t i = 0; i < crystals.size(); i++)
        {
            if (crystals[i].collected) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, crystals[i].position);
            model = glm::scale(model, glm::vec3(2.4f));

            modelShader.setMat4("model", model);
            crystalModel.Draw(modelShader);
        }

        // ----------------------------
        // Draw enemies
        // ----------------------------
        for (const auto& enemy : enemies)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, enemy.position);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(1.1f));

            modelShader.setMat4("model", model);
            enemyModel.Draw(modelShader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ----------------------------
// Input
// ----------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if ((gameWon || gameOver) && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        resetGame(window);
        return;
    }

    if (gameWon || gameOver)
        return;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        playerYaw += turnSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        playerYaw -= turnSpeed * deltaTime;

    glm::vec3 forward;
    forward.x = sin(glm::radians(playerYaw));
    forward.y = 0.0f;
    forward.z = cos(glm::radians(playerYaw));
    forward = glm::normalize(forward);

    glm::vec3 moveDir(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDir += forward;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDir -= forward;

    if (glm::length(moveDir) > 0.001f)
    {
        moveDir = glm::normalize(moveDir);

        glm::vec3 nextPos = playerPosition + moveDir * playerSpeed * deltaTime;

        const float minX = -22.0f;
        const float maxX = 22.0f;
        const float minZ = -22.0f;
        const float maxZ = 22.0f;

        if (nextPos.x > minX && nextPos.x < maxX &&
            nextPos.z > minZ && nextPos.z < maxZ)
        {
            playerPosition = nextPos;
        }
    }
}

// ----------------------------
// Camera update
// ----------------------------
void updateCamera()
{
    glm::vec3 target = playerPosition + glm::vec3(0.0f, 2.0f, 0.0f);

    float camYaw = playerYaw;
    glm::vec3 offset;
    offset.x = cameraDistance * cos(glm::radians(cameraPitch)) * sin(glm::radians(camYaw));
    offset.y = cameraDistance * sin(glm::radians(cameraPitch));
    offset.z = cameraDistance * cos(glm::radians(cameraPitch)) * cos(glm::radians(camYaw));

    camera.Position = target - offset;
    camera.Front = glm::normalize(target - camera.Position);
    camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0.0f, 1.0f, 0.0f)));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
}

// ----------------------------
// Mouse
// ----------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.08f;
    yoffset *= sensitivity;

    cameraPitch += yoffset;

    if (cameraPitch > 20.0f)  cameraPitch = 20.0f;
    if (cameraPitch < -55.0f) cameraPitch = -55.0f;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraDistance -= (float)yoffset;

    if (cameraDistance < 5.0f)  cameraDistance = 5.0f;
    if (cameraDistance > 16.0f) cameraDistance = 16.0f;
}

// ----------------------------
// Utilities
// ----------------------------
bool checkSphereCollision(const glm::vec3& a, float ra, const glm::vec3& b, float rb)
{
    float dist = glm::length(a - b);
    return dist < (ra + rb);
}

void initCrystals()
{
    crystals.clear();

    crystals.push_back({ glm::vec3(4.0f, 0.6f, -3.0f), false, 2.0f });
    crystals.push_back({ glm::vec3(-6.0f, 0.6f,  5.0f), false, 2.0f });
    crystals.push_back({ glm::vec3(8.0f, 0.6f,  8.0f), false, 2.0f });
    crystals.push_back({ glm::vec3(-10.0f, 0.6f, -7.0f), false, 2.0f });
    crystals.push_back({ glm::vec3(12.0f, 0.6f,  2.0f), false, 2.0f });
}

void initEnemies()
{
    enemies.clear();

    Enemy e1;
    e1.startPosition = glm::vec3(0.0f, 0.4f, 10.0f);
    e1.position = e1.startPosition;
    e1.radius = 1.5f;
    e1.moveRange = 8.0f;
    e1.speed = 1.8f;
    e1.moveAxis = 0;
    e1.phase = 0.0f;
    enemies.push_back(e1);

    Enemy e2;
    e2.startPosition = glm::vec3(-12.0f, 0.4f, -2.0f);
    e2.position = e2.startPosition;
    e2.radius = 1.5f;
    e2.moveRange = 6.0f;
    e2.speed = 1.4f;
    e2.moveAxis = 1;
    e2.phase = 1.2f;
    enemies.push_back(e2);

    Enemy e3;
    e3.startPosition = glm::vec3(10.0f, 0.4f, -10.0f);
    e3.position = e3.startPosition;
    e3.radius = 1.5f;
    e3.moveRange = 5.0f;
    e3.speed = 2.0f;
    e3.moveAxis = 0;
    e3.phase = 2.0f;
    enemies.push_back(e3);
}

void updateEnemies(float time)
{
    for (auto& enemy : enemies)
    {
        enemy.position = enemy.startPosition;

        float offset = sin(time * enemy.speed + enemy.phase) * enemy.moveRange;

        if (enemy.moveAxis == 0)
            enemy.position.x += offset;
        else
            enemy.position.z += offset;
    }
}

void resetGame(GLFWwindow* window)
{
    score = 0;
    gameWon = false;
    gameOver = false;

    playerPosition = glm::vec3(0.0f, 0.3f, 0.0f);
    playerYaw = 180.0f;

    cameraPitch = -20.0f;
    cameraDistance = 10.0f;

    initCrystals();
    initEnemies();
    updateWindowTitle(window);
}

void updateWindowTitle(GLFWwindow* window)
{
    std::stringstream ss;

    if (gameWon)
        ss << "Crystal Collector | You Win! Press R to Reset";
    else if (gameOver)
        ss << "Crystal Collector | Hit by Enemy! Press R to Restart";
    else
        ss << "Crystal Collector | Score: " << score << " / " << WIN_SCORE;

    glfwSetWindowTitle(window, ss.str().c_str());
}

// ----------------------------
// GLFW callbacks
// ----------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}