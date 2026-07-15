#include "Easing.h"
#include"cmath"
#include "numbers"

float easeInSine(float x) {
	return 1 - std::cosf((x * std::numbers::pi_v<float>) / 2);
}
float easeOutSine(float x) {
	return std::sinf((x * std::numbers::pi_v<float>) / 2);
}
float easeInOutSine(float x) {
	return -(std::cosf(std::numbers::pi_v<float> * x) - 1) / 2;
}
float easeInQuad(float x) {
	return x * x;
}
float easeOutQuad(float x) {
	return 1 - (1 - x) * (1 - x);
}
float easeInOutQuad(float x) {
	return x < 0.5 ? 2 * x * x : 1 - std::powf(-2 * x + 2, 2) / 2;
}
float easeInCubic(float x) {
	return x * x * x;
}
float easeOutCubic(float x) {
	return 1 - std::powf(1 - x, 3);
}
float easeInOutCubic(float x) {
	return x < 0.5 ? 4 * x * x * x : 1 - std::powf(-2 * x + 2, 3) / 2;
}
float easeInQuart(float x) {
	return x * x * x * x;
}
float easeOutQuart(float x) {
	return 1 - std::powf(1 - x, 4);
}
float easeInOutQuart(float x) {
	return x < 0.5 ? 8 * x * x * x * x : 1 - std::powf(-2 * x + 2, 4) / 2;
}
float easeInQuint(float x) {
	return x * x * x * x * x;
}
float easeOutQuint(float x) {
	return 1 - std::powf(1 - x, 5);
}
float easeInOutQuint(float x) {
	return x < 0.5 ? 16 * x * x * x * x * x : 1 - std::powf(-2 * x + 2, 5) / 2;
}
float easeInExpo(float x) {
	return x == 0 ? 0 : std::powf(2, 10 * x - 10);
}
float easeOutExpo(float x) {
	return x == 1 ? 1 : 1 - std::powf(2, -10 * x);
}
float easeInOutExpo(float x) {
	return x == 0
		? 0
		: x == 1
		? 1
		: x < 0.5 ? std::powf(2, 20 * x - 10) / 2
		: (2 - std::powf(2, -20 * x + 10)) / 2;
}
float easeInCirc(float x) {
	return 1 - std::sqrtf(1 - std::powf(x, 2));
}
float easeOutCirc(float x) {
	return  std::sqrtf(1 - std::powf(x - 1, 2));
}
float easeInOutCirc(float x) {
	return x < 0.5
		? (1 - std::sqrtf(1 - std::powf(2 * x, 2))) / 2
		: (std::sqrtf(1 - std::powf(-2 * x + 2, 2)) + 1) / 2;
}
float easeInBack(float x) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1;

	return c3 * x * x * x - c1 * x * x;
}
float easeOutBack(float x) {
	const float c1 = 1.70158f;
	const float c3 = c1 + 1;

	return 1 + c3 * std::powf(x - 1, 3) + c1 * std::powf(x - 1, 2);
}
float easeInOutBack(float x) {
	const float  c1 = 1.70158f;
	const float  c2 = c1 * 1.525f;

	return x < 0.5
		? (std::powf(2 * x, 2) * ((c2 + 1) * 2 * x - c2)) / 2
		: (std::powf(2 * x - 2, 2) * ((c2 + 1) * (x * 2 - 2) + c2) + 2) / 2;
}
float easeInElastic(float x) {
	const float  c4 = (2 * std::numbers::pi_v<float>) / 3;

	return x == 0
		? 0
		: x == 1
		? 1
		: -std::powf(2, 10 * x - 10) * std::sinf((x * 10 - 10.75f) * c4);
}
float easeOutElastic(float x) {
	const float  c4 = (2 * std::numbers::pi_v<float>) / 3;

	return x == 0
		? 0
		: x == 1
		? 1
		: std::powf(2, -10 * x) * std::sinf((x * 10 - 0.75f) * c4) + 1;
}
float easeInOutElastic(float x) {
	const float  c5 = (2 * std::numbers::pi_v<float>) / 4.5f;

	return x == 0
		? 0
		: x == 1
		? 1
		: x < 0.5
		? -(std::powf(2, 20 * x - 10) * std::sinf((20 * x - 11.125f) * c5)) / 2
		: (std::powf(2, -20 * x + 10) * std::sinf((20 * x - 11.125f) * c5)) / 2 + 1;
}
float easeOutBounce(float x) {
	const float  n1 = 7.5625f;
	const float  d1 = 2.75f;

	if (x < 1 / d1) {
		return n1 * x * x;
	}
	else if (x < 2 / d1) {
		return n1 * (x -= 1.5f / d1) * x + 0.75f;
	}
	else if (x < 2.5 / d1) {
		return n1 * (x -= 2.25f / d1) * x + 0.9375f;
	}
	else {
		return n1 * (x -= 2.625f / d1) * x + 0.984375f;
	}
}
float easeInBounce(float x) {
	return 1 - easeOutBounce(1 - x);
}
float easeInOutBounce(float x) {
	return x < 0.5
		? (1 - easeOutBounce(1 - 2 * x)) / 2
		: (1 + easeOutBounce(2 * x - 1)) / 2;
}
