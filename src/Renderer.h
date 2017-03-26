#ifndef Renderer_h
#define Renderer_h
#include <gl3w/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "helpers/GenerationUtilities.h"

#include "Framebuffer.h"
#include "Gbuffer.h"
#include "camera/Camera.h"
#include "Object.h"
#include "Skybox.h"
#include "ScreenQuad.h"
#include "lights/DirectionalLight.h"
#include "lights/PointLight.h"

class Renderer {

public:


	~Renderer();

	/// Init function
	Renderer(int width, int height);

	/// Draw function
	void draw();

	void physics(float elapsedTime);

	/// Clean function
	void clean();

	/// Handle screen resizing
	void resize(int width, int height);

	/// Handle keyboard inputs
	void keyPressed(int key, int action);

	/// Handle mouse inputs
	void buttonPressed(int button, int action, double x, double y);

	void mousePosition(int x, int y, bool leftPress, bool rightPress);


private:

	float _timer;

	Camera _camera;

	Object _suzanne;
	Object _dragon;
	Skybox _skybox;
	Object _plane;

	std::shared_ptr<Framebuffer> _lightFramebuffer;
	std::shared_ptr<Framebuffer> _blurFramebuffer;
	std::shared_ptr<Gbuffer> _gbuffer;
	std::shared_ptr<Framebuffer> _sceneFramebuffer;
	std::shared_ptr<Framebuffer> _fxaaFramebuffer;

	ScreenQuad _blurScreen;
	ScreenQuad _fxaaScreen;
	ScreenQuad _finalScreen;

	std::vector<DirectionalLight> _directionalLights;
	std::vector<PointLight> _pointLights;

};

#endif
