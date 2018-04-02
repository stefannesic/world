#include "ReliefMap.h"

#include <utility>
#include <list>
#include <tuple>

#include "worldapi/assets/Image.h"
#include "../maths/MathsHelper.h"
#include "../maths/Interpolation.h"

using namespace arma;


namespace world {

	// -----
	ReliefMapGenerator::ReliefMapGenerator() : _rng(time(NULL)) {

	}


	// -----
	const double CustomWorldRMGenerator::PIXEL_UNIT = 10;

	CustomWorldRMGenerator::CustomWorldRMGenerator(double biomeDensity, int limitBrightness) :
			_biomeDensity(biomeDensity),
			_limitBrightness(limitBrightness),
			_diffLaw(std::make_unique<relief::CustomWorldDifferential>()) {

	}

	CustomWorldRMGenerator::CustomWorldRMGenerator(const CustomWorldRMGenerator &other)
			: _biomeDensity(other._biomeDensity),
			  _limitBrightness(other._limitBrightness),
			  _diffLaw(other._diffLaw->clone()) {

	}

	void CustomWorldRMGenerator::setBiomeDensity(float biomeDensity) {
		_biomeDensity = biomeDensity;
	}

	void CustomWorldRMGenerator::setLimitBrightness(int p) {
		_limitBrightness = p;
	}

	void CustomWorldRMGenerator::setDifferentialLaw(const relief::diff_law &law) {
		_diffLaw = std::unique_ptr<relief::diff_law>(law.clone());
	}

	void CustomWorldRMGenerator::generate(Terrain &height, Terrain &heightDiff) const {
		// Nombre de biomes � g�n�rer.
		int size = height.getSize() * height.getSize();
		int biomeCount = (int) (_biomeDensity * (double) size / (PIXEL_UNIT * PIXEL_UNIT));


		// -> Cr�ation de la grille pour le placement des points de mani�re al�atoire,
		// mais avec une distance minimum.

		// Calcul des dimensions de la grille
		double minDistance = PIXEL_UNIT / 2.0 * sqrt(_biomeDensity);

		// TODO unifier
		int sliceCount = max<int>((int) ((float) height.getSize() / minDistance), 1);
		float sliceSize = (float) height.getSize() / (float) sliceCount;

		// the terrains are squared so there are as much slices as there are cases per slice
		int caseCount = sliceCount;
		float caseSize = sliceCount;

		// Pr�paration de la grille
		typedef std::pair<vec2d, vec2d> point; // pour plus de lisibilit�
		std::vector<std::vector<point>> pointsMap;
		pointsMap.reserve(sliceCount);
		std::vector<std::pair<int, int>> grid;
		grid.reserve(sliceCount * caseCount);

		for (int x = 0; x < sliceCount; x++) {
			pointsMap.emplace_back();

			std::vector<point> &slice = pointsMap[x];
			slice.reserve(caseCount);

			for (int y = 0; y < caseCount; y++) {
				grid.emplace_back(x, y);
				slice.emplace_back(vec2d(-1, -1), vec2d(0, 0));
			}
		}

		// G�n�ration des points
		std::uniform_real_distribution<double> rand(0.0, 1.0);

		for (int i = 0; i < biomeCount; i++) { // TODO dans les cas limites la grille peut se vider totalement
			// G�n�ration des coordonn�es des points
			int randIndex = (int) (rand(_rng) * grid.size());
			std::pair<int, int> randPoint = grid.at(randIndex);
			grid.erase(grid.begin() + randIndex);

			int x = randPoint.first;
			int y = randPoint.second;

			// Calcul des limites dans lesquelles on peut avoir un point
			double limNegX = 0;
			double limPosX = sliceSize;
			double limNegY = 0;
			double limPosY = caseSize;

			if (x > 0) {
				auto negXCase = pointsMap[x - 1][y];

				if (negXCase.first.x >= 0) {
					limNegX = negXCase.first.x;
				}
			}
			if (x < sliceCount - 1) {
				auto posXCase = pointsMap[x + 1][y];

				if (posXCase.first.x >= 0) {
					limPosX = posXCase.first.x;
				}
			}
			if (y > 0) {
				auto negYCase = pointsMap[x][y - 1];

				if (negYCase.first.y >= 0) {
					limNegY = negYCase.first.y;
				}
			}
			if (y < caseCount - 1) {
				auto posYCase = pointsMap[x][y + 1];

				if (posYCase.first.y >= 0) {
					limPosY = posYCase.first.y;
				}
			}

			// � partir des limites on peut d�terminer la position random du point
			double randX = rand(_rng);
			double randY = rand(_rng);

			// TODO L'utilisateur n'a aucun contr�le sur le premier param�tre.
			double elevation = rand(_rng);
			double diff = (*_diffLaw)(std::pair<double, double>(elevation, rand(_rng)));

			pointsMap[x][y] = std::make_pair<vec2d, vec2d>(
					vec2d(
							randX * (limPosX - limNegX) + limNegX + x * sliceSize,
							randY * (limPosY - limNegY) + limNegY + y * caseSize),
					vec2d(elevation, diff));
		}


		// -> Interpolation des valeurs des points pour reconstituer une map

		// Cr�ation des interpolateur
		IDWInterpolator<vec2d, vec2d> interpolator(_limitBrightness);

		// On pr�pare les donn�es � interpoler.
		for (auto &slice : pointsMap) {
			for (auto &pt : slice) {
				if (pt.first.x >= 0) {
					interpolator.addData(pt.first, pt.second);
				}
			}
		}

		// On remplit la grille � l'aide de l'interpolateur. And enjoy.
		for (int x = 0; x < height.getSize(); x++) {
			for (int y = 0; y < height.getSize(); y++) {
				vec2d pt(x + 0.5, y + 0.5);
				vec2d result = interpolator.getData(pt);
				height(x, y) = result.x;
				heightDiff(x, y) = result.y;
			}
		}
	}
}