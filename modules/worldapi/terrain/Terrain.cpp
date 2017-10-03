#include "Terrain.h"

#include <algorithm>
#include <iostream>
#include <math.h>

#include <worldapi/maths/MathsHelper.h>
#include <worldapi/maths/Interpolation.h>

#include "../Interop.h"
#include "../Image.h"
#include "../Mesh.h"
#include "../MeshOps.h"
#include "../Stream.h"

using namespace maths;
using namespace img;
using namespace arma;

Terrain::Terrain(int size) : 
	_array(size, size),
	_texture(std::make_unique<Image>(1, 1, ImageType::RGB)) {

}

Terrain::Terrain(const Mat<double> & data) :
	_array(data), 
	_texture(nullptr) {

	if (data.n_rows != data.n_cols) {
		throw std::runtime_error("Terrain must be squared !");
	}
}

Terrain::Terrain(const Terrain &terrain)
		: _array(terrain._array),
		  _texture(std::make_unique<Image>(*terrain._texture)) {
	
}

Terrain::Terrain(Terrain &&terrain)
		: _array(std::move(terrain._array)),
		  _texture(std::move(terrain._texture)) {

}

Terrain::~Terrain() {

}

double Terrain::getZ(double x, double y) const {
	int posX = (int) (x * _array.n_rows);
	int posY = (int) (y * _array.n_cols);

	return _array(posX, posY);
}

double Terrain::getZInterpolated(double x, double y) const {
	int width = (int)_array.n_rows;
	int height = (int)_array.n_cols;

	x *= width;
	y *= height;
	// TODO d�finir le comportement de mani�re plus exacte
	int posX1 = min(width - 1, (int) floor(x));
	int posY1 = min(height - 1, (int) floor(y));
	int posX2 = posX1 + 1; if (posX2 >= width) posX2 = posX1;
	int posY2 = posY1 + 1; if (posY2 >= height) posY2 = posY1;

	double ip1 = interpolateLinear(posX1, _array(posX1, posY1), posX2, _array(posX2, posY1), x);
	double ip2 = interpolateLinear(posX1, _array(posX1, posY2), posX2, _array(posX2, posY2), x);

	return interpolateLinear(posY1, ip1, posY2, ip2, y);
}

Mesh * Terrain::convertToMesh(float sizeX, float sizeY, float sizeZ) const {
	return convertToMesh(-sizeX / 2, -sizeY / 2, 0, sizeX, sizeY, sizeZ);
}

Mesh * Terrain::convertToMesh(float offsetX, float offsetY, float offsetZ, float sizeX, float sizeY, float sizeZ) const {
	Mesh * mesh = new Mesh();

	// R�servation de m�moire
	int vertCount = _array.n_rows * _array.n_cols;
	mesh->allocateVertices<VType::POSITION>(vertCount);
	mesh->allocateVertices<VType::TEXTURE>(vertCount);
	mesh->allocateVertices<VType::NORMAL>(vertCount);

	//Vertices
	int i = 0;
	for (int x = 0; x < _array.n_rows; x++) {
		for (int y = 0; y < _array.n_cols; y++) {
			float xd = (float)x / (_array.n_rows - 1);
			float yd = (float)y / (_array.n_cols - 1);

			Vertex<VType::POSITION> vert;

			vert.add(xd * sizeX + offsetX)
				.add(yd * sizeY + offsetY)
				.add(_array(x, y) * sizeZ + offsetZ);

			mesh->addVertex(vert);


			Vertex<VType::TEXTURE> vertext;

			vertext.add(xd)
				.add(1 - yd);

			mesh->addVertex(vertext);
		}
	}

	//Faces
	auto indice = [this](int x, int y)->int { return x * this->_array.n_cols + y; };
	mesh->allocateFaces((_array.n_rows - 1) * (_array.n_cols - 1) * 2);

	for (int x = 0; x < _array.n_rows - 1; x++) {
		for (int y = 0; y < _array.n_cols - 1; y++) {
			Face face1, face2;
			
			face1.addVertexUniqueID(indice(x, y));
			face1.addVertexUniqueID(indice(x + 1, y));
			face1.addVertexUniqueID(indice(x, y + 1));

			face2.addVertexUniqueID(indice(x + 1, y + 1));
			face2.addVertexUniqueID(indice(x, y + 1));
			face2.addVertexUniqueID(indice(x + 1, y));

			mesh->addFace(face1);
			mesh->addFace(face2);
		}
	}

	MeshOps::recalculateNormals(*mesh);

	return mesh;
}

Image Terrain::convertToImage() const {
	return Image(this->_array);
}

void Terrain::writeRawData(std::ostream &stream, float height, float offset) const {
	bin_ostream binstream(stream);

	for (int x = 0 ; x < _array.n_rows ; x++) {
		for (int y = 0 ; y < _array.n_cols ; y++) {
			binstream << (float) _array(x, y) * height + offset;
		}
	}
}

char * Terrain::getRawData(int &rawDataSize, float height, float offset) const {
	float * result = new float[_array.n_rows * _array.n_cols];

	for (int x = 0 ; x < _array.n_rows ; x++) {
		for (int y = 0 ; y < _array.n_cols ; y++) {
			result[x * _array.n_cols + y] = (float) _array(x, y) * height + offset;
		}
	}

	rawDataSize = getRawDataSize();

	return (char*) result;
}

int Terrain::getRawDataSize() const {
	return (int) (_array.n_rows * _array.n_cols) * sizeof(float) / sizeof(char);
}

Image Terrain::getTexture() const {
	if (_texture == nullptr) throw std::runtime_error("No texture");
	return *_texture;
}

const Image & Terrain::texture() const {
	if (_texture == nullptr) throw std::runtime_error("No texture");
	return *_texture;
}

vec2i Terrain::getPixelPos(double x, double y) const {
	return {
		(int) maths::min(x * _array.n_rows, _array.n_rows - 1),
		(int) maths::min(y * _array.n_cols, _array.n_cols - 1)
	};
}