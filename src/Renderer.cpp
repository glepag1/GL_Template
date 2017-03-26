#include <stdio.h>
#include <iostream>
#include <vector>
// glm additional header to generate transformation matrices directly.
#include <glm/gtc/matrix_transform.hpp>
#include <cstring> // For memcopy depending on the platform.

#include "helpers/ProgramUtilities.h"
#include "Renderer.h"


Renderer::~Renderer(){}

Renderer::Renderer(int width, int height){

	// Initialize the timer.
	_timer = glfwGetTime();
	// Initialize random generator;
	Random::seed();
	// Setup projection matrix.
	_camera.screen(width, height);
	
	
	_gbuffer = std::make_shared<Gbuffer>(_camera._renderSize[0],_camera._renderSize[1]);
	_sceneFramebuffer = std::make_shared<Framebuffer>(_camera._renderSize[0],_camera._renderSize[1], GL_RGBA,GL_UNSIGNED_BYTE,GL_LINEAR,GL_CLAMP_TO_EDGE);
	_fxaaFramebuffer = std::make_shared<Framebuffer>(_camera._renderSize[0],_camera._renderSize[1], GL_RGBA,GL_UNSIGNED_BYTE,GL_LINEAR,GL_CLAMP_TO_EDGE);
	
	// Create directional light.
	_directionalLights.emplace_back(glm::vec3(0.0f), glm::vec3(0.9f), glm::ortho(-0.75f,0.75f,-0.75f,0.75f,2.0f,6.0f));
	
	// Create point lights.
	const float lI = 6.0; // Light intensity.
	std::vector<glm::vec3> colors = { glm::vec3(lI,0.0,0.0), glm::vec3(0.0,lI,0.0), glm::vec3(0.0,0.0,lI), glm::vec3(lI,lI,0.0), glm::vec3(lI,0.0,lI), glm::vec3(0.0,lI,lI), glm::vec3(lI,lI,lI), glm::vec3(lI,lI/2.0,lI/3.0), glm::vec3(lI/3.0,lI,lI/2.0), glm::vec3(lI/2.0,lI/3.0,lI)};
	
	for(size_t i = 0; i < 64; ++i){
		// 8 by 8 grid, from -2 to 2 on both axis.
		float radius = Random::Float(0.3f, 0.6f);
		glm::vec3 position = glm::vec3(-2.0f + 4.0*float(i%9)/8.0f, Random::Float(-0.25f, 0.0f), -2.0f + 4.0*float(i/9)/8.0f );
		_pointLights.emplace_back(position, colors[Random::Int(0, 9)], radius);
	}
	
	PointLight::loadProgramAndGeometry();
	
	// Query the renderer identifier, and the supported OpenGL version.
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	std::cout << "Renderer: " << renderer << std::endl;
	std::cout << "OpenGL version supported: " << version << std::endl;
	checkGLError();

	// GL options
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation (GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);
	checkGLError();

	// Initialize objects.
	const std::vector<std::string> texturesSuzanne = { "ressources/suzanne_texture_color.png", "ressources/suzanne_texture_normal.png", "ressources/suzanne_texture_ao_specular_reflection.png", "ressources/cubemap/cubemap", "ressources/cubemap/cubemap_diff" };
	_suzanne.init("ressources/suzanne.obj", texturesSuzanne, 1);
	
	const std::vector<std::string> texturesDragon = {"ressources/dragon_texture_color.png", "ressources/dragon_texture_normal.png", "ressources/dragon_texture_ao_specular_reflection.png", "ressources/cubemap/cubemap", "ressources/cubemap/cubemap_diff"  };
	_dragon.init("ressources/dragon.obj", texturesDragon,  1);
	
	const std::vector<std::string> texturesPlane = { "ressources/plane_texture_color.png", "ressources/plane_texture_normal.png", "ressources/plane_texture_depthmap.png", "ressources/cubemap/cubemap", "ressources/cubemap/cubemap_diff" };
	_plane.init("ressources/plane.obj", texturesPlane,  2);
	
	_skybox.init();
	
	_ambientScreen.init(_gbuffer->textureIds({ TextureType::Albedo, TextureType::Normal }), "ressources/shaders/gbuffer/ambient");
	
	const std::vector<TextureType> includedTextures = { TextureType::Albedo, TextureType::Depth, TextureType::Normal, TextureType::Effects };
	for(auto& dirLight : _directionalLights){
		dirLight.init(_gbuffer->textureIds(includedTextures));
	}
	
	for(auto& pointLight : _pointLights){
		pointLight.init(_gbuffer->textureIds(includedTextures));
	}
	
	_fxaaScreen.init(_sceneFramebuffer->textureId(), "ressources/shaders/screens/fxaa");
	_finalScreen.init(_fxaaFramebuffer->textureId(), "ressources/shaders/screens/final_screenquad");
	checkGLError();
	
}


void Renderer::draw(){
	
	// Compute the time elapsed since last frame
	float elapsed = glfwGetTime() - _timer;
	_timer = glfwGetTime();
	
	// Physics simulation
	physics(elapsed);
	
	
	glm::vec2 invRenderSize = 1.0f / _camera._renderSize;
	
	// --- Light pass -------
	
	// Draw the scene inside the framebuffer.
	for(auto& dirLight : _directionalLights){
		
		dirLight.bind();
		
		// Draw objects.
		_suzanne.drawDepth(dirLight._mvp);
		_dragon.drawDepth(dirLight._mvp);
		//_plane.drawDepth(planeModel, _light._mvp);
		
		dirLight.blurAndUnbind();
	
	}
	
	// ----------------------
	
	// --- Scene pass -------
	// Bind the full scene framebuffer.
	_gbuffer->bind();
	// Set screen viewport
	glViewport(0,0,_gbuffer->_width,_gbuffer->_height);
	
	// Clear the depth buffer (we know we will draw everywhere, no need to clear color.
	glClear(GL_DEPTH_BUFFER_BIT);
	
	// Draw objects
	_suzanne.draw(_camera._view, _camera._projection);
	_dragon.draw(_camera._view, _camera._projection);
	_plane.draw(_camera._view, _camera._projection);
	_skybox.draw(_camera._view, _camera._projection);
	
	// Unbind the full scene framebuffer.
	_gbuffer->unbind();
	// ----------------------
	
	glDisable(GL_DEPTH_TEST);
	// --- Gbuffer composition pass
	_sceneFramebuffer->bind();
	
	
	glViewport(0,0,_sceneFramebuffer->_width, _sceneFramebuffer->_height);
	
	_ambientScreen.draw( invRenderSize, _camera._view, _camera._projection);
	
	glEnable(GL_BLEND);
	for(auto& dirLight : _directionalLights){
		dirLight.draw( invRenderSize, _camera._view, _camera._projection);
	}
	
	for(auto& pointLight : _pointLights){
		pointLight.draw( invRenderSize, _camera._view, _camera._projection);
	}
	glDisable(GL_BLEND);
	
	_sceneFramebuffer->unbind();
	
	// --- FXAA pass -------
	// Bind the post-processing framebuffer.
	_fxaaFramebuffer->bind();
	// Set screen viewport.
	glViewport(0,0,_fxaaFramebuffer->_width, _fxaaFramebuffer->_height);
	
	// Draw the fullscreen quad
	_fxaaScreen.draw( invRenderSize );
	
	_fxaaFramebuffer->unbind();
	// ----------------------
	
	
	// --- Final pass -------
	// We now render a full screen quad in the default framebuffer, using sRGB space.
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	// Set screen viewport.
	glViewport(0,0,_camera._screenSize[0],_camera._screenSize[1]);
	
	// Draw the fullscreen quad
	_finalScreen.draw( 1.0f / _camera._screenSize);
	
	glDisable(GL_FRAMEBUFFER_SRGB);
	// ----------------------
	glEnable(GL_DEPTH_TEST);
	
	// Update timer
	_timer = glfwGetTime();
}

void Renderer::physics(float elapsedTime){
	
	_camera.update(elapsedTime);
	
	// Update lights.
	_directionalLights[0].update(glm::vec3(2.0f, 1.5f + sin(0.5*_timer),2.0f), _camera._view);
	
	for(size_t i = 0; i <_pointLights.size(); ++i){
		auto& pointLight = _pointLights[i];
		glm::vec4 newPosition = glm::rotate(glm::mat4(1.0f), (i%2==0 ? 1.0f: -1.0f) * elapsedTime, glm::vec3(0.0f, 1.0f, 0.0f))*glm::vec4(pointLight._local, 1.0f);
		pointLight.update(glm::vec3(newPosition), _camera._view);
	}
	
	// Update objects.
	const glm::mat4 dragonModel = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-0.1,-0.05,-0.25)),glm::vec3(0.5f));
	const glm::mat4 suzanneModel = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.2,0.0,0.0)),float(_timer),glm::vec3(0.0f,1.0f,0.0f)),glm::vec3(0.25f));
	const glm::mat4 planeModel = glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.0f,-0.35f,-0.5f)), glm::vec3(2.0f));
	
	_dragon.update(dragonModel);
	_suzanne.update(suzanneModel);
	_plane.update(planeModel);
	
}


void Renderer::clean(){
	// Clean objects.
	_suzanne.clean();
	_dragon.clean();
	_plane.clean();
	_skybox.clean();
	for(auto& dirLight : _directionalLights){
		dirLight.clean();
	}
	_ambientScreen.clean();
	_fxaaScreen.clean();
	_finalScreen.clean();
	_gbuffer->clean();
	_sceneFramebuffer->clean();
	_fxaaFramebuffer->clean();
}


void Renderer::resize(int width, int height){
	//Update the size of the viewport.
	glViewport(0, 0, width, height);
	// Update the projection matrix.
	_camera.screen(width, height);
	// Resize the framebuffer.
	_gbuffer->resize(_camera._renderSize);
	_sceneFramebuffer->resize(_camera._renderSize);
	_fxaaFramebuffer->resize(_camera._renderSize);
}

void Renderer::keyPressed(int key, int action){
	if(action == GLFW_PRESS){
		_camera.key(key, true);
	} else if(action == GLFW_RELEASE) {
		_camera.key(key, false);
	}
}

void Renderer::buttonPressed(int button, int action, double x, double y){
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			_camera.mouse(MouseMode::Start,x, y);
		} else if (action == GLFW_RELEASE) {
			_camera.mouse(MouseMode::End, 0.0, 0.0);
		}
	} else {
		std::cout << "Button: " << button << ", action: " << action << std::endl;
	}
}

void Renderer::mousePosition(int x, int y, bool leftPress, bool rightPress){
	if (leftPress){
		_camera.mouse(MouseMode::Move, float(x), float(y));
	}
}



