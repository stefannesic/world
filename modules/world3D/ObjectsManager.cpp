#include "ObjectsManager.h"

#include <memory>

#include "WorldIrrlicht.h"
#include "Application.h"

using namespace irr;
using namespace scene;
using namespace video;


//----- ObjectNodeHandler

ObjectNodeHandler::ObjectNodeHandler(ChunkNodeHandler * parent, const Object3D & object) 
	: _parent(parent), _meshNode(NULL) {

	updateObject3D(object);
}

ObjectNodeHandler::~ObjectNodeHandler() {
	// TODO produit une segfault si le scenemanager est d�j� supprim� (fermeture de l'application)
	// ==> RESOUDRE
}

void ObjectNodeHandler::updateObject3D(const Object3D & object) {
	SMesh * irrMesh = ObjectsManager::convertToIrrlichtMesh(object.getMesh(), _parent->_objectsManager->_driver);

	if (_meshNode == NULL) {
		_meshNode = _parent->sceneManager()->addMeshSceneNode(irrMesh, 0, -1);
	}
	else {
		_meshNode->setMesh(irrMesh);
	}

	_meshNode->setPosition(toIrrlicht(object.getPosition()));
	_meshNode->setScale(toIrrlicht(object.getScale()));

	_meshNode->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);

	irrMesh->drop();
}


//----- ChunkNodeHandler

ChunkNodeHandler::ChunkNodeHandler(ObjectsManager * manager) 
	: _objectsManager(manager) {

}

ChunkNodeHandler::~ChunkNodeHandler() {

}

void ChunkNodeHandler::updateObject(const Object3D & object) {
	_objects[_objects.size()] = ObjectNodeHandler(this, object);
}

void ChunkNodeHandler::clearObjects() {
	_objects.clear();
}

ISceneManager * ChunkNodeHandler::sceneManager() const {
	return _objectsManager->_sceneManager;
}


//----- ObjectsManager

ObjectsManager::ObjectsManager(Application & app, IrrlichtDevice * device)
	: RenderingModule(app, device) {

}

ObjectsManager::~ObjectsManager() {

}

void ObjectsManager::initialize(const World & world) {
	_chunks.clear();

	update(world);
}

void ObjectsManager::update(const World & world) {
	// Pour l'instant on ne cr�e qu'un chunk node unique.
	// TODO hierarchiser les objets par chunk
	ChunkNodeHandler & coreNode = getOrCreateNode({ 0, 0 });
	const IPointOfView & userView = _app.getUserPointOfView();

	std::unique_ptr<Scene> scene(world.createSceneFrom(userView));

	coreNode.clearObjects();

	for (Object3D * object : scene->getObjects()) {
		coreNode.updateObject(*object);
	}
}

ChunkNodeHandler & ObjectsManager::getOrCreateNode(const maths::vec2i & pos) {
	auto occ = _chunks.find(pos);

	if (occ != _chunks.end()) {
		return occ->second;
	}
	else {
		return _chunks[pos] = ChunkNodeHandler(this);
	}
}


SMesh * ObjectsManager::convertToIrrlichtMesh(const Mesh & mesh, IVideoDriver * driver) {
	SMesh * irrMesh = new SMesh();
	s32 maxPrimitives = driver->getMaximalPrimitiveCount();

	auto & positions = mesh.getVertices<VType::POSITION>();
	auto & texCoords = mesh.getVertices<VType::TEXTURE>();
	auto & normals = mesh.getVertices<VType::NORMAL>();

	auto & faces = mesh.getFaces();
	int primitiveCount = 0;

	int bufID = -1;
	SMeshBuffer * buffer;

	for (const Face & face : faces) {
		if (face.vertexCount() != 3)
			continue;

		// Switch to next buffer if the current one is filled
		if (bufID == -1 || primitiveCount + 3 >= maxPrimitives) {
			// last calculations on current buffer
			if (bufID != -1) {
				buffer->recalculateBoundingBox();
			}

			// switch to new buffer
			bufID++;

			if (bufID < irrMesh->getMeshBufferCount()) {
				buffer = (SMeshBuffer*)irrMesh->getMeshBuffer(bufID);
			}
			else {
				buffer = new SMeshBuffer();
				irrMesh->addMeshBuffer(buffer);
				buffer->drop();
			}

			buffer->Vertices.set_used(faces.size() * 3);
			buffer->Indices.set_used(faces.size() * 3);
		}

		// Add face data
		for (int i = 0; i < 3; i++) {
			int posID = face.getID<VType::POSITION>(i);
			int texID = face.getID<VType::TEXTURE>(i);
			int normID = face.getID<VType::NORMAL>(i);

			S3DVertex& v = buffer->Vertices[primitiveCount];
			v.Pos = toIrrlicht(positions[posID].toVec3() * 100); // TODO check if posID == -1;
			
			if (texID != -1)
				v.TCoords = toIrrlicht(positions[texID].toVec2());

			if (normID != -1)
				v.Normal = toIrrlicht(positions[normID].toVec3());

			v.Color = SColor(255, 255, 255, 255);

			buffer->Indices[primitiveCount] = primitiveCount;

			primitiveCount++;
		}
	}

	irrMesh->setDirty();
	irrMesh->recalculateBoundingBox();

	return irrMesh;
}