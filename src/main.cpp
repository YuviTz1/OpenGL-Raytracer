#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp> 

#include <iostream>
#include <chrono>
#include <thread>

#include "engine/engine.hpp"
#include "renderer/renderer.hpp"

double previousTime = glfwGetTime();
int frameCount = 0;

const unsigned int SCREEN_WIDTH = 1600;
const unsigned int SCREEN_HEIGHT = 900;

int main()
{
    Engine engine(SCREEN_WIDTH, SCREEN_HEIGHT, "Compute Shader Engine");
	Renderer renderer(SCREEN_WIDTH, SCREEN_HEIGHT);

    glfwSetCursorPosCallback(engine.m_window, Renderer::mouse_callback);
    glfwSetWindowUserPointer(engine.m_window, &renderer);

  //  while (!glfwWindowShouldClose(engine.m_window))
  //  {
  //      auto frameStart = std::chrono::high_resolution_clock::now();

  //      double currentTime = glfwGetTime();
  //      frameCount++;
  //      // If a second has passed.
  //      if ( currentTime - previousTime >= 1.0 )
  //      {
  //          // Display the frame count here any way you want.
  //          std::cout<<"FPS: "<< frameCount<<std::endl;

  //          frameCount = 0;
  //          previousTime = currentTime;
  //      }

  //      // clear screen with specified color
  //      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //      glClearColor(0.0782, 0.0782, 0.0782, 1);

  //      renderer.processInput(engine.m_window);

  //      // quadShader.use();
  //      // glBindVertexArray(VAO);
  //      // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  //      // glBindVertexArray(0);

  //      glBindBuffer(GL_UNIFORM_BUFFER, renderer.m_cameraUBO);
  //      glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(renderer.m_camData), &renderer.m_camData);

  //      renderer.m_computeShader.use_compute(ceil(SCREEN_WIDTH / 8), ceil(SCREEN_HEIGHT / 4), 1);

  //      renderer.m_QuadShader.use();
  //      glBindTextureUnit(0, renderer.m_screenTex);
  //      renderer.m_QuadShader.setInt("screen", 0);
		//glBindVertexArray(renderer.m_VAO);
		//glDrawElements(GL_TRIANGLES, sizeof(renderer.m_indices) / sizeof(renderer.m_indices[0]), GL_UNSIGNED_INT, 0);

  //      /* Swap front and back buffers */
  //      glfwSwapBuffers(engine.m_window);
  //      /* Poll for and process events */
  //      glfwPollEvents();

  //      // Frame limiting to 60 FPS
  //      auto frameEnd = std::chrono::high_resolution_clock::now();
  //      std::chrono::duration<double, std::milli> frameDuration = frameEnd - frameStart;
  //      double targetFrameTime = 1000/110;

  //      if (frameDuration.count() < targetFrameTime)
  //      {
  //          std::this_thread::sleep_for(
  //              std::chrono::duration<double, std::milli>(targetFrameTime - frameDuration.count())
  //          );
  //      }
  //  }

	engine.Run(renderer);
    return 0;
}