#pragma once

#include <chrono>

#include <glm/matrix.hpp>

#include <il.h>

#include <globjects\Program.h>
#include <globjects\VertexArray.h>
#include <globjects\Buffer.h>
#include <globjects\Framebuffer.h>
#include <globjects\Texture.h>

#include "MeshLoader.h"

namespace Camera
{
	struct Camera {
		glm::vec3 eye;
		glm::vec3 center;
		glm::vec3 up;

		Camera(glm::vec3 t_eye, glm::vec3 t_center, glm::vec3 t_up)
		{
			eye = t_eye;
			center = t_center;
			up = t_up;
		}
	};
}

namespace
{
	const int c_aBufferMaxLayers = 16;
	const double PI = std::atan(1) * 4;
	const bool c_twoLines = true;
	const bool c_printFPS = true;
	const glm::vec4 c_planeColor(0.25f, 0.25f, 0.25f, 1.f);
	const glm::vec4 c_streetsColor(0.1f, 0.1f, 0.1f, 1.f);
	const glm::vec4 c_lineColor(1.0f, 0.6f, 0.0f, 1.0f);
	const glm::vec4 c_line2Color(0.0f, 0.6f, 1.0f, 1.0f);
	const glm::vec3 c_clearColor(1.0f, 1.0f, 1.0f);		//for pictures which are supposed to be printed on white paper
	//const glm::vec3 c_clearColor(0.5f, 0.7f, 0.9f);	//a sky blue
}

class Painter
{
public:
	enum class CameraMode {
		Rotation,
		Pos1,
		Pos2
	};

public:
    Painter(int initialWindowWidth, int initialWindowHeight);

	void resizeWindow(int width, int height);
	void bindStaticTextures();

    void initialize();
	void draw(short renderMode);

	void setCameraMode(CameraMode mode);

protected:
	double getTimeDifference();
	void initializeShader(globjects::Program & program, std::string & pathVertexShader, std::string & pathFragmentShader);
	void initializeShader_geom(globjects::Program & program, std::string & pathVertexShader, std::string & pathGeometryShader, std::string & pathFragmentShader);
	void setUpShader();
	void loadGeometry();
	void loadGeometryToGPU(globjects::ref_ptr<globjects::VertexArray> & vao,
									globjects::ref_ptr<globjects::Buffer> & vboIndices,
									std::vector<glm::vec3> & vertices,
									std::vector<unsigned int> & indices, bool useNormals);

	void bindStaticTextures(globjects::ref_ptr<globjects::Program> program);
	void update(globjects::ref_ptr<globjects::Program> program, bool useNormals, bool renderDepthValueForTextureUsage, bool newFrame = false, bool inputChanged = false, bool to_fromABuffer = false, int kernelSize = 16);

	//helper draw methods
	void mixWithEnhancedEdges(globjects::Texture & source, bool inputChanged);
	void drawStandardCity(bool inputChanged);

	//techniques
	void drawNormalScene(bool inputChanged);
	void drawOutlineHintsVisualization(bool inputChanged, bool forCompositing = false, globjects::Texture * resultTexture = nullptr);
	void drawStaticTransparancyVisualization(bool inputChanged, bool forCompositing = false, globjects::Texture * resultTexture = nullptr);
	void drawAdaptiveTransparancyPerPixelVisualization(bool inputChanged, bool forCompositing = false, globjects::Texture * resultTexture = nullptr);
	void drawGhostedViewVisualization(bool inputChanged, bool forCompositing = false, globjects::Texture * resultTexture = nullptr);
	void drawFenceHintsVisualization(bool inputChanged, bool forCompositing = false, globjects::Texture * resultTexture = nullptr);
	void drawFullFlatVisualization(bool inputChanged);
	void drawFullFootprintVisualization(bool inputChanged);

	//mixtures of techniques
	void mix_onDepth(bool inputChanged);
	void mix_onLayer(bool inputChanged);
	
	void drawToABufferOnly(globjects::VertexArray * vao, std::vector<unsigned int> & indices, globjects::Program * program, glm::vec4 specifiedColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.f), unsigned int typeId = 0u, gl::GLenum drawMode = gl::GL_TRIANGLES);
	void drawGeneralGeometry(globjects::VertexArray * vao, std::vector<unsigned int> & indices, globjects::Program * program, glm::vec4 specifiedColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.f));
	void drawSAQ(globjects::Program * program, std::vector<globjects::ref_ptr<globjects::Texture>> * textures);
	void drawFenceGradient(globjects::VertexArray * vao, std::vector<unsigned int> & indices, glm::vec4 specifiedColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.f));
	void drawWithLineRepresentation(globjects::VertexArray * vao, std::vector<unsigned int> & indices, globjects::Program * program, std::vector<globjects::ref_ptr<globjects::Texture>> * textures, glm::vec4 specifiedColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.f), gl::GLenum drawMode = gl::GL_LINE_STRIP);

	void setGlState();
	void unsetGlState();

	void setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const bool value) const;
	void setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const unsigned int value) const;
	void setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const int value) const;
	void setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const glm::vec3 & value) const;
	void setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const glm::vec4 & value) const;
	void setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const glm::mat4x4 & value) const;

	void setUpMatrices();
	void rotateModelByTime(double timeDifference);
	void setUpFBOs();
	void setFBO(globjects::ref_ptr<globjects::Framebuffer> & fbo, std::vector<globjects::ref_ptr<globjects::Texture>> * textures, int numberOfTextures);
	void setUpABuffer();
	void setUpCubeMap();
	void loadImageToGPU(std::string & filename, gl::GLenum target, ILuint handle);

protected:
	MeshLoader m_meshLoader;

	//########################################################## Programs ##########################################################
	//#### visualization techniques
	globjects::ref_ptr<globjects::Program> m_generalProgram;
	globjects::ref_ptr<globjects::Program> m_outlineHintsProgram;
	globjects::ref_ptr<globjects::Program> m_transparentCityProgram;
	globjects::ref_ptr<globjects::Program> m_adaptiveTransparancyPerPixelProgram;
	globjects::ref_ptr<globjects::Program> m_ghostedViewProgram;
	globjects::ref_ptr<globjects::Program> m_fenceHintsProgram;
	globjects::ref_ptr<globjects::Program> m_footprintProgram;

	//#### helper programs for single visualizations
	globjects::ref_ptr<globjects::Program> m_haloLineABufferedProgram;
	globjects::ref_ptr<globjects::Program> m_toABufferOnlyProgram;
	globjects::ref_ptr<globjects::Program> m_extrudedLinetoABufferOnlyProgram;
	globjects::ref_ptr<globjects::Program> m_clearABufferProgram;
	globjects::ref_ptr<globjects::Program> m_sortABufferProgram;
	globjects::ref_ptr<globjects::Program> m_toABufferTypedProgram;
	globjects::ref_ptr<globjects::Program> m_maskingBoxFilterForAdaptiveTransparancyProgram;
	globjects::ref_ptr<globjects::Program> m_maskingBoxFilterForGhostedViewProgram;
	globjects::ref_ptr<globjects::Program> m_fenceHintsCubeProgram;
	globjects::ref_ptr<globjects::Program> m_fenceHintsLineProgram;
	globjects::ref_ptr<globjects::Program> m_fenceGradientProgram;

	//#### composition visualization programs
	globjects::ref_ptr<globjects::Program> m_mixByMaskProgram;
	globjects::ref_ptr<globjects::Program> m_depthMaskProgram;
	globjects::ref_ptr<globjects::Program> m_layerMaskProgram;

	//#### helper programs for combining visualizations
	globjects::ref_ptr<globjects::Program> m_perspectiveDepthMaskProgram;

	//#### programs for additional effects
	globjects::ref_ptr<globjects::Program> m_edgeDetectionProgram;
	globjects::ref_ptr<globjects::Program> m_dilationFilterProgram;
	globjects::ref_ptr<globjects::Program> m_erosionFilterProgram;
	globjects::ref_ptr<globjects::Program> m_mixEnhancedEdgesProgram;

	//########################################################## FBO ##########################################################
	//#### visualization fbos
	globjects::ref_ptr<globjects::Framebuffer> m_fboNormalVisualization;
	globjects::ref_ptr<globjects::Framebuffer> m_fboOutlineHints;
	globjects::ref_ptr<globjects::Framebuffer> m_fboStaticTransparancy;
	globjects::ref_ptr<globjects::Framebuffer> m_fboAdaptiveTranspancyPerPixel;
	globjects::ref_ptr<globjects::Framebuffer> m_fboGhostedView;
	globjects::ref_ptr<globjects::Framebuffer> m_fboFenceHints;

	//#### composition visualization fbos
	globjects::ref_ptr<globjects::Framebuffer> m_fbo_mix_onDepth;
	globjects::ref_ptr<globjects::Framebuffer> m_fbo_mix_onLayer;

	//#### additional fbos
	globjects::ref_ptr<globjects::Framebuffer> m_fboStandardCity;
	globjects::ref_ptr<globjects::Framebuffer> m_fboEdgeEnhancement;

	//########################################################## Textures ##########################################################
	//#### visualization textures
	std::vector<globjects::ref_ptr<globjects::Texture>> m_normalVisualizationTextures;
	std::vector<globjects::ref_ptr<globjects::Texture>> m_outlineHintsTextures;
	std::vector<globjects::ref_ptr<globjects::Texture>> m_staticTransparancyTextures;
	std::vector<globjects::ref_ptr<globjects::Texture>> m_adaptiveTransparancyPerPixelTextures;
	std::vector<globjects::ref_ptr<globjects::Texture>> m_ghostedViewTextures;
	std::vector<globjects::ref_ptr<globjects::Texture>> m_fenceHintsTextures;

	//#### composition visualization textures
	std::vector<globjects::ref_ptr<globjects::Texture>> m_mix_onDepthTextures;
	std::vector<globjects::ref_ptr<globjects::Texture>> m_mix_onLayerTextures;

	//#### additional textures
	std::vector<globjects::ref_ptr<globjects::Texture>> m_standardCityTexture;
	std::vector<globjects::ref_ptr<globjects::Texture>> m_enhancedEdgeTexture;
	gl::GLuint m_aBufferTextureArrayID;
	gl::GLuint m_aBufferAlphaArrayID;
	gl::GLuint m_aBufferIndexTexture;
	gl::GLuint m_transparentTypedTexture;
	gl::GLuint m_cubeMap;

	//########################################################## Data ##########################################################
	globjects::ref_ptr<globjects::VertexArray> m_vaoCity;
	globjects::ref_ptr<globjects::VertexArray> m_vaoLine_realGeometry;
	globjects::ref_ptr<globjects::VertexArray> m_vaoLine2_realGeometry;
	globjects::ref_ptr<globjects::VertexArray> m_vaoPlanePath;
	globjects::ref_ptr<globjects::VertexArray> m_vaoPlanePath2;
	globjects::ref_ptr<globjects::VertexArray> m_vaoSAQ;
	globjects::ref_ptr<globjects::VertexArray> m_vaoPlane;
	globjects::ref_ptr<globjects::VertexArray> m_vaoStreets;

	globjects::ref_ptr<globjects::Buffer> m_vboCityIndices;
	globjects::ref_ptr<globjects::Buffer> m_vboLineIndices;
	globjects::ref_ptr<globjects::Buffer> m_vboLine2Indices;
	globjects::ref_ptr<globjects::Buffer> m_vboPlanePathIndices;
	globjects::ref_ptr<globjects::Buffer> m_vboPlanePath2Indices;
	globjects::ref_ptr<globjects::Buffer> m_vboSAQIndices;
	globjects::ref_ptr<globjects::Buffer> m_vboPlaneIndices;
	globjects::ref_ptr<globjects::Buffer> m_vboStreetsIndices;

	std::vector<glm::vec3> m_cityVertices;
	std::vector<glm::vec3> m_lineVertices_OLD;
	std::vector<glm::vec3> m_line2Vertices_OLD;
	std::vector<glm::vec3> m_planePathVertices;
	std::vector<glm::vec3> m_planePath2Vertices;
	std::vector<glm::vec3> m_planeVertices;
	std::vector<glm::vec3> m_streetsVertices;
	std::vector<glm::vec3> m_screenAlignedQuad;

	//line representation vertices of first path
	globjects::ref_ptr<globjects::VertexArray> m_vaoLineVertices;
	std::vector<glm::vec3> m_lineVertices;
	globjects::ref_ptr<globjects::Buffer> m_vboLineVertices;
	std::vector<unsigned int> m_lineIndices;

	//line representation vertices of second path
	globjects::ref_ptr<globjects::VertexArray> m_vaoLineVertices2;
	std::vector<glm::vec3> m_lineVertices2;
	globjects::ref_ptr<globjects::Buffer> m_vboLineVertices2;
	std::vector<unsigned int> m_lineIndices2;

	std::vector<unsigned int> m_cityIndices;
	std::vector<unsigned int> m_lineIndices_OLD;
	std::vector<unsigned int> m_line2Indices_OLD;
	std::vector<unsigned int> m_linesIndices;
	std::vector<unsigned int> m_planePathIndices;
	std::vector<unsigned int> m_planePath2Indices;
	std::vector<unsigned int> m_SAQIndices;
	std::vector<unsigned int> m_planeIndices;
	std::vector<unsigned int> m_streetsIndices;
	std::vector<glm::vec3> m_cityNormals;

	//########################################################## Miscellaneous ##########################################################
	glm::mat4x4 m_projection;
	glm::mat4x4 m_view;
	glm::mat4x4 m_model;
	glm::mat4x4 m_transform;

	Camera::Camera m_camera;

	//lights are not used at the moment, lighting is achieved by a texture cube map (fixed lighting)
	glm::vec3 m_sceneLight1;
	glm::vec3 m_sceneLight2;

	glm::vec4 m_currentHaloColor;

	std::chrono::steady_clock::time_point m_applicationStartTime;
	std::chrono::steady_clock::time_point m_lastFPS;
	double m_lastTimeStamp;
	double m_avgFPS;
	int m_renderCallsCount;

	int m_windowWidth;
	int m_windowHeight;

	unsigned short m_renderMode;
	bool m_inputChanged;
	bool m_timeValid;

	CameraMode m_cameraMode;
};
