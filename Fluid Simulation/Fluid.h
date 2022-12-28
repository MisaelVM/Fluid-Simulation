#pragma once
#ifndef FLUID_H
#define FLUID_H

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/color_space.hpp>

#include <vector>

class Fluid {
private:
	enum class ColorSpace { GRAYSCALE, HSV };

	const unsigned int size;

	float dt = 0;
	float diff;
	float visc;

	std::vector<float> pVx;
	std::vector<float> pVy;

	std::vector<float> Vx;
	std::vector<float> Vy;

	std::vector<float> s;
	std::vector<float> density;

	ColorSpace renderColorSpace;

private:
	int IndexAt(int x, int y);

	void LinSolve(int b, std::vector<float>& x, std::vector<float>& x0, float a, float c, int iter);
	void SetBnd(int b, std::vector<float>& x);

	void Diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt, int iter);
	void ClearDivergence(std::vector<float>& vx, std::vector<float>& vy, std::vector<float>& p, std::vector<float>& div, int iter);
	void Advect(int b, std::vector<float>& d, std::vector<float>& d0, std::vector<float>& vx, std::vector<float>& vy, float dt);

public:
	Fluid(const unsigned int& grid_size, const float& diffusion, const float& viscocity);

	void AddDensity(float x, float y, float amount);
	void AddVelocity(float x, float y, glm::vec2 amount);

	void Update(const float& dt);
	void Draw(void* ptr);

	void Clean();
	void SetGrayscaleSpace();
	void SetHSVSpace();
	void PrintDensity();
	void FadeDensity(int size);

	std::vector<glm::vec4> densityPixel;
};

Fluid::Fluid(const unsigned int& grid_size, const float& diffusion, const float& viscocity)
	: size(grid_size), diff(diffusion), visc(viscocity), renderColorSpace(ColorSpace::GRAYSCALE) {

	pVx = std::vector<float>(size * size);
	pVy = std::vector<float>(size * size);
	Vx = std::vector<float>(size * size);
	Vy = std::vector<float>(size * size);
	s = std::vector<float>(size * size);
	density = std::vector<float>(size * size);
	densityPixel = std::vector<glm::vec4>(size * size);
}

int Fluid::IndexAt(int x, int y) {
	if (x < 0) { x = 0; }
	if (x > size - 1) { x = size - 1; }

	if (y < 0) { y = 0; }
	if (y > size - 1) { y = size - 1; }

	return (y * size) + x;
}

void Fluid::AddDensity(float x, float y, float amount) {
	int index = IndexAt(x, y);
	density[index] += amount;
}

void Fluid::AddVelocity(float x, float y, glm::vec2 amount) {
	int index = IndexAt(x, y);
	Vx[index] += amount.x;
	Vy[index] += amount.y;
}

void Fluid::Update(const float& dt) {
	Diffuse(1, pVx, Vx, visc, dt, 16);
	Diffuse(2, pVy, Vy, visc, dt, 16);

	ClearDivergence(pVx, pVy, Vx, Vy, 16);

	Advect(1, Vx, pVx, pVx, pVy, dt);
	Advect(2, Vy, pVy, pVx, pVy, dt);

	ClearDivergence(Vx, Vy, pVx, pVy, 16);

	Diffuse(0, s, density, diff, dt, 16);
	Advect(0, density, s, Vx, Vy, dt);
}

void Fluid::Draw(void* ptr) {
	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			int index = IndexAt(i, j);
			densityPixel[index] = (renderColorSpace == ColorSpace::HSV)
				? glm::vec4(glm::rgbColor(glm::vec3(density[index], 1.0f, 1.0f)), 1.0f)
				: glm::vec4(glm::vec3(density[index]) / 255.0f, 1.0f);
		}
	}
	memcpy(ptr, densityPixel.data(), densityPixel.size() * sizeof(glm::vec4));
}

void Fluid::Clean() {
	std::fill(pVx.begin(), pVx.end(), 0.0f);
	std::fill(pVy.begin(), pVy.end(), 0.0f);
	std::fill(Vx.begin(), Vx.end(), 0.0f);
	std::fill(Vy.begin(), Vy.end(), 0.0f);
	std::fill(s.begin(), s.end(), 0.0f);
	std::fill(density.begin(), density.end(), 0.0f);
	std::fill(densityPixel.begin(), densityPixel.end(), glm::vec4(0.0f));
}

void Fluid::SetGrayscaleSpace() {
	renderColorSpace = ColorSpace::GRAYSCALE;
}

void Fluid::SetHSVSpace() {
	renderColorSpace = ColorSpace::HSV;
}

void Fluid::SetBnd(int b, std::vector<float>& x) {
	for (int i = 1; i < size - 1; i++) {
		x[IndexAt(i, 0)] = b == 2 ? -x[IndexAt(i, 1)] : x[IndexAt(i, 1)];
		x[IndexAt(i, size - 1)] = b == 2 ? -x[IndexAt(i, size - 2)] : x[IndexAt(i, size - 2)];
	}

	for (int j = 1; j < size - 1; j++) {
		x[IndexAt(0, j)] = b == 1 ? -x[IndexAt(1, j)] : x[IndexAt(1, j)];
		x[IndexAt(size - 1, j)] = b == 1 ? -x[IndexAt(size - 2, j)] : x[IndexAt(size - 2, j)];
	}

	x[IndexAt(0, 0)] = 0.33f * (x[IndexAt(1, 0)]
		+ x[IndexAt(0, 1)]
		+ x[IndexAt(0, 0)]);
	x[IndexAt(0, size - 1)] = 0.33f * (x[IndexAt(1, size - 1)]
		+ x[IndexAt(0, size - 2)]
		+ x[IndexAt(0, size - 1)]);
	x[IndexAt(size - 1, 0)] = 0.33f * (x[IndexAt(size - 2, 0)]
		+ x[IndexAt(size - 1, 1)]
		+ x[IndexAt(size - 1, 0)]);
	x[IndexAt(size - 1, size - 1)] = 0.33f * (x[IndexAt(size - 2, size - 1)]
		+ x[IndexAt(size - 1, size - 2)]
		+ x[IndexAt(size - 1, size - 1)]);
}

void Fluid::LinSolve(int b, std::vector<float>& x, std::vector<float>& x0, float a, float c, int iter) {
	float cRecip = 1.0 / c;
	for (int k = 0; k < iter; k++) {
		for (int j = 1; j < size - 1; j++) {
			for (int i = 1; i < size - 1; i++) {
				x[IndexAt(i, j)] = (x0[IndexAt(i, j)] + a
					* (x[IndexAt(i + 1, j)]
						+ x[IndexAt(i - 1, j)]
						+ x[IndexAt(i, j + 1)]
						+ x[IndexAt(i, j - 1)]
						+ x[IndexAt(i, j)]
						+ x[IndexAt(i, j)]
						)) * cRecip;
			}
		}
		SetBnd(b, x);
	}
}

void Fluid::Diffuse(int b, std::vector<float>& x, std::vector<float>& x0, float diff, float dt, int iter) {
	float a = dt * diff * (size - 2) * (size - 2);
	LinSolve(b, x, x0, a, 1 + 6 * a, iter);
}

void Fluid::ClearDivergence(std::vector<float>& vx, std::vector<float>& vy, std::vector<float>& p, std::vector<float>& div, int iter) {
	for (int j = 1; j < size - 1; j++) {
		for (int i = 1; i < size - 1; i++) {
			div[IndexAt(i, j)] = -0.5f * (
				vx[IndexAt(i + 1, j)]
				- vx[IndexAt(i - 1, j)]
				+ vy[IndexAt(i, j + 1)]
				- vy[IndexAt(i, j - 1)]
				) / size;
			p[IndexAt(i, j)] = 0;
		}
	}

	SetBnd(0, div);
	SetBnd(0, p);
	LinSolve(0, p, div, 1, 6, iter);

	for (int j = 1; j < size - 1; j++) {
		for (int i = 1; i < size - 1; i++) {
			vx[IndexAt(i, j)] -= 0.5f * (p[IndexAt(i + 1, j)] - p[IndexAt(i - 1, j)]) * size;
			vy[IndexAt(i, j)] -= 0.5f * (p[IndexAt(i, j + 1)] - p[IndexAt(i, j - 1)]) * size;
		}
	}
	SetBnd(1, vx);
	SetBnd(2, vy);
}

void Fluid::Advect(int b, std::vector<float>& d, std::vector<float>& d0, std::vector<float>& vx, std::vector<float>& vy, float dt) {
	float i0, i1, j0, j1;

	float dtx = dt * (size - 2);
	float dty = dt * (size - 2);

	float s0, s1, t0, t1;
	float tmp1, tmp2, x, y;

	float Nfloat = size;
	float ifloat, jfloat;

	int i, j;

	for (j = 1, jfloat = 1; j < size - 1; j++, jfloat++) {
		for (i = 1, ifloat = 1; i < size - 1; i++, ifloat++) {
			tmp1 = dtx * vx[IndexAt(i, j)];
			tmp2 = dty * vy[IndexAt(i, j)];
			x = ifloat - tmp1;
			y = jfloat - tmp2;

			if (x < 0.5f) x = 0.5f;
			if (x > Nfloat + 0.5f) x = Nfloat + 0.5f;
			i0 = ::floorf(x);
			i1 = i0 + 1.0f;
			if (y < 0.5f) y = 0.5f;
			if (y > Nfloat + 0.5f) y = Nfloat + 0.5f;
			j0 = ::floorf(y);
			j1 = j0 + 1.0f;

			s1 = x - i0;
			s0 = 1.0f - s1;
			t1 = y - j0;
			t0 = 1.0f - t1;

			int i0i = i0;
			int i1i = i1;
			int j0i = j0;
			int j1i = j1;

			d[IndexAt(i, j)] =
				s0 * (t0 * d0[IndexAt(i0i, j0i)] + t1 * d0[IndexAt(i0i, j1i)]) +
				s1 * (t0 * d0[IndexAt(i1i, j0i)] + t1 * d0[IndexAt(i1i, j1i)]);
		}
	}
	SetBnd(b, d);
}

void Fluid::PrintDensity() {
	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j)
			std::cout << density[IndexAt(i, j)] << "\t";
		std::cout << "\n";
	}
	std::cout << "\n";
}

#endif
