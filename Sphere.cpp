#include "Sphere.H"

const int Sphere::SPHERE_SECTIONS = 40;

const int Sphere::SPHERE_PITCH_SECTIONS = 20;
const int Sphere::SPHERE_YAW_SECTIONS = 40;

Sphere::Sphere(const glm::dvec3 &center, double radius)
{
	_center = center;
	_radius = radius;
}

Sphere::~Sphere()
{
}

void Sphere::setCenter(const glm::dvec3 &center)
{
	_center = center;
}

void Sphere::setRadius(double radius)
{
	_radius = radius;
}

glm::dvec3 Sphere::getCenter() const
{
	return _center;
}

double Sphere::getRadius() const
{
	return _radius;
}

//Returns true if there is an overlap
bool Sphere::intersects(const Sphere &other) const
{
	float distance = glm::length(_center - other.getCenter());
	return (_radius + other.getRadius()) >= distance;
}

//Returns true if point is less than or equal to radius away from the center. 
bool Sphere::contains(const glm::dvec3 &point) const
{
	return (glm::length(point-_center) <= _radius);
}

bool Sphere::contains(const Sphere& other) const {
	float distance = glm::length(_center - other.getCenter());
	return (_radius >= other.getRadius()) && (distance <= (_radius - other.getRadius()));
}

//Sets this to the smallest sphere that encapsulates both. 
void Sphere::merge(const Sphere &other)
{
	if (other.contains(*this)) {
        *this = other;
    } else if (! contains(other)) {
        // The farthest distance is along the axis between the centers, which
        // must not be colocated since neither contains the other.
		glm::dvec3 toMe = _center - other.getCenter();
        // Get a point on the axis from each
		toMe = glm::normalize(toMe);
        const glm::dvec3& A = _center + toMe * _radius;
		const glm::dvec3& B = other.getCenter() - toMe * other.getRadius();

        // Now just bound the A->B segment
        _center = (A + B) * 0.5;
		_radius = glm::length(A - B);
    }
    // (if this contains other, we're done)
}

void Sphere::merge(const glm::dvec3 &point)
{
	if (!contains(point)) {
		glm::dvec3 toMe = glm::normalize(_center - point);
		const glm::dvec3& A = _center + toMe * _radius;

        // Now just bound the A->point segment
        _center = (A + point) * 0.5;
		_radius = glm::length(A - point);
	}
}

bool Sphere::operator==(const Sphere& other) const {
	return (_center == other.getCenter()) && (_radius == other.getRadius());
}

bool Sphere::operator!=(const Sphere& other) const {
	return !((_center == other.getCenter()) && (_radius == other.getRadius()));
}

void Sphere::drawSphereSection(int threadID, MinVR::AbstractCameraRef camera, glm::dmat4 virtualToRoomFrame, GPUMeshMgrRef meshMgr, GLSLProgram* shader, TextureMgrRef textureMgr, MaterialRef material, CFrameMgrRef cFrameMgr)
{

    // The first time this is invoked we create a series of quad strips in a vertex array.
    // Future calls then render from the array. 

	glm::dmat4 cframe(1.0);
	cframe[0][0] = _radius;
	cframe[1][1] = _radius;
	cframe[2][2] = _radius;
	cframe = glm::column(cframe, 3, glm::dvec4(_center, 1.0));

	const std::string meshName = "DrawSphere";

	GPUMeshRef mesh = meshMgr->getMesh(meshName, threadID);
	if (mesh.get() == nullptr) {
        // The normals are the same as the vertices for a sphere
		std::vector<GPUMesh::Vertex> cpuVertexArray;
		std::vector<int> cpuIndexArray;

        for (int p = 0; p < SPHERE_PITCH_SECTIONS; ++p) {
            const double pitch0 = p * glm::pi<double>() / SPHERE_PITCH_SECTIONS;
            const double pitch1 = (p + 1) * glm::pi<double>() / SPHERE_PITCH_SECTIONS;

            const double sp0 = sin(pitch0);
            const double sp1 = sin(pitch1);
            const double cp0 = cos(pitch0);
            const double cp1 = cos(pitch1);

            for (int y = 0; y <= SPHERE_YAW_SECTIONS; ++y) {
                const double yaw = -y * (2.0*glm::pi<double>()) / SPHERE_YAW_SECTIONS;

                const double cy = cos(yaw);
                const double sy = sin(yaw);

                const glm::dvec3 v0(cy * sp0, cp0, sy * sp0);
                const glm::dvec3 v1(cy * sp1, cp1, sy * sp1);

				GPUMesh::Vertex vert;
				vert.position = v0;
				vert.normal = glm::normalize(v0);
				cpuVertexArray.push_back(vert);
				cpuIndexArray.push_back(cpuIndexArray.size() - 1);
				vert.position = v1;
				vert.normal = glm::normalize(v1);
				cpuVertexArray.push_back(vert);
				cpuIndexArray.push_back(cpuIndexArray.size() - 1);
            }
			
            const glm::dvec3& degen = glm::dvec3(1.0 * sp1, cp1, 0.0 * sp1);
			GPUMesh::Vertex vert;
			vert.position = degen;
            cpuVertexArray.push_back(vert);
			cpuIndexArray.push_back(cpuIndexArray.size() - 1);
			cpuVertexArray.push_back(vert);
			cpuIndexArray.push_back(cpuIndexArray.size() - 1);
			
        }

		const int numVertices = cpuVertexArray.size();
		const int cpuVertexByteSize = sizeof(GPUMesh::Vertex) * numVertices;
		const int cpuIndexByteSize = sizeof(int) * cpuIndexArray.size();
		
		// creates a GPUMesh object
		meshMgr->createMesh(meshName, threadID, GL_STATIC_DRAW, cpuVertexByteSize, cpuIndexByteSize, 0, cpuVertexArray, cpuIndexByteSize, &cpuIndexArray[0]);
		mesh = meshMgr->getMesh(meshName, threadID);
    }

	//camera->setObjectToWorldMatrix(virtualToRoomFrame * cframe);

	shader->setUniform("model_mat", virtualToRoomFrame * cframe);
	shader->setUniform("normal_matrix", (glm::dmat3(cframe)));
	shader->setUniform("backside", 1.0f);
	material->setShaderArgs(threadID, shader, textureMgr);
	glBindVertexArray(mesh->getVAOID());
	glDrawArrays(GL_QUAD_STRIP, 0, (int)(mesh->getFilledIndexByteSize()/sizeof(int)));
}


void Sphere::draw(int threadID, MinVR::AbstractCameraRef camera, glm::dmat4 virtualToRoomFrame, GPUMeshMgrRef meshMgr, GLSLProgram* shader, TextureMgrRef textureMgr,  MaterialRef material, CFrameMgrRef cFrameMgr) {

    int numPasses = 1;

    if (material->hasPartialCoverage()) {
        numPasses = 2;
        glCullFace(GL_FRONT);
        glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
    } else {
        glCullFace(GL_BACK);
    }

    for (int k = 0; k < numPasses; ++k) {
        drawSphereSection(threadID, camera, virtualToRoomFrame, meshMgr, shader, textureMgr, material, cFrameMgr);
        glCullFace(GL_BACK);
    }

	if (material->hasPartialCoverage()) {
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
}