/**
 * /author Bret Jackson
 *
 * /file  Sphere.H
 * /brief Represents a sphere to be used for bounds tests
 *
 */ 

#ifndef SPHERE_H
#define SPHERE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/constants.hpp> 
#include <glm/gtx/orthonormalize.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <memory>
#include "Material.H"
#include "GPUMeshMgr.H"
#include "GPUMesh.H"
#include "CFrameMgr.H"
#include <map>
#include <MVRCore/AbstractCamera.H>
#include <MVRCore/CameraOffAxis.H>
#include "GLSLProgram.H"

typedef std::shared_ptr<class Sphere> SphereRef;

class Sphere : public std::enable_shared_from_this<Sphere>
{
public:
	Sphere(const glm::dvec3 &center, double radius);
	~Sphere();
	void setCenter(const glm::dvec3 &center);
	void setRadius(double radius);
	glm::dvec3 getCenter() const;
	double getRadius() const;

	//Returns true if point is less than or equal to radius away from the center. 
	bool contains(const glm::dvec3 &point) const;
	bool contains(const Sphere &other) const;

	//Returns true if there is an overlap
	bool intersects(const Sphere &other) const;

	//Sets this to the smallest sphere that encapsulates both. 
	void merge(const Sphere &other);
	void merge(const glm::dvec3 &point);

	bool operator==(const Sphere& other) const;
	bool operator!=(const Sphere& other) const;
	
	void draw(int threadID, MinVR::AbstractCameraRef camera, glm::dmat4 virtualToRoomFrame, GPUMeshMgrRef meshMgr, GLSLProgram* shader, TextureMgrRef textureMgr, MaterialRef material, CFrameMgrRef cFrameMgr);

private:
	glm::dvec3 _center;
	double _radius;

	static const int SPHERE_SECTIONS;
	static const int SPHERE_PITCH_SECTIONS;
	static const int SPHERE_YAW_SECTIONS;
	void drawSphereSection(int threadID, MinVR::AbstractCameraRef camera, glm::dmat4 virtualToRoomFrame, GPUMeshMgrRef meshMgr, GLSLProgram* shader, TextureMgrRef textureMgr, MaterialRef material, CFrameMgrRef cFrameMgr);
};
#endif
