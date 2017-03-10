
# 
# Configuration for all sub-projects
# 

# Generate version-header
string(TOUPPER ${META_PROJECT_NAME} META_PROJECT_NAME_UPPER)
#configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME}/${META_PROJECT_NAME}-version.h)

# Target name
set(target occlusion_hints)

# 
# Sources
# 

set(source_path  "${CMAKE_CURRENT_SOURCE_DIR}")

set(headers
	${source_path}/Painter.h
	${source_path}/MeshLoader.h
)

set(sources
	${source_path}/main.cpp
	${source_path}/Painter.cpp
	${source_path}/MeshLoader.cpp
)

add_executable(${target}
    ${sources}
)

target_sources(${target} 
	PUBLIC
    ${sources}
	${header}
)

set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    FOLDER "${IDE_FOLDER}"
)

target_include_directories(${target}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}

    PUBLIC
    ${GLM_INCLUDE_DIR}
	${GLFW_INCLUDE_DIR}
	${IL_INCLUDE_DIR}
	${ASSIMP_INCLUDE_DIR}
	${GLBINDING_INCLUDE_DIR}
	${GLOBJECTS_INCLUDE_DIR}

    INTERFACE
)

target_link_libraries(${target}
    PRIVATE

    PUBLIC
	${DEFAULT_LIBRARIES}
	cpplocate::cpplocate
	${ASSIMP_LIBRARIES}
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${ILUT_LIBRARIES}
	glbinding::glbinding
	globjects::globjects
	${GLFW_LIBRARIES}

    INTERFACE
)

# 
# Deployment
# 

# Deploy generated headers
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME} DESTINATION include COMPONENT dev)
