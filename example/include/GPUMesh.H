/**
 * /author Bret Jackson
 *
 * /file  GPUMesh.H
 * /brief Represents a VAO and VBO for a particular rendering thread
 *
 */ 

#ifndef GPUMESH_H
#define GPUMESH_H

#include "GL/glew.h"
#ifdef _WIN32
#include "GL/wglew.h"
#elif (!defined(__APPLE__))
#include "GL/glxew.h"
#endif

#include <glm/glm.hpp>
#include <boost/thread.hpp>
#include <memory>

typedef std::shared_ptr<class GPUMesh> GPUMeshRef;

class GPUMesh : public std::enable_shared_from_this<GPUMesh>
{
public:
	struct Vertex {
		glm::dvec3 position;
		glm::dvec3 normal;
		glm::dvec2 texCoord0;
	};

	struct FloatVertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord0;
	};

	// Creates a vao and vbo. Usage should be GL_STATIC_DRAW, GL_DYNAMIC_DRAW, etc. Leave data empty to just allocate but not upload.
	GPUMesh(GLenum usage, int allocateVertexByteSize, int allocateIndexByteSize, int vertexOffset, const std::vector<Vertex> &data, int indexByteSize=0, int* index=nullptr);
	~GPUMesh();

	// Returns the number of bytes allocated in the vertexVBO
	int getAllocatedVertexByteSize() const;
	int getAllocatedIndexByteSize() const;
	// Returns the number of bytes actually filled with data in the vertexVBO
	int getFilledVertexByteSize() const;
	int getFilledIndexByteSize() const;
	
	GLuint getVAOID() const;

	// Update the vbos. startByteOffset+dataByteSize must be <= allocatedByteSize
	void updateVertexData(int startByteOffset, int vertexOffset, const std::vector<Vertex> &data);
	void updateIndexData(int startByteOffset, int indexByteSize, int* index);

private:
	GLuint _vaoID;
	GLuint _vertexVBO;
	GLuint _indexVBO;

	int _allocatedVertexByteSize;
	int _allocatedIndexByteSize;
	int _filledVertexByteSize;
	int _filledIndexByteSize;

	boost::thread::id _creatorThreadID;
};

#endif