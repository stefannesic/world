#include "Tree.h"


Tree::Tree() {

}

Tree::~Tree() {

}

void Tree::fillScene(Scene & scene) const {
	scene.createObject(_trunkMesh);
}