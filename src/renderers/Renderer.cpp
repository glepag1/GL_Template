#include "Renderer.hpp"
#include "../Object.hpp"
#include "../input/Input.hpp"

#include <stdio.h>
#include <vector>


Renderer::~Renderer(){}

Renderer::Renderer(Config & config) : _config(config) {
	
	// Initial render resolution.
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
	
	defaultGLSetup();
	
	_scene = nullptr;
	
}

Renderer::Renderer(Config & config, std::shared_ptr<Scene> & scene) : _config(config) {
	
	// Initial render resolution.
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
	
	defaultGLSetup();
	
	_scene = scene;
	if(_scene){
		_scene->init();
	}
	
}



void Renderer::update(){
	if(Input::manager().resized()){
		resize((int)Input::manager().size()[0], (int)Input::manager().size()[1]);
	}
}


void Renderer::clean() const {
	// Clean objects.
	if(_scene){
		_scene->clean();
	}
}


void Renderer::updateResolution(int width, int height){
	_config.screenResolution[0] = float(width > 0 ? width : 1);
	_config.screenResolution[1] = float(height > 0 ? height : 1);
	// Same aspect ratio as the display resolution
	_renderResolution = (_config.internalVerticalResolution/_config.screenResolution[1]) * _config.screenResolution;
}

void Renderer::defaultGLSetup(){
	// Default GL setup.
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}



