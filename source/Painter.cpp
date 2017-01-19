#include "Painter.h"

#include <ratio>
#include <cmath>
#include <stdio.h>
#include <iostream>

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
	, m_timeValid(true)
{
}

Painter::~Painter()
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

	m_generalProgram = (new globjects::Program());
	m_outlineHintsProgram = (new globjects::Program());
	m_extrudedLinetoABufferOnlyProgram = (new globjects::Program());
	m_fenceGradientProgram = (new globjects::Program());
	m_haloLineABufferedProgram = (new globjects::Program());
	m_clearABufferProgram = (new globjects::Program());
	m_sortABufferProgram = (new globjects::Program());
	m_toABufferOnlyProgram = (new globjects::Program());
	m_toABufferTypedProgram = (new globjects::Program());
	m_transparentCityProgram = (new globjects::Program());
	m_adaptiveTransparancyPerPixelProgram = (new globjects::Program());
	m_maskingBoxFilterForAdaptiveTransparancyProgram = (new globjects::Program());
	m_maskingBoxFilterForGhostedViewProgram = (new globjects::Program());
	m_ghostedViewProgram = (new globjects::Program());
	m_fenceHintsProgram = (new globjects::Program());
	m_fenceHintsCubeProgram = (new globjects::Program());
	m_fenceHintsLineProgram = (new globjects::Program());
	m_footprintProgram = (new globjects::Program());
	m_perspectiveDepthMaskProgram = (new globjects::Program());
	m_mixByMaskProgram = (new globjects::Program());
	m_vaoCity = (new globjects::VertexArray());
	m_vaoLine = (new globjects::VertexArray());
	m_vaoLine2 = (new globjects::VertexArray());
	m_vaoPath = (new globjects::VertexArray());
	m_vaoPath2 = (new globjects::VertexArray());
	m_vaoSAQ = (new globjects::VertexArray());
	m_vaoPlane = (new globjects::VertexArray());
	m_vaoStreets = (new globjects::VertexArray());
	m_vboCityIndices = (new globjects::Buffer());
	m_vboLineIndices = (new globjects::Buffer());
	m_vboLine2Indices = (new globjects::Buffer());
	m_vboPathIndices = (new globjects::Buffer());
	m_vboPath2Indices = (new globjects::Buffer());
	m_vboPlaneIndices = (new globjects::Buffer());
	m_vboStreetsIndices = (new globjects::Buffer());
	m_fboOutlineHints = (new globjects::Framebuffer());
	m_fboStaticTransparancy = (new globjects::Framebuffer());
	m_fboAdaptiveTranspancyPerPixel = (new globjects::Framebuffer());
	m_fboGhostedView = (new globjects::Framebuffer());
	m_fboFenceHints = (new globjects::Framebuffer());
	m_fboPerspectiveDepthMask = (new globjects::Framebuffer());

	//line vertices
	m_vaoLineVertices = (new globjects::VertexArray());
	m_vboLineVertices = (new globjects::Buffer());

	loadGeometryToGPU(m_vaoCity, m_vboCityIndices, m_cityVertices, m_cityIndices, true);
	loadGeometryToGPU(m_vaoPlane, m_vboPlaneIndices, m_planeVertices, m_planeIndices, false);
	loadGeometryToGPU(m_vaoStreets, m_vboStreetsIndices, m_streetsVertices, m_streetsIndices, false);
	loadGeometryToGPU(m_vaoLine, m_vboLineIndices, m_lineVertices_OLD, m_lineIndices_OLD, false);
	loadGeometryToGPU(m_vaoPath, m_vboPathIndices, m_pathVertices, m_pathIndices, false);
	loadGeometryToGPU(m_vaoLineVertices, m_vboLineVertices, m_lineVertices, m_lineIndices, false);

	if (c_twoLines)
	{
		loadGeometryToGPU(m_vaoLine2, m_vboLine2Indices, m_line2Vertices, m_line2Indices, false);
		loadGeometryToGPU(m_vaoPath2, m_vboPath2Indices, m_path2Vertices, m_path2Indices, false);
	}

	//set up ressources
	setUpShader();
	setUpMatrices();
	setUpFBOs();
	m_aBufferTextureArrayID = 0;
	setUpABuffer();

	//set up timer
	std::chrono::steady_clock currentTime;
	m_applicationStartTime = currentTime.now();

	//define screen aligned quad
	m_screenAlignedQuad.push_back(glm::vec3(-1.0f, -1.0f, 0.0f));
	m_screenAlignedQuad.push_back(glm::vec3(1.0f, -1.0f, 0.0f));
	m_screenAlignedQuad.push_back(glm::vec3(-1.0f, 1.0f, 0.0f));
	m_screenAlignedQuad.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
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
}

void Painter::loadGeometry()
{
	m_meshLoader.loadFileData();
	bool verticesValid = m_meshLoader.getVertices(m_cityVertices, m_lineVertices_OLD, m_line2Vertices, m_pathVertices, m_path2Vertices, m_planeVertices, m_streetsVertices);
	bool indicesValid = m_meshLoader.getIndices(m_cityIndices, m_lineIndices_OLD, m_line2Indices, m_pathIndices, m_path2Indices, m_planeIndices, m_streetsIndices);
	bool normalsValid = m_meshLoader.getNormals(m_normals);

	m_meshLoader.getLineVertices(m_lineVertices, m_lineIndices);

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

	switch (renderMode)
	{
	case 0:
		drawNormalScene();
		break;
	case 1:
		drawOutlineHintsVisualization();
		break;
	case 2:
		drawStaticTransparancyVisualization();
		break;
	case 3:
		drawAdaptiveTransparancyPerPixelVisualization();
		break;
	case 4:
		drawGhostedViewVisualization();
		break;
	case 5:
		drawFenceHintsVisualization();
		break;
	case 6:
		drawFullFlatVisualization();
		break;
	case 7:
		drawFullFootprintVisualization();
		break;
	case 10:
		mix_outlineHints_adaptiveTransparancy_onDepth();
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
}

void Painter::drawNormalScene()
{
	setGlState();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);

	unsetGlState();
}

void Painter::drawOutlineHintsVisualization()
{
	setGlState();

	//########## clear A-Buffer and IndexImage #############
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## Render city+plane+streets image to FBO ##############
	m_fboOutlineHints->bind(GL_FRAMEBUFFER);
	m_fboOutlineHints->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);
	
	//########## Render extruded line to ABuffer ##############
	//drawToABufferOnly(m_vaoLine/**/, m_vboLineIndices/**/, m_lineIndices_OLD/**/, m_toABufferOnlyProgram/*m_extrudedLinetoABufferOnlyProgram*/, false, true, glm::vec4(1.f, 1.f, 1.f, 1.f)/*, 0u, gl::GL_LINE_STRIP*/);
	drawToABufferOnly(m_vaoLineVertices, m_vboLineVertices, m_lineIndices, m_extrudedLinetoABufferOnlyProgram, false, true, glm::vec4(1.f, 1.f, 1.f, 1.f), 0u, gl::GL_LINE_STRIP);
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

	if(c_twoLines)
	{
		//########## clear A-Buffer and IndexImage #############
		drawToSAQ(m_clearABufferProgram, &m_outlineHintsTextures);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		//########## Render extruded line2 to ABuffer ##############
		drawToABufferOnly(m_vaoLine2, m_vboLine2Indices, m_line2Indices, m_toABufferOnlyProgram, false, true, glm::vec4(1.f, 1.f, 1.f, 1.f));
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

	//########## Render to the Screen ##############
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_outlineHintsTextures.at(0)->bindActive(GL_TEXTURE0);
	m_outlineHintsTextures.at(1)->bindActive(GL_TEXTURE1);
	m_outlineHintsTextures.at(2)->bindActive(GL_TEXTURE2);
	drawToSAQ(m_outlineHintsProgram, &m_outlineHintsTextures);
	unsetGlState();
}

void Painter::drawStaticTransparancyVisualization()
{
	setGlState();

	//########## clear A-Buffer and IndexImage #############
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## render city to A-Buffer ############
	//TODO: does it really need an own fbo?
	m_fboStaticTransparancy->bind(GL_FRAMEBUFFER);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawToABufferOnly(m_vaoCity, m_vboCityIndices, m_cityIndices, m_toABufferTypedProgram, true, true, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), 0);
	drawToABufferOnly(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_toABufferTypedProgram, false, true, c_planeColor, 1);
	drawToABufferOnly(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_toABufferTypedProgram, false, true, c_streetsColor, 2);
	drawToABufferOnly(m_vaoPath, m_vboPathIndices, m_pathIndices, m_toABufferTypedProgram, false, true, c_lineColor, 3);
	drawToABufferOnly(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_toABufferTypedProgram, false, true, c_line2Color, 3);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//drawToSAQ(m_sortABufferProgram, nullptr);
	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	//########## Render to the Screen ##############
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawToSAQ(m_transparentCityProgram, nullptr);
	unsetGlState();
}

void Painter::drawAdaptiveTransparancyPerPixelVisualization()
{
	setGlState();

	//########## clear A-Buffer and IndexImage #############
	drawToSAQ(m_clearABufferProgram, nullptr);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## render city to A-Buffer ############
	drawToABufferOnly(m_vaoCity, m_vboCityIndices, m_cityIndices, m_toABufferTypedProgram, true, false);
	drawToABufferOnly(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_toABufferTypedProgram, false, false, c_planeColor, 1);
	drawToABufferOnly(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_toABufferTypedProgram, false, false, c_streetsColor, 2);
	drawToABufferOnly(m_vaoPath, m_vboPathIndices, m_pathIndices, m_toABufferTypedProgram, false, false, c_lineColor, 3);
	drawToABufferOnly(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_toABufferTypedProgram, false, false, c_line2Color, 3);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//drawToSAQ(m_sortABufferProgram, nullptr);

	//########## render transparancy mask texture ############
	m_fboAdaptiveTranspancyPerPixel->bind(GL_FRAMEBUFFER);
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);

	//########## enhance transparancy mask texture with box-filter ############
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawToSAQ(m_maskingBoxFilterForAdaptiveTransparancyProgram, &m_adaptiveTransparancyPerPixelTextures);
	
	//########## render city & flatLines to texture ############
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);
	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	//########## Render to the Screen ##############
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_adaptiveTransparancyPerPixelTextures[0]->bindActive(GL_TEXTURE0);
	m_adaptiveTransparancyPerPixelTextures[1]->bindActive(GL_TEXTURE1);
	m_adaptiveTransparancyPerPixelTextures[2]->bindActive(GL_TEXTURE2);
	drawToSAQ(m_adaptiveTransparancyPerPixelProgram, &m_adaptiveTransparancyPerPixelTextures);
	unsetGlState();
}

void Painter::drawGhostedViewVisualization()
{
	setGlState();

	//########## render city to Texture ############
	m_fboGhostedView->bind(GL_FRAMEBUFFER);
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_generalProgram, true, false);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, false, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, false, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, false, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, false, c_line2Color);

	//########## render city(without houses) to Texture ############
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, false, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, false, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, false, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, false, c_line2Color);

	//########## render transparancy mask texture ############
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);

	//########## enhance transparancy mask texture with box-filter ############
	m_fboGhostedView->setDrawBuffer(GL_COLOR_ATTACHMENT3);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawToSAQ(m_maskingBoxFilterForGhostedViewProgram, &m_ghostedViewTextures);
	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);

	//########## Render to the screen ##############
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_ghostedViewTextures[0]->bindActive(GL_TEXTURE0);
	m_ghostedViewTextures[1]->bindActive(GL_TEXTURE1);
	//TODO what happend to number 2???
	m_ghostedViewTextures[3]->bindActive(GL_TEXTURE3);
	drawToSAQ(m_ghostedViewProgram, &m_ghostedViewTextures);

	unsetGlState();
}

void Painter::drawFenceHintsVisualization()
{
	setGlState();
	//TODO: Viel. alles zusammen in einem Durchlauf möglich -> keine extra Texturen??
	//########## render fence top components to texture ##############
	m_fboFenceHints->bind(GL_FRAMEBUFFER);
	m_fboFenceHints->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawWithLineRepresentation(m_vaoLineVertices, m_vboLineVertices, m_lineIndices, m_fenceHintsCubeProgram, nullptr, true, c_lineColor, GL_POINTS);
	//glLineWidth(3.0f);
	drawWithLineRepresentation(m_vaoLineVertices, m_vboLineVertices, m_lineIndices, m_fenceHintsLineProgram, nullptr, true, c_lineColor, GL_LINE_STRIP);
	//glLineWidth(1.0f);
	//TODO: missing second line

	//########## render city to texture ##############
	m_fboFenceHints->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);
	//TODO verallgemeinern
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	drawFenceGradient(true, c_lineColor);
	//drawFenceGradient(c_line2Color);
	glDisable(GL_BLEND);

	globjects::Framebuffer::unbind(GL_FRAMEBUFFER);
	
	//########## render to screen ##############
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_fenceHintsTextures[0]->bindActive(GL_TEXTURE0);
	m_fenceHintsTextures[1]->bindActive(GL_TEXTURE1);
	drawToSAQ(m_fenceHintsProgram, &m_fenceHintsTextures);
	
	unsetGlState();
}

void Painter::drawFullFlatVisualization()
{
	setGlState();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);

	unsetGlState();
}

void Painter::drawFullFootprintVisualization()
{
	setGlState();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_footprintProgram, true, true);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);

	unsetGlState();
}

void Painter::mix_outlineHints_adaptiveTransparancy_onDepth()
{
	//TODO: allgemeinerer Ansatz: mixOnDepth(enum ...) der die Visualizierungen spezifiziert
	setGlState();
	
	m_fboPerspectiveDepthMask->bind(GL_FRAMEBUFFER);
	m_fboPerspectiveDepthMask->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);

	//########## Render extruded line to ABuffer ##############
	//drawToABufferOnly(m_vaoLine/**/, m_vboLineIndices/**/, m_lineIndices_OLD/**/, m_toABufferOnlyProgram/*m_extrudedLinetoABufferOnlyProgram*/, false, true, glm::vec4(1.f, 1.f, 1.f, 1.f)/*, 0u, gl::GL_LINE_STRIP*/);
	drawToABufferOnly(m_vaoLineVertices, m_vboLineVertices, m_lineIndices, m_extrudedLinetoABufferOnlyProgram, false, true, glm::vec4(1.f, 1.f, 1.f, 1.f), 0u, gl::GL_LINE_STRIP);
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
		drawToABufferOnly(m_vaoLine2, m_vboLine2Indices, m_line2Indices, m_toABufferOnlyProgram, false, true, glm::vec4(1.f, 1.f, 1.f, 1.f));
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
	drawToABufferOnly(m_vaoCity, m_vboCityIndices, m_cityIndices, m_toABufferTypedProgram, true, false);
	drawToABufferOnly(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_toABufferTypedProgram, false, false, c_planeColor, 1);
	drawToABufferOnly(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_toABufferTypedProgram, false, false, c_streetsColor, 2);
	drawToABufferOnly(m_vaoPath, m_vboPathIndices, m_pathIndices, m_toABufferTypedProgram, false, false, c_lineColor, 3);
	drawToABufferOnly(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_toABufferTypedProgram, false, false, c_line2Color, 3);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//########## render transparancy mask texture ############
	m_fboAdaptiveTranspancyPerPixel->bind(GL_FRAMEBUFFER);
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);
	
	//########## enhance transparancy mask texture with box-filter ############
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawToSAQ(m_maskingBoxFilterForAdaptiveTransparancyProgram, &m_adaptiveTransparancyPerPixelTextures);
	
	//########## render city & flatLines to texture ############
	m_fboAdaptiveTranspancyPerPixel->setDrawBuffer(GL_COLOR_ATTACHMENT2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawGeneralGeometry(m_vaoCity, m_vboCityIndices, m_cityIndices, m_generalProgram, true, true);
	drawGeneralGeometry(m_vaoPlane, m_vboPlaneIndices, m_planeIndices, m_generalProgram, false, true, c_planeColor);
	drawGeneralGeometry(m_vaoStreets, m_vboStreetsIndices, m_streetsIndices, m_generalProgram, false, true, c_streetsColor);
	drawGeneralGeometry(m_vaoPath, m_vboPathIndices, m_pathIndices, m_generalProgram, false, true, c_lineColor);
	drawGeneralGeometry(m_vaoPath2, m_vboPath2Indices, m_path2Indices, m_generalProgram, false, true, c_line2Color);
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

void Painter::drawToABufferOnly(globjects::VertexArray * vao, globjects::Buffer * vbo, std::vector<unsigned int> & indices, globjects::Program * program, bool useNormals, bool renderDepthValueForTextureUsage, glm::vec4 specifiedColor, unsigned int typeId, GLenum drawMode)
{
	if (!vao || !vbo || !program)
		return;

	vao->bind();
	vbo->bind(GL_ELEMENT_ARRAY_BUFFER);
	program->use();

	//get uniform locations
	GLint loc_model = program->getUniformLocation("model");
	GLint loc_view = program->getUniformLocation("view");
	GLint loc_projection = program->getUniformLocation("projection");
	GLint loc_viewVector = program->getUniformLocation("viewVector");
	GLint loc_lightVector = program->getUniformLocation("lightVector");
	GLint loc_lightVector2 = program->getUniformLocation("lightVector2");
	GLint loc_aBuffer = program->getUniformLocation("aBufferImg");
	GLint loc_aBufferIndexTexture = program->getUniformLocation("aBufferIndexImg");
	GLint loc_typeIdImg = program->getUniformLocation("typeIdImg");
	GLint loc_useNormals = program->getUniformLocation("useNormals");
	GLint loc_specifiedColor = program->getUniformLocation("specifiedColor");
	GLint loc_renderDepthValueForTextureUsage = program->getUniformLocation("renderDepthValueForTextureUsage");
	GLint loc_typeId = program->getUniformLocation("typeId");

	//bind uniforms
	if (loc_model >= 0)
		program->setUniform(loc_model, m_model);
	if (loc_view >= 0)
		program->setUniform(loc_view, m_view);
	if (loc_projection >= 0)
		program->setUniform(loc_projection, m_projection);
	if (loc_viewVector >= 0)
		program->setUniform(loc_viewVector, glm::vec3(m_camera.center - m_camera.eye));
	if (loc_lightVector >= 0)
		program->setUniform(loc_lightVector, m_sceneLight1);
	if (loc_lightVector2 >= 0)
		program->setUniform(loc_lightVector2, m_sceneLight2);
	if (loc_useNormals >= 0)
		program->setUniform(loc_useNormals, useNormals);
	if (loc_specifiedColor >= 0)
		program->setUniform(loc_specifiedColor, specifiedColor);
	if (loc_renderDepthValueForTextureUsage >= 0)
		program->setUniform(loc_renderDepthValueForTextureUsage, renderDepthValueForTextureUsage);
	if (loc_typeId >= 0)
		program->setUniform(loc_typeId, typeId);
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
		glBindTexture(GL_TEXTURE_2D, m_transparentCityTexture);
		glProgramUniform1i(program->id(), glGetUniformLocation(program->id(), "typeIdImg"), 2);
	}

	vao->drawElements(drawMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT);
	program->release();
	vao->unbind();
}

void Painter::drawGeneralGeometry(globjects::VertexArray * vao, globjects::Buffer * vbo, std::vector<unsigned int> & indices, globjects::Program * program, bool useNormals, bool renderDepthValueForTextureUsage, glm::vec4 specifiedColor)
{
	if (!vao || !vbo || !program)
		return;

	vao->bind();
	vbo->bind(GL_ELEMENT_ARRAY_BUFFER);
	program->use();

	//get uniform locations
	GLint loc_model = program->getUniformLocation("model");
	GLint loc_view = program->getUniformLocation("view");
	GLint loc_projection = program->getUniformLocation("projection");
	GLint loc_viewVector = program->getUniformLocation("viewVector");
	GLint loc_lightVector = program->getUniformLocation("lightVector");
	GLint loc_lightVector2 = program->getUniformLocation("lightVector2");
	GLint loc_useNormals = program->getUniformLocation("useNormals");
	GLint loc_specifiedColor = program->getUniformLocation("specifiedColor");
	GLint loc_renderDepthValueForTextureUsage = program->getUniformLocation("renderDepthValueForTextureUsage");

	//bind uniforms
	if (loc_model >= 0)
		program->setUniform(loc_model, m_model);
	if (loc_view >= 0)
		program->setUniform(loc_view, m_view);
	if (loc_projection >= 0)
		program->setUniform(loc_projection, m_projection);
	if (loc_viewVector >= 0)
		program->setUniform(loc_viewVector, glm::vec3(m_camera.center - m_camera.eye));
	if (loc_lightVector >= 0)
		program->setUniform(loc_lightVector, m_sceneLight1);
	if (loc_lightVector2 >= 0)
		program->setUniform(loc_lightVector2, m_sceneLight2);
	if (loc_useNormals >= 0)
		program->setUniform(loc_useNormals, useNormals);
	if (loc_specifiedColor >= 0)
		program->setUniform(loc_specifiedColor, specifiedColor);
	if (loc_renderDepthValueForTextureUsage >= 0)
		program->setUniform(loc_renderDepthValueForTextureUsage, renderDepthValueForTextureUsage);

	vao->drawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT);
	program->release();
	vao->unbind();
}

void Painter::drawToSAQ(globjects::Program * program, std::vector<globjects::ref_ptr<globjects::Texture>> * textures)
{
	m_vaoSAQ->bind();
	program->use();

	//TODO geht das nicht schöner?
	globjects::ref_ptr<globjects::Buffer> vbo_vertices = new globjects::Buffer();
	vbo_vertices->bind(GL_ARRAY_BUFFER);
	vbo_vertices->setData(m_screenAlignedQuad.size() * sizeof(glm::vec3), m_screenAlignedQuad.data(), GL_STATIC_DRAW);

	m_vaoSAQ->binding(0)->setAttribute(0);
	m_vaoSAQ->binding(0)->setBuffer(vbo_vertices, 0, sizeof(float) * 3);
	m_vaoSAQ->binding(0)->setFormat(3, GL_FLOAT, GL_FALSE, 0);
	m_vaoSAQ->enable(0);

	GLint loc_aBuffer = program->getUniformLocation("aBufferImg");
	GLint loc_aBufferIndexTexture = program->getUniformLocation("aBufferIndexImg");
	GLint loc_typeIdImg = program->getUniformLocation("typeIdImg");
	GLint loc_haloColor = program->getUniformLocation("haloColor");
	GLint loc_clearColor = program->getUniformLocation("clearColor");
	GLint loc_windowWidth = program->getUniformLocation("windowWidth");
	GLint lom_windowHeight = program->getUniformLocation("windowHeight");
	GLint loc_maxLayer = program->getUniformLocation("maxLayer");

	if(textures)
	{
		for (int i = 0; i < textures->size(); i++)
		{
			GLint loc_texture = program->getUniformLocation(std::string("texture").append(std::to_string(i)));

			if (loc_texture >= 0)
			{
				textures->at(i)->bindActive(GL_TEXTURE + i);
				program->setUniform(loc_texture, i);
			}
		}
	}
	if (loc_aBuffer >= 0)
	{
		glProgramUniform1i(program->id(), glGetUniformLocation(program->id(), "aBufferImg"), 0);
	}
	if (loc_aBufferIndexTexture >= 0)
	{
		glProgramUniform1i(program->id(), glGetUniformLocation(program->id(), "aBufferIndexImg"), 1);
	}
	if (loc_typeIdImg >= 0)
	{
		glProgramUniform1i(program->id(), glGetUniformLocation(program->id(), "typeIdImg"), 2);
	}
	if (loc_haloColor >= 0)
	{
		program->setUniform(loc_haloColor, m_currentHaloColor);
	}
	if (loc_clearColor >= 0)
	{
		program->setUniform(loc_clearColor, c_clearColor);
	}
	if (loc_windowWidth >= 0)
	{
		program->setUniform(loc_windowWidth, m_windowWidth);
	}
	if (lom_windowHeight >= 0)
	{
		program->setUniform(lom_windowHeight, m_windowHeight);
	}
	if (loc_maxLayer >= 0)
	{
		program->setUniform(loc_maxLayer, c_aBufferMaxLayers);
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	program->release();
	m_vaoSAQ->unbind();
}

//TODO Maybe remove later
void Painter::drawFenceGradient(bool renderDepthValueForTextureUsage, glm::vec4 specifiedColor)
{
	m_vaoLineVertices->bind();
	m_vboLineVertices->bind(GL_ELEMENT_ARRAY_BUFFER);
	m_fenceGradientProgram->use();

	//get uniform locations
	GLint loc_model = m_fenceGradientProgram->getUniformLocation("model");
	GLint loc_view = m_fenceGradientProgram->getUniformLocation("view");
	GLint loc_projection = m_fenceGradientProgram->getUniformLocation("projection");
	GLint loc_specifiedColor = m_fenceGradientProgram->getUniformLocation("specifiedColor");
	GLint loc_renderDepthValueForTextureUsage = m_fenceGradientProgram->getUniformLocation("renderDepthValueForTextureUsage");

	//bind uniforms
	if (loc_model >= 0)
		m_fenceGradientProgram->setUniform(loc_model, m_model);
	if (loc_view >= 0)
		m_fenceGradientProgram->setUniform(loc_view, m_view);
	if (loc_projection >= 0)
		m_fenceGradientProgram->setUniform(loc_projection, m_projection);
	if (loc_specifiedColor >= 0)
		m_fenceGradientProgram->setUniform(loc_specifiedColor, specifiedColor);
	if (loc_renderDepthValueForTextureUsage >= 0)
		m_fenceGradientProgram->setUniform(loc_renderDepthValueForTextureUsage, renderDepthValueForTextureUsage);

	m_vaoLineVertices->drawElements(GL_LINE_STRIP, static_cast<GLsizei>(m_lineIndices.size()), GL_UNSIGNED_INT);
	m_fenceGradientProgram->release();
	m_vaoLineVertices->unbind();
}

void Painter::drawWithLineRepresentation(globjects::VertexArray * vao, globjects::Buffer * vbo, std::vector<unsigned int>& indices, globjects::Program * program, std::vector<globjects::ref_ptr<globjects::Texture>> * textures, bool renderDepthValueForTextureUsage, glm::vec4 specifiedColor, GLenum drawMode)
{
	if (!vao || !vbo || !program)
		return;

	vao->bind();
	vbo->bind(GL_ELEMENT_ARRAY_BUFFER);
	program->use();

	//get uniform locations
	GLint loc_model = program->getUniformLocation("model");
	GLint loc_view = program->getUniformLocation("view");
	GLint loc_projection = program->getUniformLocation("projection");
	GLint loc_specifiedColor = program->getUniformLocation("specifiedColor");
	GLint loc_renderDepthValueForTextureUsage = program->getUniformLocation("renderDepthValueForTextureUsage");

	//bind uniforms
	if (loc_model >= 0)
		program->setUniform(loc_model, m_model);
	if (loc_view >= 0)
		program->setUniform(loc_view, m_view);
	if (loc_projection >= 0)
		program->setUniform(loc_projection, m_projection);
	if (loc_specifiedColor >= 0)
		program->setUniform(loc_specifiedColor, specifiedColor);
	if (loc_renderDepthValueForTextureUsage >= 0)
		program->setUniform(loc_renderDepthValueForTextureUsage, renderDepthValueForTextureUsage);

	if (textures)
	{
		for (int i = 0; i < textures->size(); i++)
		{
			GLint loc_texture = program->getUniformLocation(std::string("texture").append(std::to_string(i)));

			if (loc_texture >= 0)
			{
				textures->at(i)->bindActive(GL_TEXTURE + i);
				program->setUniform(loc_texture, i);
			}
		}
	}

	vao->drawElements(drawMode, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT);
	program->release();
	vao->unbind();
}

void Painter::setUpMatrices()
{
	m_model = glm::mat4x4(1.0f, 0.0f, 0.0f, 0.0f,
						  0.0f, 1.0f, 0.0f, 0.0f,
						  0.0f, 0.0f, 1.0f, 0.0f,
						  0.0f, 0.0f, 0.0f, 1.0f);
	m_view = glm::lookAt(m_camera.eye, m_camera.center, m_camera.up);
	m_projection = glm::perspectiveFov<float>(1.74f, m_windowWidth, m_windowHeight, 0.1f, 200.f);
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
	setFBO(m_fboOutlineHints, &m_outlineHintsTextures, 3);
	setFBO(m_fboStaticTransparancy, nullptr, 0);
	setFBO(m_fboAdaptiveTranspancyPerPixel, &m_adaptiveTransparancyPerPixelTextures, 3);
	setFBO(m_fboGhostedView, &m_ghostedViewTextures, 4);
	setFBO(m_fboFenceHints, &m_fenceHintsTextures, 2);
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
	glGenTextures(1, &m_transparentCityTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_transparentCityTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_windowWidth, m_windowHeight, 0, GL_RED, GL_FLOAT, 0);
	glBindImageTexture(2, m_transparentCityTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
}
