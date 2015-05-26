#include "GPUMesh.H"

GPUMesh::GPUMesh(GLenum usage, int allocateVertexByteSize, int allocateIndexByteSize, int vertexOffset, const std::vector<Vertex> &data, int indexByteSize/*=0*/, int* index/*=nullptr*/)
{
	_creatorThreadID = boost::this_thread::get_id();

	assert(data.size() - vertexOffset >= 0);
	int dataByteSize =  sizeof(FloatVertex) * (data.size()-vertexOffset);

	_allocatedVertexByteSize = allocateVertexByteSize;
	_allocatedIndexByteSize = allocateIndexByteSize;
	_filledVertexByteSize = dataByteSize;
	_filledIndexByteSize = indexByteSize;

	// create the vao
	glGenVertexArrays(1, &_vaoID);
	glBindVertexArray(_vaoID);
				
	// create the vbo
	glGenBuffers(1, &_vertexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, _vertexVBO);

	// initialize size
	glBufferData(GL_ARRAY_BUFFER, allocateVertexByteSize, NULL, usage);

	if (dataByteSize > 0) {
		//buffer data

		// We store the vertices as doubles for precision, but only render with floats.
		// Convert here
		std::vector<FloatVertex> vertices;
		FloatVertex vert;
		for(int i=vertexOffset; i < data.size(); i++) {
			vert.position = glm::vec3(data[i].position.x, data[i].position.y, data[i].position.z);
			vert.normal = glm::vec3(data[i].normal.x, data[i].normal.y, data[i].normal.z);
			vert.texCoord0 = glm::vec2(data[i].texCoord0.x, data[i].texCoord0.y);
			vertices.push_back(vert);
		}

		glBufferSubData(GL_ARRAY_BUFFER, 0, dataByteSize, &vertices[0]);
	}

	// set up vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FloatVertex), (void*)offsetof(FloatVertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(FloatVertex), (void*)offsetof(FloatVertex, normal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FloatVertex), (void*)offsetof(FloatVertex, texCoord0));

	// Create indexstream
	glGenBuffers(1, &_indexVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexVBO);

	// copy data into the buffer object
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, allocateIndexByteSize, NULL, usage);
	
	if (indexByteSize > 0 && index != nullptr) {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexByteSize, index);
	}

	// unbind the vao
	glBindVertexArray(0);
}

GPUMesh::~GPUMesh()
{
	// Make sure that this mesh is deleted by the creator thread so that the correct context is active
	//assert(_creatorThreadID == boost::this_thread::get_id());

	if (_creatorThreadID == boost::this_thread::get_id()) {
		//Assumes object is deleted with the correct context current
		glDeleteBuffers(1, &_vertexVBO);
		glDeleteBuffers(1, &_indexVBO);
		glDeleteVertexArrays(1, &_vaoID);
	}
}

int GPUMesh::getAllocatedVertexByteSize() const
{
	return _allocatedVertexByteSize;
}

int GPUMesh::getAllocatedIndexByteSize() const
{
	return _allocatedIndexByteSize;
}

int GPUMesh::getFilledVertexByteSize() const
{
	return _filledVertexByteSize;
}

int GPUMesh::getFilledIndexByteSize() const
{
	return _filledIndexByteSize;
}
	
GLuint GPUMesh::getVAOID() const
{
	return _vaoID;
}

void GPUMesh::updateVertexData(int startByteOffset, int vertexOffset, const std::vector<Vertex> &data)
{
	assert(startByteOffset <= _filledVertexByteSize);

	int dataByteSize = sizeof(FloatVertex)*(data.size()-vertexOffset);

	int totalBytes = startByteOffset + dataByteSize;
	if (_filledVertexByteSize < totalBytes) {
		_filledVertexByteSize = totalBytes;
	}

	assert(_filledVertexByteSize <= _allocatedVertexByteSize);
	glBindBuffer(GL_ARRAY_BUFFER, _vertexVBO);

	// We store the vertices as doubles for precision, but only render with floats.
	// Convert here
	std::vector<FloatVertex> vertices;
	FloatVertex vert;
	for(int i=vertexOffset; i < data.size(); i++) {
		vert.position = glm::vec3(data[i].position.x, data[i].position.y, data[i].position.z);
		vert.normal = glm::vec3(data[i].normal.x, data[i].normal.y, data[i].normal.z);
		vert.texCoord0 = glm::vec2(data[i].texCoord0.x, data[i].texCoord0.y);
		vertices.push_back(vert);
	}

	glBufferSubData(GL_ARRAY_BUFFER, startByteOffset, dataByteSize, &vertices[0]);
}

void GPUMesh::updateIndexData(int startByteOffset, int indexByteSize, int* index)
{
	assert(startByteOffset <= _filledIndexByteSize);\
	int totalBytes = startByteOffset + indexByteSize;
	if (_filledIndexByteSize < totalBytes) {
		_filledIndexByteSize = totalBytes;
	}
	assert(_filledIndexByteSize <= _allocatedIndexByteSize);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexVBO);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, startByteOffset, indexByteSize, index);
}