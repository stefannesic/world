#include <iostream>
#include <stdexcept>

#include <armadillo/armadillo>

#include <worldapi/assets/ObjLoader.h>
#include <worldapi/assets/Scene.h>
#include <worldapi/assets/Material.h>
#include <worldapi/assets/Interop.h>
#include <worldapi/IOUtil.h>
#include <worldapi/maths/Perlin.h>
#include <worldapi/terrain/Terrain.h>
#include <worldapi/terrain/TerrainGenerator.h>
#include <worldapi/terrain/TerrainTexmapBuilder.h>

using namespace arma;
using namespace world;

//DECLARATIONS
void testTinyObjLoader(int argc, char** argv);
void testRepeatable(int argc, char** argv);
void testPerlin(int argc, char** argv);


//MAIN
int main(int argc, char** argv) {
	testRepeatable(argc, argv);
}



//IMPLEMENTATIONS
void testTinyObjLoader(int argc, char** argv) {
	if (argc >= 2) {
		try {
			ObjLoader obj;
			std::cout << "Lecture du mesh" << std::endl;

			std::unique_ptr<Scene> scene(obj.read(argv[1]));
			std::cout << "Mesh lu !" << std::endl;

			std::vector<Object3D*> output;
			scene->getObjects(output);
			std::shared_ptr<Mesh> mesh2 = output.at(0)->getMeshPtr();

			std::cout << "Reecriture du fichier sous un autre nom" << std::endl;
			obj.write(*scene, "tests/result.obj");
		}
		catch (std::exception & e) {
			std::cout << e.what() << std::endl;
		}
	}
	else {
		std::cout << "Usage : test [nom d'un fichier]" << std::endl;
	}
}

void testRepeatable(int argc, char** argv) {

    int size = 257;
    int octaves = 4;
    int freq = 4;
    float persistence = 0.4;

	Perlin perlin;

    // CREATION D'UN PERLIN REPETABLE
    std::cout << "G�n�ration de bruit de perlin r�p�table..." << std::endl;
    Mat<double> repeatable = perlin.generatePerlinNoise2D(size, 0, octaves, freq, persistence, true);
    Image repeat(repeatable);
    repeat.write("tests/repeat.png");

    std::cout << "G�n�ration du terrain associ�..." << std::endl;
    Terrain tr(repeatable);
    Scene repeatableobj;
	repeatableobj.createObject(std::shared_ptr<Mesh>(tr.convertToMesh()));

	ObjLoader loader;
    loader.write(repeatableobj, "tests/repeatable");
}

void testPerlin(int argc, char** argv) {

	//ANALYSE DES ARGUMENTS

	int size = 129;
	int octaves = 4;
	int freq = 4;
	float persistence = 0.3;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		try {
			int argval = std::stoi(arg.substr(1));
			if ("s" == arg.substr(0, 1)) {
				size = argval;
			}
			if ("o" == arg.substr(0, 1)) {
				octaves = argval;
			}
			if ("f" == arg.substr(0, 1)) {
				freq = argval;
			}
		}
		catch (std::exception &) {
			std::cout << "erreur dans la ligne de commande, utilisation des param�tres par d�faut..." << std::endl;
		}
	}

	//CREATION DU DOSSIER DE TESTS
	std::cout << "Cr�ation du dossier de tests..." << std::endl;
	createDirectory("tests");

	Perlin perlin;

	// @Deprecated
    // CREATION DE DEUX PERLIN ET JOIN
    try {
        std::cout << "G�n�ration de deux bruits de perlin..." << std::endl;
        auto perlin1 = perlin.generatePerlinNoise2D(size, 0, octaves, freq, persistence);
        auto perlin2 = perlin.generatePerlinNoise2D(size, 0, octaves, freq, persistence);
        std::cout << "Join des deux bruits de perlin..." << std::endl;
        perlin.join(perlin1, perlin2, perlin::Direction::AXIS_Y, octaves, freq, persistence, true);
        Image perlin1img(perlin1);
        Image perlin2img(perlin2);
        perlin1img.write("tests/perlin1.png");
        perlin2img.write("tests/perlin2.png");

		/*Terrain t1(perlin1);
		Terrain t2(perlin2);

		ObjLoader files;
		files.addMesh(std::shared_ptr<Mesh>(t1.convertToMesh()));
		files.addMesh(std::shared_ptr<Mesh>(t2.convertToMesh(0, 1, 0, 1.0, 1.0, 0.4)));
		files.write("tests/join.obj");*/
    }
    catch (std::exception &e) {
        std::cerr << "Le test de join a lev� une exception : " << e.what() << std::endl;
    }
    
	//CREATION DU GENERATEUR

	PerlinTerrainGenerator generator(0, octaves, freq, persistence);

	//GENERATION DE L'IMAGE DU TERRAIN

	std::cout << "G�n�ration du terrain..." << std::endl;
    Terrain terrain(size);
    generator.process(terrain);

	std::cout << "ecriture de l'image du terrain..." <<std::endl;
	Image image = terrain.convertToImage();
	try {
		image.write("tests/terrain.png");
	}
	catch (std::exception & e) {
		std::cout << "erreur : " << e.what() << std::endl;
	}

	//ECRITURE DU FICHIER OBJ DU TERRAIN

	ObjLoader meshIO;

	std::cout << "Conversion du terrain en mesh... " << std::endl;
	Scene scene1;
	std::shared_ptr<Mesh> mesh = std::shared_ptr<Mesh>(terrain.convertToMesh());
	scene1.createObject(mesh);
	
	std::shared_ptr<Material> material = std::make_shared<Material>("material01");
	material->setMapKd("test2_tex.png");
	material->setKs(0, 0, 0);
	scene1.addMaterial(material);

	std::cout << "Ecriture du mesh dans un fichier .obj..." << std::endl;

	try {
		meshIO.write(scene1, "tests/terrain.obj");
	}
	catch (std::exception & e) {
		std::cout << "erreur : " << e.what() << std::endl;
	}

	//GENERATION DE LA TEXTURE DU TERRAIN

	std::cout << "creation de la texture" << std::endl;

	//---
	TerrainTexmapBuilder texmapBuilder(0, 255);

	std::vector<ColorPart> slice1;
	slice1.emplace_back(47.0 / 255, 128.0 / 255, 43.0 / 255, 0.75);
	slice1.emplace_back(65.0 / 255, 53.0 / 255, 22.0 / 255, 0.25);

	std::vector<ColorPart> slice2;
	slice2.emplace_back(47.0 / 255, 128.0 / 255, 43.0 / 255, 0.15);
	slice2.emplace_back(65.0 / 255, 53.0 / 255, 22.0 / 255, 0.25);
	slice2.emplace_back(0.25, 0.25, 0.25, 0.6);

	std::vector<ColorPart> slice3;
	slice3.emplace_back(1, 1, 1, 0.7);
	slice3.emplace_back(0.25, 0.25, 0.25, 0.3);
	
	texmapBuilder.addSlice(1, slice1);
	texmapBuilder.addSlice(180, slice2);
	texmapBuilder.addSlice(230, slice3);
	//---

	arma::Mat<double> randomArray = perlin.generatePerlinNoise2D(size * 8, 0, 7, 16, (float)0.9);
	Image texture = generator.generateTexture(terrain, texmapBuilder, randomArray);

	std::cout << "ecriture de la texture..." << std::endl;
	try {
		texture.write("tests/test2_tex.png");
	}
	catch (std::exception & e) {
		std::cout << "erreur : " << e.what() << std::endl;
	}

	std::cout << "ecriture de la carte" << std::endl;
	try {
		Image(texmapBuilder.convertToMap()).write("tests/test2_map.png");
	}
	catch (std::exception & e) {
		std::cout << "erreur : " << e.what() << std::endl;
	}

	// CREATION DES SUBDIVISIONS DU TERRAIN
	std::cout << "g�n�ration des subdivisions" << std::endl;
	TerrainSubdivisionTree * terrainTree = generator.generateSubdivisions(terrain, 4, 1);
	std::cout << "g�n�ration termin�e !" << std::endl;

	std::cout << "Conversion en mesh de 4 sous-terrains..." << std::endl;
	Scene subterrainScene;
	TerrainSubdivisionTree & subtree = terrainTree->getSubtree(0, 0);
	std::shared_ptr<Mesh> submesh = std::shared_ptr<Mesh>(subtree.convertToSubmesh());
	subterrainScene.createObject(submesh);
	subterrainScene.createObject(std::shared_ptr<Mesh>(terrainTree->getSubtree(1, 0).convertToSubmesh()));
	subterrainScene.createObject(std::shared_ptr<Mesh>(terrainTree->getSubtree(1, 1).convertToSubmesh()));
	subterrainScene.createObject(std::shared_ptr<Mesh>(terrainTree->getSubtree(0, 1).convertToSubmesh()));
	
	std::cout << "Ecriture du fichier .obj..." << std::endl;
	meshIO.write(subterrainScene, "tests/subterrain");
	std::cout << "Fichier �crit !" << std::endl;
	
	std::cout << "Ecriture de l'image associ�e..." << std::endl;
	Image image2 = subtree.terrain().convertToImage();
	image2.write("tests/subterrain.png");
	std::cout << "Image �crite !" << std::endl;
}