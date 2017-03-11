#include "Painter.h"

#include <ratio>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <glbinding/Binding.h>
#include <glbinding/ContextInfo.h>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/functions-patches.h>

#include <globjects/Shader.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Renderbuffer.h>

using namespace gl;

Painter::Painter(int initialWindowWidth, int initialWindowHeight)
    : m_camera(glm::vec3(30.0f, 24.0f, 20.0f), glm::vec3(0.f, -5.f, 0.f), glm::vec3(0.f, 1.f, 0.f))
	, m_sceneLight1(50.f, 10.f, 20.f)
	, m_sceneLight2(-40.f, 80.f, -30.f)
	, m_currentHaloColor(0.f, 0.f, 0.f, 1.f)
	, m_lastTimeStamp(0.0)
	, m_avgFPS(0.f)
	, m_renderCallsCount(0)
	, m_windowWidth(initialWindowWidth)
	, m_windowHeight(initialWindowHeight)
	, m_renderMode(0)
	, m_inputChanged(true)
	, m_timeValid(true)
{
}

void Painter::resizeWindow(int width, int height)
{
	m_windowWidth = width;
	m_windowHeight = height;
	glViewport(0, 0, width, height);
	setUpMatrices();
	setUpFBOs();
	setUpABuffer();

}

void Painter::initialize()
{
	loadGeometry();

	m_generalProgram = new globjects::Program();
	m_outlineHintsProgram = new globjects::Program();
	m_extrudedLinetoABufferOnlyProgram = new globjects::Program();
	m_fenceGradientProgram = new globjects::Program();
	m_haloLineABufferedProgram = new globjects::Program();
	m_clearABufferProgram = new globjects::Program();
	m_sortABufferProgram = new globjects::Program();
	m_toABufferOnlyProgram = new globjects::Program();
	m_toABufferTypedProgram = new globjects::Program();
	m_transparentCityProgram = new globjects::Program();
	m_adaptiveTransparancyPerPixelProgram = new globjects::Program();
	m_maskingBoxFilterForAdaptiveTransparancyProgram = new globjects::Program();
	m_maskingBoxFilterForGhostedViewProgram = new globjects::Program();
	m_ghostedViewProgram = new globjects::Program();
	m_fenceHintsProgram = new globjects::Program();
	m_fenceHintsCubeProgram = new globjects::Program();
	m_fenceHintsLineProgram = new globjects::Program();
	m_footprintProgram = new globjects::Program();
	m_perspectiveDepthMaskProgram = new globjects::Program();
	m_edgeDetectionProgram = new globjects::Program();
	m_dilationFilterProgram = new globjects::Program();
	m_mixEnhancedEdgesProgram = new globjects::Program();
	m_mixByMaskProgram = new globjects::Program();
	m_vaoCity = new globjects::VertexArray();
	m_vaoLine_realGeometry = new globjects::VertexArray();
	m_vaoLine2_realGeometry = new globjects::VertexArray();
	m_vaoPlanePath = new globjects::VertexArray();
	m_vaoPlanePath2 = new globjects::VertexArray();
	m_vaoSAQ = new globjects::VertexArray();
	m_vaoPlane = new globjects::VertexArray();
	m_vaoStreets = new globjects::VertexArray();
	m_vboCityIndices = new globjects::Buffer();
	m_vboLineIndices = new globjects::Buffer();
	m_vboLine2Indices = new globjects::Buffer();
	m_vboPlanePathIndices = new globjects::Buffer();
	m_vboPlanePath2Indices = new globjects::Buffer();
	m_vboSAQIndices = new globjects::Buffer();
	m_vboPlaneIndices = new globjects::Buffer();
	m_vboStreetsIndices = new globjects::Buffer();
	m_fboStandardCity = new globjects::Framebuffer();
	m_fboNormalVisualization = new globjects::Framebuffer();
	m_fboOutlineHints = new globjects::Framebuffer();
	m_fboStaticTransparancy = new globjects::Framebuffer();
	m_fboAdaptiveTranspancyPerPixel = new globjects::Framebuffer();
	m_fboGhostedView = new globjects::Framebuffer();
	m_fboFenceHints = new globjects::Framebuffer();
	m_fboPerspectiveDepthMask = new globjects::Framebuffer();
	m_fboEdgeEnhancement = new globjects::Framebuffer();

	//line representation
	m_vaoLineVertices = new globjects::VertexArray();
	m_vboLineVertices = new globjects::Buffer();
	m_vaoLineVertices2 = new globjects::VertexArray();
	m_vboLineVertices2 = new globjects::Buffer();

	//define screen aligned quad
	m_screenAlignedQuad.push_back(glm::vec3(-1.0f, -1.0f, 0.0f));
	m_screenAlignedQuad.push_back(glm::vec3(1.0f, -1.0f, 0.0f));
	m_screenAlignedQuad.push_back(glm::vec3(-1.0f, 1.0f, 0.0f));
	m_screenAlignedQuad.push_back(glm::vec3(1.0f, 1.0f, 0.0f));

	m_SAQIndices = { 0u, 1u, 2u, 3u };

	loadGeometryToGPU(m_vaoSAQ, m_vboSAQIndices, m_screenAlignedQuad, m_SAQIndices, false);

	loadGeometryToGPU(m_vaoCity, m_vboCityIndices, m_cityVertices, m_cityIndices, true);
	loadGeometryToGPU(m_vaoPlane, m_vboPlaneIndices, m_planeVertices, m_planeIndices, false);
	loadGeometryToGPU(m_vaoStreets, m_vboStreetsIndices, m_streetsVertices, m_streetsIndices, false);
	loadGeometryToGPU(m_vaoLine_realGeometry, m_vboLineIndices, m_lineVertices_OLD, m_lineIndices_OLD, false);
	loadGeometryToGPU(m_vaoPlanePath, m_vboPlanePathIndices, m_planePathVertices, m_planePathIndices, false);
	loadGeometryToGPU(m_vaoLineVertices, m_vboLineVertices, m_lineVertices, m_lineIndices, false);

	if (c_twoLines)
	{
		loadGeometryToGPU(m_vaoLine2_realGeometry, m_vboLine2Indices, m_line2Vertices_OLD, m_line2Indices_OLD, false);
		loadGeometryToGPU(m_vaoPlanePath2, m_vboPlanePath2Indices, m_planePath2Vertices, m_planePath2Indices, false);

		loadGeometryToGPU(m_vaoLineVertices2, m_vboLineVertices2, m_lineVertices2, m_lineIndices2, false);
	}

	//set up ressources
	setUpShader();
	setUpMatrices();
	setUpFBOs();
	m_aBufferTextureArrayID = 0;
	setUpABuffer();
	setUpCubeMap();
	bindStaticTextures();

	//set up timer
	std::chrono::steady_clock currentTime;
	m_applicationStartTime = currentTime.now();
}

double Painter::getTimeDifference()
{
	std::chrono::steady_clock currentTime;
	double timeSpan = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(currentTime.now() - m_applicationStartTime).count();
	return timeSpan;
}

void Painter::setUpShader()
{
	m_generalProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/basic.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/basic.frag")
	);
	m_generalProgram->link();

	m_outlineHintsProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/outlineHints.frag")
	);
	m_outlineHintsProgram->link();

	m_extrudedLinetoABufferOnlyProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/basicWithoutTransform.vert"),
		globjects::Shader::fromFile(gl::GL_GEOMETRY_SHADER, "data/extrudeLines.geom"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/drawToABufferOnly.frag")
	);
	m_extrudedLinetoABufferOnlyProgram->link();

	m_haloLineABufferedProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/haloLineABuffered.frag")
	);
	m_haloLineABufferedProgram->link();

	m_clearABufferProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/clearABuffer.frag")
	);
	m_clearABufferProgram->link();

	//Not used?
	m_sortABufferProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/sortABuffer.frag")
	);
	m_sortABufferProgram->link();

	m_toABufferOnlyProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/basic.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/drawToABufferOnly.frag")
	);
	m_toABufferOnlyProgram->link();

	m_toABufferTypedProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/basic.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/drawToABufferTyped.frag")
	);
	m_toABufferTypedProgram->link();

	m_transparentCityProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/staticTransparancy.frag")
	);
	m_transparentCityProgram->link();

	m_adaptiveTransparancyPerPixelProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/adaptiveTransparancyPerPixel.frag")
	);
	m_adaptiveTransparancyPerPixelProgram->link();

	m_maskingBoxFilterForAdaptiveTransparancyProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/maskingBoxFilterAdaptiveTransparancy.frag")
	);
	m_maskingBoxFilterForAdaptiveTransparancyProgram->link();

	m_maskingBoxFilterForGhostedViewProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/maskingBoxFilterGhostedView.frag")
	);
	m_maskingBoxFilterForGhostedViewProgram->link();

	m_ghostedViewProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/ghostedView.frag")
	);
	m_ghostedViewProgram->link();

	m_fenceHintsProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/fenceHints.frag")
	);
	m_fenceHintsProgram->link();

	m_fenceHintsCubeProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/basicWithoutTransform.vert"),
		globjects::Shader::fromFile(gl::GL_GEOMETRY_SHADER, "data/fenceHintsCube.geom"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/basic.frag")
	);
	m_fenceHintsCubeProgram->link();

	m_fenceHintsLineProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/basicWithoutTransform.vert"),
		globjects::Shader::fromFile(gl::GL_GEOMETRY_SHADER, "data/fenceHintsLine.geom"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/basic.frag")
	);
	m_fenceHintsLineProgram->link();

	//TODO: fenceGradient.geom und extrudeLines.geom unterscheiden sich nur in der Höhe
	m_fenceGradientProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/basicWithoutTransform.vert"),
		globjects::Shader::fromFile(gl::GL_GEOMETRY_SHADER, "data/fenceGradient.geom"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/fenceGradient.frag")
	);
	m_fenceGradientProgram->link();

	m_footprintProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/footprint.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/basic.frag")
	);
	m_ghostedViewProgram->link();

	m_mixByMaskProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/mixByMask.frag")
	);
	m_mixByMaskProgram->link();

	m_perspectiveDepthMaskProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/depthMask.frag")
	);
	m_perspectiveDepthMaskProgram->link();

	m_edgeDetectionProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/edgeDetection.frag")
	);
	m_edgeDetectionProgram->link();

	m_dilationFilterProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/dilation.frag")
	);
	m_dilationFilterProgram->link();

	m_mixEnhancedEdgesProgram->attach(
		globjects::Shader::fromFile(gl::GL_VERTEX_SHADER, "data/screenAlignedQuad.vert"),
		globjects::Shader::fromFile(gl::GL_FRAGMENT_SHADER, "data/mixEnhancedEdge.frag")
	);
	m_mixEnhancedEdgesProgram->link();
}

void Painter::loadGeometry()
{
	m_meshLoader.loadFileData();
	bool verticesValid = m_meshLoader.getVertices(m_cityVertices, m_lineVertices_OLD, m_line2Vertices_OLD, m_planePathVertices, m_planePath2Vertices, m_planeVertices, m_streetsVertices);
	bool indicesValid = m_meshLoader.getIndices(m_cityIndices, m_lineIndices_OLD, m_line2Indices_OLD, m_planePathIndices, m_planePath2Indices, m_planeIndices, m_streetsIndices);
	bool normalsValid = m_meshLoader.getNormals(m_normals);

	m_meshLoader.getLineVertices(m_lineVertices, m_lineIndices, true);
	m_meshLoader.getLineVertices(m_lineVertices2, m_lineIndices2, false);

	if (verticesValid == false)
		std::printf("No geometry accessable");

	if (indicesValid == false)
		std::printf("No indices accessable");

	if (normalsValid == false)
		std::printf("No indices accessable");
}

void Painter::loadGeometryToGPU(globjects::ref_ptr<globjects::VertexArray> & vao,
								globjects::ref_ptr<globjects::Buffer> & vboIndices,
								std::vector<glm::vec3> & vertices,
								std::vector<unsigned int> & indices, bool useNormals)
{
	vao->bind();

	globjects::ref_ptr<globjects::Buffer> vbo_vertices = new globjects::Buffer();
	vbo_vertices->bind(GL_ARRAY_BUFFER);
	vbo_vertices->setData(vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	vboIndices->setData(indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	vboIndices->bind(GL_ELEMENT_ARRAY_BUFFER);

	vao->binding(0)->setAttribute(0);
	vao->binding(0)->setBuffer(vbo_vertices, 0, sizeof(float) * 3);
	vao->binding(0)->setFormat(3, GL_FLOAT, GL_FALSE, 0);
	vao->enable(0);

	globjects::ref_ptr<globjects::Buffer> vbo_normals = new globjects::Buffer();

	if (useNormals)
	{
		vbo_normals->bind(GL_ARRAY_BUFFER);
		vbo_normals->setData(m_normals, GL_STATIC_DRAW);
		vao->binding(1)->setAttribute(1);
		vao->binding(1)->setBuffer(vbo_normals, 0, sizeof(glm::vec3));
		vao->binding(1)->setFormat(3, GL_FLOAT, GL_FALSE, 0);
		vao->enable(1);
	}

	vao->unbind();
}

void Painter::bindStaticTextures(globjects::Program & program)
{
	GLint loc_cube = program.getUniformLocation(std::string("cubeMap"));
	if (loc_cube >= 0)
	{
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMap);
		program.setUniform(loc_cube, 10);
	}

	GLint loc_city = program.getUniformLocation(std::string("cityTexture"));
	if (loc_city >= 0)
	{
		m_standardCityTexture.at(0)->bindActive(GL_TEXTURE11);
		program.setUniform(loc_city, 11);
	}
}

void Painter::update(globjects::ref_ptr<globjects::Program> program, bool useNormals, bool renderDepthValueForTextureUsage, bool newFrame, bool inputChanged, bool to_fromABuffer)
{
	if (inputChanged)
	{
		//TODO - how much of these are still needed?
		setUniformOn(program, "lightVector", m_sceneLight1);
		setUniformOn(program, "lightVector2", m_sceneLight2);
		setUniformOn(program, "clearColor", c_clearColor);
	}

	if (newFrame)
	{
		setUniformOn(program, "transform", m_transform);
		setUniformOn(program, "viewVector", glm::vec3(m_camera.center - m_camera.eye));
		setUniformOn(program, "windowWidth", m_windowWidth);
		setUniformOn(program, "windowHeight", m_windowHeight);
	}

	if (to_fromABuffer)
	{
		GLint loc_aBuffer = program->getUniformLocation("aBufferImg");
		GLint loc_aBufferIndexTexture = program->getUniformLocation("aBufferIndexImg");
		GLint loc_typeIdImg = program->getUniformLocation("typeIdImg");

		if (loc_aBuffer >= 0)
		{
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D_ARRAY, m_aBufferTextureArrayID);
			glProgramUniform1i(program->id(), glGetUniformLocation(program->id(), "aBufferImg"), 0);
		}
		if (loc_aBufferIndexTexture >= 0)
		{
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, m_aBufferIndexTexture);
			glProgramUniform1i(program->id(), glGetUniformLocation(program->id(), "aBufferIndexImg"), 1);
		}
		if (loc_typeIdImg >= 0)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_transparentTypedTexture);
			glProgramUniform1i(program->id(), glGetUniformLocation(program->id(), "typeIdImg"), 2);
		}

		setUniformOn(program, "typeId", 0u);
		setUniformOn(program, "maxLayer", c_aBufferMaxLayers);
	}

	setUniformOn(program, "useNormals", useNormals);
	setUniformOn(program, "renderDepthValueForTextureUsage", renderDepthValueForTextureUsage);
}

void Painter::draw(short renderMode)
{
	//calculation time difference for FPS count and scene rotation
	double time = getTimeDifference();
	if (c_printFPS) {
		double renderTime = time - m_lastTimeStamp;
		m_lastTimeStamp = time;
		m_renderCallsCount += 1;
		m_avgFPS += 1.0 / renderTime;
		if (m_renderCallsCount > 10)
		{
			std::printf("FPS:\t %f \n", m_avgFPS / m_renderCallsCount);
			m_renderCallsCount = 0;
			m_avgFPS = 0;
		}
	}
	rotateModelByTime(time);

	if (m_renderMode != renderMode)
	{
		m_renderMode = renderMode;
		m_inputChanged = true;
	}

	m_transform = m_projection * m_view * m_model;

	switch (renderMode)
	{
	case 0:
		drawNormalScene(m_inputChanged);
		break;
	case 1:
		drawOutlineHintsVisualization(m_inputChanged);
		break;
	case 2:
		drawStaticTransparancyVisualization(m_inputChanged);
		break;
	case 3:
		drawAdaptiveTransparancyPerPixelVisualization(m_inputChanged);
		break;
	case 4:
		drawGhostedViewVisualization(m_inputChanged);
		break;
	case 5:
		drawFenceHintsVisualization(m_inputChanged);
		break;
	case 6:
		drawFullFlatVisualization(m_inputChanged);
		break;
	case 7:
		drawFullFootprintVisualization(m_inputChanged);
		break;
	case 10:
		mix_outlineHints_adaptiveTransparancy_onDepth(m_inputChanged);
		break;
	case 11:
		;
		break;
	case 12:
		;
		break;
	default:
		break;
	}

	m_inputChanged = false;
}

void Painter::mixWithEnhancedEdges(globjects::Texture & source, bool inputChanged)
{
	setGlState();

	m_fboEdgeEnhancement->bind(GL_FRAMEBUFFER);
	m_fboEdgeEnhancement->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	update(m_generalProgram, true, true, true, inputChanged);
	drawGeneralGeometry(m_vaoCity, m_cityIndices, m_generalProgram);

	m_fboEdgeEnhancement->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_edgeDetectionProgram, false, true, true);
	drawToSAQ(m_edgeDetectionProgram, &m_enhancedEdgeTexture);

	//TODO - use or delete dilation program
	//update(m_dilationFilterProgram, false, true, true);
	//drawToSAQ(m_dilationFilterProgram, &m_enhancedEdgeTexture);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	update(m_mixEnhancedEdgesProgram, false, false, true, inputChanged);
	GLint loc_texture0 = m_mixEnhancedEdgesProgram->getUniformLocation(std::string("texture0"));
	if (loc_texture0 >= 0)
	{
		m_enhancedEdgeTexture.at(1)->bindActive(GL_TEXTURE0);
		m_mixEnhancedEdgesProgram->setUniform(loc_texture0, 0);
	}

	GLint loc_texture1 = m_mixEnhancedEdgesProgram->getUniformLocation(std::string("texture1"));
	if (loc_texture1 >= 0)
	{
		source.bindActive(GL_TEXTURE1);
		m_mixEnhancedEdgesProgram->setUniform(loc_texture1, 1);
	}

	drawToSAQ(m_mixEnhancedEdgesProgram, nullptr);

	unsetGlState();
}

//Use this to render the city with standard settings to the texture 'm_standardCityTexture'
void Painter::drawStandardCity(bool inputChanged)
{
	setGlState();

	m_fboStandardCity->bind(GL_FRAMEBUFFER);
	m_fboStandardCity->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_generalProgram, false, true, true, inputChanged);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	update(m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoCity, m_cityIndices, m_generalProgram);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
}

void Painter::drawNormalScene(bool inputChanged)
{
	setGlState();
	drawStandardCity(inputChanged);
	mixWithEnhancedEdges(*(m_standardCityTexture.at(0)), inputChanged);
	unsetGlState();
}

void Painter::drawOutlineHintsVisualization(bool inputChanged, bool forCompositing, globjects::Texture * resultTexture)
{
	setGlState();

	if (!forCompositing)
	{
		//########## Render standard city to FBO ##############
		drawStandardCity(inputChanged);
	}

	//########## clear A-Buffer and IndexImage #############
	update(m_clearABufferProgram, false, false, true, inputChanged, true);
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	
	//########## Render extruded line to ABuffer ##############
	update(m_extrudedLinetoABufferOnlyProgram, false, true, true, inputChanged, true);
	drawToABufferOnly(m_vaoLineVertices, m_lineIndices, m_extrudedLinetoABufferOnlyProgram, glm::vec4(1.f, 1.f, 1.f, 1.f), 0u, gl::GL_LINE_STRIP);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## Render halo image to FBO ##############
	m_fboOutlineHints->bind(GL_FRAMEBUFFER);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_currentHaloColor = c_lineColor;

	update(m_haloLineABufferedProgram, false, true, true, inputChanged, true);
	drawToSAQ(m_haloLineABufferedProgram, &m_outlineHintsTextures);

	if(c_twoLines)
	{
		//########## clear A-Buffer and IndexImage #############
		drawToSAQ(m_clearABufferProgram, &m_outlineHintsTextures);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//########## Render extruded line2 to ABuffer ##############
		drawToABufferOnly(m_vaoLineVertices2, m_lineIndices2, m_extrudedLinetoABufferOnlyProgram, glm::vec4(1.f, 1.f, 1.f, 1.f), 0u, gl::GL_LINE_STRIP);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//########## Render halo from line2 image to FBO ##############
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_currentHaloColor = c_line2Color;

		drawToSAQ(m_haloLineABufferedProgram, &m_outlineHintsTextures);
	}

	if (forCompositing && resultTexture)
	{
		//########## Render to resultTexture ##############
		m_fboOutlineHints->bind(GL_FRAMEBUFFER);

		if (inputChanged)
		{
			m_fboOutlineHints->attachTexture(GL_COLOR_ATTACHMENT2, 0);
			m_fboOutlineHints->attachTexture(GL_COLOR_ATTACHMENT2, resultTexture);
		}

		m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT2);
		glClearColor(c_clearColor.x, c_clearColor.y, c_clearColor.z, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		update(m_outlineHintsProgram, false, false, true, inputChanged);
		drawToSAQ(m_outlineHintsProgram, &m_outlineHintsTextures);

		globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	}
	else
	{
		//########## Render to the Screen ##############
		m_fboOutlineHints->bind(GL_FRAMEBUFFER);

		if (inputChanged)
		{
			m_fboOutlineHints->attachTexture(GL_COLOR_ATTACHMENT2, 0);
			m_fboOutlineHints->attachTexture(GL_COLOR_ATTACHMENT2, m_outlineHintsTextures.at(2));
		}

		m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT2);
		glClearColor(c_clearColor.x, c_clearColor.y, c_clearColor.z, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		update(m_outlineHintsProgram, false, false, true, inputChanged);
		drawToSAQ(m_outlineHintsProgram, &m_outlineHintsTextures);

		globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

		//########## Add edge enhancement ##############
		mixWithEnhancedEdges(*(m_outlineHintsTextures.at(2)), inputChanged);
	}

	unsetGlState();
}

void Painter::drawStaticTransparancyVisualization(bool inputChanged, bool forCompositing, globjects::Texture * resultTexture)
{
	setGlState();

	//########## clear A-Buffer and IndexImage #############
	update(m_clearABufferProgram, false, false, true, inputChanged, true);
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## render city to A-Buffer ############
	m_fboStaticTransparancy->bind(GL_FRAMEBUFFER);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_toABufferTypedProgram, true, false, true, inputChanged, true);
	drawToABufferOnly(m_vaoCity, m_cityIndices, m_toABufferTypedProgram, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), 0u);

	update(m_toABufferTypedProgram, false, false, false, inputChanged);
	drawToABufferOnly(m_vaoPlane, m_planeIndices, m_toABufferTypedProgram, c_planeColor, 1u);
	drawToABufferOnly(m_vaoStreets, m_streetsIndices, m_toABufferTypedProgram, c_streetsColor, 2u);
	drawToABufferOnly(m_vaoPlanePath, m_planePathIndices, m_toABufferTypedProgram, c_lineColor, 3u);
	drawToABufferOnly(m_vaoPlanePath2, m_planePath2Indices, m_toABufferTypedProgram, c_line2Color, 3u);

	//drawToSAQ(m_sortABufferProgram, nullptr);

	if (forCompositing && resultTexture)
	{
		//########## Render to resultTexture ##############
		if (inputChanged)
		{
			m_fboStaticTransparancy->attachTexture(GL_COLOR_ATTACHMENT0, 0);
			m_fboStaticTransparancy->attachTexture(GL_COLOR_ATTACHMENT0, resultTexture);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		update(m_transparentCityProgram, false, false, true, inputChanged);
		drawToSAQ(m_transparentCityProgram, nullptr);

		globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	}
	else
	{
		//########## Render to the Screen ##############
		if (inputChanged)
		{
			m_fboStaticTransparancy->attachTexture(GL_COLOR_ATTACHMENT2, 0);
			m_fboStaticTransparancy->attachTexture(GL_COLOR_ATTACHMENT2, m_staticTransparancyTextures.at(0));
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		update(m_transparentCityProgram, false, false, true, inputChanged, true);
		drawToSAQ(m_transparentCityProgram, &m_staticTransparancyTextures);

		globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

		mixWithEnhancedEdges(*(m_staticTransparancyTextures.at(0)), inputChanged);
	}

	unsetGlState();
}

void Painter::drawAdaptiveTransparancyPerPixelVisualization(bool inputChanged, bool forCompositing, globjects::Texture * resultTexture)
{
	setGlState();

	//########## clear A-Buffer and IndexImage #############
	update(m_clearABufferProgram, false, false, true, inputChanged, true);
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## render city to A-Buffer ############
	update(m_toABufferTypedProgram, true, false, true, inputChanged, true);
	drawToABufferOnly(m_vaoCity, m_cityIndices, m_toABufferTypedProgram, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), 0u);

	update(m_toABufferTypedProgram, false, false, false, inputChanged);
	drawToABufferOnly(m_vaoPlane, m_planeIndices, m_toABufferTypedProgram, c_planeColor, 1u);
	drawToABufferOnly(m_vaoStreets, m_streetsIndices, m_toABufferTypedProgram, c_streetsColor, 2u);
	drawToABufferOnly(m_vaoPlanePath, m_planePathIndices, m_toABufferTypedProgram, c_lineColor, 3u);
	drawToABufferOnly(m_vaoPlanePath2, m_planePath2Indices, m_toABufferTypedProgram, c_line2Color, 3u);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//drawToSAQ(m_sortABufferProgram, nullptr);

	//########## render transparancy mask texture ############
	m_fboAdaptiveTranspancyPerPixel->bind(GL_FRAMEBUFFER);
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_generalProgram, false, true, true, inputChanged);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	//########## enhance transparancy mask texture with box-filter ############
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_maskingBoxFilterForAdaptiveTransparancyProgram, false, false, true, inputChanged);
	drawToSAQ(m_maskingBoxFilterForAdaptiveTransparancyProgram, &m_adaptiveTransparancyPerPixelTextures);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	
	if (!forCompositing)
	{
		//########## Render standard city to FBO ##############
		drawStandardCity(inputChanged);
	}


	if (forCompositing && resultTexture)
	{
		//########## Render to resultTexture ##############
		m_fboAdaptiveTranspancyPerPixel->bind(GL_FRAMEBUFFER);

		if (inputChanged)
		{
			m_fboAdaptiveTranspancyPerPixel->attachTexture(GL_COLOR_ATTACHMENT2, 0);
			m_fboAdaptiveTranspancyPerPixel->attachTexture(GL_COLOR_ATTACHMENT2, resultTexture);
		}

		m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT2);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		update(m_adaptiveTransparancyPerPixelProgram, false, false, true, inputChanged, true);
		drawToSAQ(m_adaptiveTransparancyPerPixelProgram, &m_adaptiveTransparancyPerPixelTextures);

		globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	}
	else
	{
		//########## Render to the Screen ##############
		m_fboAdaptiveTranspancyPerPixel->bind(GL_FRAMEBUFFER);

		if (inputChanged)
		{
			m_fboAdaptiveTranspancyPerPixel->attachTexture(GL_COLOR_ATTACHMENT2, 0);
			m_fboAdaptiveTranspancyPerPixel->attachTexture(GL_COLOR_ATTACHMENT2, m_adaptiveTransparancyPerPixelTextures.at(2));
		}

		m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT2);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		update(m_adaptiveTransparancyPerPixelProgram, false, false, true, inputChanged, true);
		drawToSAQ(m_adaptiveTransparancyPerPixelProgram, &m_adaptiveTransparancyPerPixelTextures);

		globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

		mixWithEnhancedEdges(*(m_adaptiveTransparancyPerPixelTextures.at(2)), inputChanged);
	}

	unsetGlState();
}

void Painter::drawGhostedViewVisualization(bool inputChanged, bool forCompositing, globjects::Texture * resultTexture)
{
	setGlState();

	//########## render city to Texture ############
	m_fboGhostedView->bind(GL_FRAMEBUFFER);
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoCity, m_cityIndices, m_generalProgram);

	update(m_generalProgram, false, true, true, inputChanged);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	//########## render city(without houses) to Texture ############
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	//########## render mask texture ############
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	//########## enhance transparancy mask texture with box-filter ############
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT3);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	update(m_maskingBoxFilterForGhostedViewProgram, false, true, true, inputChanged);
	drawToSAQ(m_maskingBoxFilterForGhostedViewProgram, &m_ghostedViewTextures);

	//########## Render to the screen ##############
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT4);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	update(m_ghostedViewProgram, false, false, true, inputChanged, true);
	drawToSAQ(m_ghostedViewProgram, &m_ghostedViewTextures);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	mixWithEnhancedEdges(*(m_ghostedViewTextures.at(4)), inputChanged);

	unsetGlState();
}

void Painter::drawFenceHintsVisualization(bool inputChanged, bool forCompositing, globjects::Texture * resultTexture)
{
	setGlState();

	//########## render fence top components to texture ##############
	m_fboFenceHints->bind(GL_FRAMEBUFFER);
	m_fboFenceHints->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//TODO - is the texture parameter in drawWith... still important -> only nullptr right now
	update(m_fenceHintsCubeProgram, false, true, true, inputChanged);
	drawWithLineRepresentation(m_vaoLineVertices, m_lineIndices, m_fenceHintsCubeProgram, nullptr, c_lineColor, GL_POINTS);
	if (c_twoLines)
	{
		drawWithLineRepresentation(m_vaoLineVertices2, m_lineIndices2, m_fenceHintsCubeProgram, nullptr, c_line2Color, GL_POINTS);
	}

	update(m_fenceHintsLineProgram, false, true, true, inputChanged);
	drawWithLineRepresentation(m_vaoLineVertices, m_lineIndices, m_fenceHintsLineProgram, nullptr, c_lineColor, GL_LINE_STRIP);
	if (c_twoLines)
	{
		drawWithLineRepresentation(m_vaoLineVertices2, m_lineIndices2, m_fenceHintsLineProgram, nullptr, c_line2Color, GL_LINE_STRIP);
	}

	//########## render city to texture ##############
	m_fboFenceHints->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_generalProgram, false, true, true, inputChanged);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	update(m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoCity, m_cityIndices, m_generalProgram);

	//########## render fence gradient to texture ##############
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	update(m_fenceGradientProgram, false, true, true, inputChanged);
	drawFenceGradient(m_vaoLineVertices, m_lineIndices, c_lineColor);

	if (c_twoLines)
	{
		drawFenceGradient(m_vaoLineVertices2, m_lineIndices2, c_line2Color);
	}
	glDisable(GL_BLEND);
	
	//########## render to screen ##############
	m_fboFenceHints->setDrawBuffer(GL_COLOR_ATTACHMENT2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	update(m_fenceHintsProgram, false, false, true, inputChanged);
	drawToSAQ(m_fenceHintsProgram, &m_fenceHintsTextures);
	
	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	mixWithEnhancedEdges(*(m_fenceHintsTextures.at(2)), inputChanged);

	unsetGlState();
}

void Painter::drawFullFlatVisualization(bool inputChanged)
{
	setGlState();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_generalProgram, false, false, true, inputChanged);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	unsetGlState();
}

void Painter::drawFullFootprintVisualization(bool inputChanged)
{
	setGlState();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update(m_generalProgram, false, false, true, inputChanged);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	update(m_footprintProgram, true, false, true, inputChanged);
	drawGeneralGeometry(m_vaoCity, m_cityIndices, m_footprintProgram);

	unsetGlState();
}

void Painter::mix_outlineHints_adaptiveTransparancy_onDepth(bool inputChanged)
{
	//TODO: allgemeinerer Ansatz: mixOnDepth(enum ...) der die Visualizierungen spezifiziert
	setGlState();
	
	m_fboPerspectiveDepthMask->bind(GL_FRAMEBUFFER);
	m_fboPerspectiveDepthMask->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//TODO- updates einführen
	drawToSAQ(m_perspectiveDepthMaskProgram, &m_mix_outlineHints_adaptiveTransparancy_onDepth_textures);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	//++ first visualization ++
	//########## clear A-Buffer and IndexImage #############
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## Render city+plane+streets image to FBO ##############
	m_fboOutlineHints->bind(GL_FRAMEBUFFER);
	m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	update(m_generalProgram, false, true, true, inputChanged);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);

	update(m_generalProgram, true, true, true, inputChanged);
	drawGeneralGeometry(m_vaoCity, m_cityIndices, m_generalProgram);

	//########## Render extruded line to ABuffer ##############
	update(m_extrudedLinetoABufferOnlyProgram, false, true, true, inputChanged, true);
	//drawToABufferOnly(m_vaoLine/**/, m_vboLineIndices/**/, m_lineIndices_OLD/**/, m_toABufferOnlyProgram/*m_extrudedLinetoABufferOnlyProgram*/, false, true, glm::vec4(1.f, 1.f, 1.f, 1.f)/*, 0u, gl::GL_LINE_STRIP*/);
	drawToABufferOnly(m_vaoLineVertices, m_lineIndices, m_extrudedLinetoABufferOnlyProgram, glm::vec4(1.f, 1.f, 1.f, 1.f), 0u, gl::GL_LINE_STRIP);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	//########## Render halo image to FBO ##############
	m_fboOutlineHints->bind(GL_FRAMEBUFFER);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_outlineHintsTextures.at(0)->bindActive(GL_TEXTURE0);
	m_currentHaloColor = c_lineColor;

	drawToSAQ(m_haloLineABufferedProgram, &m_outlineHintsTextures);

	if (c_twoLines)
	{
		//########## clear A-Buffer and IndexImage #############
		drawToSAQ(m_clearABufferProgram, &m_outlineHintsTextures);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//########## Render extruded line2 to ABuffer ##############
		update(m_toABufferOnlyProgram, false, true, true, inputChanged, true);
		drawToABufferOnly(m_vaoLine2_realGeometry, m_line2Indices_OLD, m_toABufferOnlyProgram, glm::vec4(1.f, 1.f, 1.f, 1.f));
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//########## Render halo from line2 image to FBO ##############
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT2);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_outlineHintsTextures.at(0)->bindActive(GL_TEXTURE0);
		m_currentHaloColor = c_line2Color;
		drawToSAQ(m_haloLineABufferedProgram, &m_outlineHintsTextures);
	}
	glClearColor(c_clearColor.x, c_clearColor.y, c_clearColor.z, 1.0f);
	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	//########## Render to mixture_FBO texture ##############
	m_fboPerspectiveDepthMask->bind(GL_FRAMEBUFFER);
	m_fboPerspectiveDepthMask->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_outlineHintsTextures.at(0)->bindActive(GL_TEXTURE0);
	m_outlineHintsTextures.at(1)->bindActive(GL_TEXTURE1);
	m_outlineHintsTextures.at(2)->bindActive(GL_TEXTURE2);
	drawToSAQ(m_outlineHintsProgram, &m_outlineHintsTextures);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	unsetGlState();

	//++ second visualization ++
	//########## clear A-Buffer and IndexImage #############
	setGlState();
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## render city to A-Buffer ############
	update(m_toABufferTypedProgram, true, false, true, inputChanged, true);
	drawToABufferOnly(m_vaoCity, m_cityIndices, m_toABufferTypedProgram, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), 0u);

	update(m_toABufferTypedProgram, false, false, false, inputChanged);
	drawToABufferOnly(m_vaoPlane, m_planeIndices, m_toABufferTypedProgram, c_planeColor, 1u);
	drawToABufferOnly(m_vaoStreets, m_streetsIndices, m_toABufferTypedProgram, c_streetsColor, 2u);
	drawToABufferOnly(m_vaoPlanePath, m_planePathIndices, m_toABufferTypedProgram, c_lineColor, 3u);
	drawToABufferOnly(m_vaoPlanePath2, m_planePath2Indices, m_toABufferTypedProgram, c_line2Color, 3u);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## render transparancy mask texture ############
	m_fboAdaptiveTranspancyPerPixel->bind(GL_FRAMEBUFFER);
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);
	
	//########## enhance transparancy mask texture with box-filter ############
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawToSAQ(m_maskingBoxFilterForAdaptiveTransparancyProgram, &m_adaptiveTransparancyPerPixelTextures);
	
	//########## render city & flatLines to texture ############
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoCity, m_cityIndices, m_generalProgram);
	drawGeneralGeometry(m_vaoPlane, m_planeIndices, m_generalProgram, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_streetsIndices, m_generalProgram, c_streetsColor);
	drawGeneralGeometry(m_vaoPlanePath, m_planePathIndices, m_generalProgram, c_lineColor);
	drawGeneralGeometry(m_vaoPlanePath2, m_planePath2Indices, m_generalProgram, c_line2Color);
	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	
	//########## Render to mixture_FBO texture ##############
	m_fboPerspectiveDepthMask->bind(GL_FRAMEBUFFER);
	m_fboPerspectiveDepthMask->setDrawBuffer(GL_COLOR_ATTACHMENT2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_adaptiveTransparancyPerPixelTextures[0]->bindActive(GL_TEXTURE0);
	m_adaptiveTransparancyPerPixelTextures[1]->bindActive(GL_TEXTURE1);
	m_adaptiveTransparancyPerPixelTextures[2]->bindActive(GL_TEXTURE2);
	drawToSAQ(m_adaptiveTransparancyPerPixelProgram, &m_adaptiveTransparancyPerPixelTextures);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	unsetGlState();

	//++ mixing both visualizations ++
	setGlState();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_mix_outlineHints_adaptiveTransparancy_onDepth_textures.at(0)->bindActive(GL_TEXTURE0);
	m_mix_outlineHints_adaptiveTransparancy_onDepth_textures.at(1)->bindActive(GL_TEXTURE1);
	m_mix_outlineHints_adaptiveTransparancy_onDepth_textures.at(2)->bindActive(GL_TEXTURE2);
	drawToSAQ(m_mixByMaskProgram, &m_mix_outlineHints_adaptiveTransparancy_onDepth_textures);

	unsetGlState();
}

void Painter::drawToABufferOnly(globjects::VertexArray * vao, std::vector<unsigned int> & indices, globjects::Program * program, glm::vec4 specifiedColor, unsigned int typeId, GLenum drawMode)
{
	if (!vao || !program)
		return;

	setUniformOn(program, "specifiedColor", specifiedColor);

	if (typeId != 0u) {
		setUniformOn(program, "typeId", typeId);
	}

	program->use();
	vao->bind();

	vao->drawElements(drawMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT);

	vao->unbind();
	program->release();
}

void Painter::drawGeneralGeometry(globjects::VertexArray * vao, std::vector<unsigned int> & indices, globjects::Program * program, glm::vec4 specifiedColor)
{
	if (!vao || !program)
		return;

	setUniformOn(program, "specifiedColor", specifiedColor);

	program->use();
	vao->bind();

	vao->drawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT);

	vao->unbind();
	program->release();
}

void Painter::drawToSAQ(globjects::Program * program, std::vector<globjects::ref_ptr<globjects::Texture>> * textures)
{
	if (!program)
		return;

	//TODO - other setup possible?
	setUniformOn(program, "haloColor", m_currentHaloColor);

	//TODO - some textures could be statically bind only once
	if (textures)
	{
		for (int i = 0; i < textures->size(); i++)
		{
			GLint loc_texture = program->getUniformLocation(std::string("texture").append(std::to_string(i)));

			if (loc_texture >= 0)
			{
				textures->at(i)->bindActive(GL_TEXTURE0 + i);
				program->setUniform(loc_texture, i);
			}
		}
	}

	program->use();
	m_vaoSAQ->bind();

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	m_vaoSAQ->unbind();
	program->release();
}

//TODO Maybe remove later
void Painter::drawFenceGradient(globjects::VertexArray * vao, std::vector<unsigned int> & indices, glm::vec4 specifiedColor)
{
	if (!vao)
		return;

	setUniformOn(m_fenceGradientProgram, "specifiedColor", specifiedColor);

	m_fenceGradientProgram->use();
	vao->bind();

	vao->drawElements(GL_LINE_STRIP, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT);

	vao->unbind();
	m_fenceGradientProgram->release();
}

void Painter::drawWithLineRepresentation(globjects::VertexArray * vao, std::vector<unsigned int> & indices, globjects::Program * program, std::vector<globjects::ref_ptr<globjects::Texture>> * textures, glm::vec4 specifiedColor, GLenum drawMode)
{
	if (!vao || !program)
		return;

	setUniformOn(program, "specifiedColor", specifiedColor);

	//TODO - some textures could be statically bind only once
	if (textures)
	{
		for (int i = 0; i < textures->size(); i++)
		{
			GLint loc_texture = program->getUniformLocation(std::string("texture").append(std::to_string(i)));

			if (loc_texture >= 0)
			{
				textures->at(i)->bindActive(GL_TEXTURE0 + i);
				program->setUniform(loc_texture, i);
			}
		}
	}

	program->use();
	vao->bind();

	vao->drawElements(drawMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT);

	vao->unbind();
	program->release();
}

void Painter::setGlState()
{
	glClearColor(c_clearColor.x, c_clearColor.y, c_clearColor.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_2D);
}

void Painter::unsetGlState()
{
	glDisable(GL_DEPTH_TEST);
}

void Painter::setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const bool value) const
{
	GLint location = program->getUniformLocation(name);

	if (location >= 0)
		program->setUniform(location, value);
}

void Painter::setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const unsigned int value) const
{
	GLint location = program->getUniformLocation(name);

	if (location >= 0)
		program->setUniform(location, value);
}

void Painter::setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const int value) const 
{
	GLint location = program->getUniformLocation(name);

	if (location >= 0)
		program->setUniform(location, value);
}

void Painter::setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const glm::vec3 & value) const
{
	GLint location = program->getUniformLocation(name);

	if (location >= 0)
		program->setUniform(location, value);
}

void Painter::setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const glm::vec4 & value) const 
{
	GLint location = program->getUniformLocation(name);

	if (location >= 0)
		program->setUniform(location, value);
}

void Painter::setUniformOn(globjects::ref_ptr<globjects::Program> program, const std::string & name, const glm::mat4x4 & value) const 
{
	GLint location = program->getUniformLocation(name);

	if (location >= 0)
		program->setUniform(location, value);
}

void Painter::setUpMatrices()
{
	m_model = glm::mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
						  0.0f, 1.0f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.0f, 0.0f, 0.0f, 1.0f);
	m_view = glm::lookAt(m_camera.eye, m_camera.center, m_camera.up);
	m_projection = glm::perspectiveFov<float>(1.74f, m_windowWidth, m_windowHeight, 0.1f, 200.f);

	m_transform = m_projection * m_view * m_model;
}

void Painter::rotateModelByTime(double timeDifference)
{
	float rotationValue = 0.0;
	if (m_timeValid)
	{
		m_model = glm::mat4();
		rotationValue = (float)std::fmod(timeDifference * 0.06, 2 * PI); //Speed of rotation

		//freezes rotation and sets camera on a fix point
		//rotationValue = 1.3;
	}
	m_model = glm::rotate(m_model, rotationValue, glm::vec3(0.0, 1.0, 0.0));
}

void Painter::setUpFBOs()
{
	setFBO(m_fboStandardCity, &m_standardCityTexture, 1);
	setFBO(m_fboNormalVisualization, &m_normalVisualizationTextures, 1);
	setFBO(m_fboEdgeEnhancement, &m_enhancedEdgeTexture, 2);
	setFBO(m_fboOutlineHints, &m_outlineHintsTextures, 3);
	setFBO(m_fboStaticTransparancy, &m_staticTransparancyTextures, 1);
	setFBO(m_fboAdaptiveTranspancyPerPixel, &m_adaptiveTransparancyPerPixelTextures, 3);
	setFBO(m_fboGhostedView, &m_ghostedViewTextures, 5);
	setFBO(m_fboFenceHints, &m_fenceHintsTextures, 3);
	setFBO(m_fboPerspectiveDepthMask, &m_mix_outlineHints_adaptiveTransparancy_onDepth_textures, 3);
}

void Painter::setFBO(globjects::ref_ptr<globjects::Framebuffer> & fbo, std::vector<globjects::ref_ptr<globjects::Texture>> * textures, int numberOfTextures)
{
	short textureIndex = 0;
	fbo->bind(GL_FRAMEBUFFER);

	if (textures)
	{
		textures->clear();

		for (int i = 0; i < numberOfTextures; i++)
		{
			textures->push_back(new globjects::Texture());
			textureIndex = static_cast<short>(textures->size()) - 1;
			textures->at(textureIndex)->bind();
			textures->at(textureIndex)->setParameter(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			textures->at(textureIndex)->setParameter(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			textures->at(textureIndex)->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			textures->at(textureIndex)->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			textures->at(textureIndex)->image2D(0, GL_RGBA8, m_windowWidth, m_windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

			fbo->attachTexture(GL_COLOR_ATTACHMENT0 + i, textures->at(i));
		}
	}

	globjects::ref_ptr<globjects::Renderbuffer> rBuffer = new globjects::Renderbuffer();
	rBuffer->storage(GL_DEPTH_COMPONENT32F, m_windowWidth, m_windowHeight);

	fbo->attachRenderBuffer(GL_DEPTH_ATTACHMENT, rBuffer);
	fbo->setDrawBuffer(GL_COLOR_ATTACHMENT0);

	// check that our framebuffer is ok
	if (fbo->checkStatus() != GL_FRAMEBUFFER_COMPLETE)
	{
		std::printf("FBO_DIFF invalid: ");
		fbo->printStatus();
	}

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
}

void Painter::setUpABuffer()
{
	//initialize A-Buffer
	glGenTextures(1, &m_aBufferTextureArrayID);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_aBufferTextureArrayID);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
	
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, m_windowWidth, m_windowHeight, c_aBufferMaxLayers, 0, GL_RGBA, GL_FLOAT, 0);
	glBindImageTexture(0, m_aBufferTextureArrayID, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

	//initialize index texture
	glGenTextures(1, &m_aBufferIndexTexture);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_aBufferIndexTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));

	//Uses GL_R32F instead of GL_R32I that is not working in R257.15
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_windowWidth, m_windowHeight, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTexture(1, m_aBufferIndexTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	//initialize type texture as image input
	glGenTextures(1, &m_transparentTypedTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_transparentTypedTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_windowWidth, m_windowHeight, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTexture(2, m_transparentTypedTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}

void Painter::setUpCubeMap()
{
	ilInit();

	ILuint imageID;
	ilGenImages(1, &imageID);

	glGenTextures(1, &m_cubeMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMap);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, static_cast<GLint>(GL_CLAMP_TO_EDGE));

	std::string path("data/cubeTexture/street/");
	
	try {
		loadImageToGPU(path + std::string("2_xPos(Right).bmp"), GL_TEXTURE_CUBE_MAP_POSITIVE_X, imageID);
		loadImageToGPU(path + std::string("3_xNeg(Left).bmp"), GL_TEXTURE_CUBE_MAP_NEGATIVE_X, imageID);

		loadImageToGPU(path + std::string("4_yPos(Top).bmp"), GL_TEXTURE_CUBE_MAP_POSITIVE_Y, imageID);
		loadImageToGPU(path + std::string("5_yNeg(Bottom).bmp"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, imageID);

		loadImageToGPU(path + std::string("0_zNeg(Back).bmp"), GL_TEXTURE_CUBE_MAP_POSITIVE_Z, imageID);
		loadImageToGPU(path + std::string("1_zPos(Front).bmp"), GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, imageID);
	}
	catch (std::exception & e)
	{
		std::cout << e.what() << std::endl;
	}
	
	ilDeleteImages(1, &imageID);
}

void Painter::loadImageToGPU(std::string & filename, GLenum target, ILuint handle)
{
	ILboolean loadSuccess = false;
	unsigned int width, height = 0;

	ilBindImage(handle);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

	loadSuccess = ilLoadImage(filename.c_str());

	if (!loadSuccess) {
		ilDeleteImages(1, &handle);
		throw std::runtime_error("Could not load " + filename + " texture");
	}

	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);

	glTexImage2D(target, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, ilGetData());
}

void Painter::bindStaticTextures()
{
	bindStaticTextures(*m_generalProgram);
	bindStaticTextures(*m_outlineHintsProgram);
	bindStaticTextures(*m_transparentCityProgram);
	bindStaticTextures(*m_adaptiveTransparancyPerPixelProgram);
	bindStaticTextures(*m_ghostedViewProgram);
	bindStaticTextures(*m_fenceHintsProgram);
	bindStaticTextures(*m_footprintProgram);
	bindStaticTextures(*m_haloLineABufferedProgram);
	bindStaticTextures(*m_toABufferOnlyProgram);
	bindStaticTextures(*m_extrudedLinetoABufferOnlyProgram);
	bindStaticTextures(*m_clearABufferProgram);
	bindStaticTextures(*m_sortABufferProgram);
	bindStaticTextures(*m_toABufferTypedProgram);
	bindStaticTextures(*m_maskingBoxFilterForAdaptiveTransparancyProgram);
	bindStaticTextures(*m_maskingBoxFilterForGhostedViewProgram);
	bindStaticTextures(*m_fenceHintsCubeProgram);
	bindStaticTextures(*m_fenceHintsLineProgram);
	bindStaticTextures(*m_fenceGradientProgram);
	bindStaticTextures(*m_mixByMaskProgram);
	bindStaticTextures(*m_perspectiveDepthMaskProgram);
	bindStaticTextures(*m_edgeDetectionProgram);
	bindStaticTextures(*m_dilationFilterProgram);
	bindStaticTextures(*m_mixEnhancedEdgesProgram);
}