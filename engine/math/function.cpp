#include "function.h"

float Normalize(float value) {
	// 値が0でない場合、絶対値が1になるように正規化
	if (value != 0.0f) {
		return value / fabs(value); // 符号を保ちながら1または-1に正規化
	}
	return 0.0f; // 0の時はそのまま0を返す
}

// 線形補間 (Linear Interpolation)
float Lerp(float a, float b, float t) { return (1.0f - t) * a + t * b; }


Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix) {
	Vector3 result;
	result.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
	result.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
	result.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;
	result.z /= w;
	return result;
}



float cotf(float theta) { return 1.0f / std::tanf(theta); }

Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip) {
	return {1.0f / aspectRatio * cotf(fovY / 2.0f), 0, 0, 0, 0, cotf(fovY / 2.0f), 0, 0, 0, 0, farClip / (farClip - nearClip), 1.0f, 0, 0, -nearClip * farClip / (farClip - nearClip), 0};
};

Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	return {2.0f / (right - left),           0,   0, 0, 0, 2.0f / (top - bottom), 0, 0, 0, 0, 1.0f / (farClip - nearClip), 0, (left + right) / (left - right), (top + bottom) / (bottom - top),
	        nearClip / (nearClip - farClip), 1.0f};
};

Matrix4x4 MakeViewPortMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
	return {width / 2.0f, 0, 0, 0, 0, -height / 2.0f, 0, 0, 0, 0, maxDepth - minDepth, 0, left + width / 2.0f, top + height / 2.0f, minDepth, 1.0f};
}
Vector3 Cross(const Vector3& v1, const Vector3& v2) { return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x}; };

Matrix4x4 LookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
	Vector3 zaxis = Normalize(target - eye);     // forward
	Vector3 xaxis = Normalize(Cross(up, zaxis)); // right
	Vector3 yaxis = Cross(zaxis, xaxis);         // up

	Matrix4x4 result = {
	    xaxis.x,
	    yaxis.x,
	    zaxis.x,
	    0,
	    xaxis.y,
	    yaxis.y,
	    zaxis.y,
	    0,
	    xaxis.z,
	    yaxis.z,
	    zaxis.z,
	    0,
	    -(xaxis.x * eye.x + xaxis.y * eye.y + xaxis.z * eye.z),
	    -(yaxis.x * eye.x + yaxis.y * eye.y + yaxis.z * eye.z),
	    -(zaxis.x * eye.x + zaxis.y * eye.y + zaxis.z * eye.z),
	    1};

	return result;
}

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
	Vector3 result{
	    v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
	    v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
	    v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2],
	};
	return result;
}

Vector3 ExtractTranslation(const Matrix4x4& matrix) {
	Vector3 translation;
	translation.x = matrix.m[3][0];
	translation.y = matrix.m[3][1];
	translation.z = matrix.m[3][2];
	return translation;
}

Matrix4x4 DirectionToDirection(const Vector3& from, const Vector3& to)
{
	Vector3 fromNorm = Normalize(from);
	Vector3 toNorm = Normalize(to);

	// 回転軸を計算
	Vector3 axis = Cross(fromNorm, toNorm);
	float sinAngle = std::sqrt(Dot(axis, axis));  // 軸の長さは sin(角度) と等しい
	float cosAngle = Dot(fromNorm, toNorm);       // 内積は cos(角度) と等しい

	// 特殊ケース: ベクトルがほぼ一致する場合（回転不要）
	if (sinAngle < 1e-6f) {
		return MakeIdentity4x4();
	}

	// 特殊ケース: 180度の回転（ベクトルが正反対の場合）
	if (cosAngle < -1.0f + 1e-6f) {
		// 例えば (1,0,0) を (0,1,0) にマッピングするような任意の軸が必要
		axis = std::abs(fromNorm.x) > 0.9f ? Vector3{ 0, 1, 0 } : Vector3{ 1, 0, 0 };
		axis = Normalize(Cross(fromNorm, axis));
		sinAngle = 1.0f;
		cosAngle = -1.0f;
	}
	else {
		// 軸を正規化
		axis = Normalize(axis);
	}

	// ロドリゲスの回転公式を使って回転行列を作成
	float x = axis.x, y = axis.y, z = axis.z;
	float oneMinusCos = 1.0f - cosAngle;

	Matrix4x4 rotationMatrix;
	rotationMatrix.m[0][0] = cosAngle + x * x * oneMinusCos;
	rotationMatrix.m[0][1] = x * y * oneMinusCos + z * sinAngle;
	rotationMatrix.m[0][2] = x * z * oneMinusCos - y * sinAngle;
	rotationMatrix.m[0][3] = 0.0f;

	rotationMatrix.m[1][0] = y * x * oneMinusCos - z * sinAngle;
	rotationMatrix.m[1][1] = cosAngle + y * y * oneMinusCos;
	rotationMatrix.m[1][2] = y * z * oneMinusCos + x * sinAngle;
	rotationMatrix.m[1][3] = 0.0f;

	rotationMatrix.m[2][0] = z * x * oneMinusCos + y * sinAngle;
	rotationMatrix.m[2][1] = z * y * oneMinusCos - x * sinAngle;
	rotationMatrix.m[2][2] = cosAngle + z * z * oneMinusCos;
	rotationMatrix.m[2][3] = 0.0f;

	rotationMatrix.m[3][0] = 0.0f;
	rotationMatrix.m[3][1] = 0.0f;
	rotationMatrix.m[3][2] = 0.0f;
	rotationMatrix.m[3][3] = 1.0f;

	return rotationMatrix;
}