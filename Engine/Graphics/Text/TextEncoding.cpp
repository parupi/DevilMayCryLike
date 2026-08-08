#include "TextEncoding.h"

namespace {
	constexpr char32_t kReplacement = 0xFFFD;

	// 先頭バイトから、その文字が何バイトで表されるかと、先頭バイトが持つ値のビットを取り出す
	bool DecodeLeadByte(uint8_t byte, int32_t& outLength, char32_t& outValue) {
		if (byte < 0x80) { outLength = 1; outValue = byte; return true; }
		if ((byte & 0xE0) == 0xC0) { outLength = 2; outValue = byte & 0x1Fu; return true; }
		if ((byte & 0xF0) == 0xE0) { outLength = 3; outValue = byte & 0x0Fu; return true; }
		if ((byte & 0xF8) == 0xF0) { outLength = 4; outValue = byte & 0x07u; return true; }
		return false;
	}
}

std::u32string DecodeUtf8(std::string_view text) {
	std::u32string result;
	result.reserve(text.size());

	size_t i = 0;
	while (i < text.size()) {
		const uint8_t lead = static_cast<uint8_t>(text[i]);

		int32_t length = 0;
		char32_t codePoint = 0;
		if (!DecodeLeadByte(lead, length, codePoint)) {
			// 継続バイトから始まっている＝壊れている
			result.push_back(kReplacement);
			++i;
			continue;
		}

		// 途中で切れている
		if (i + length > text.size()) {
			result.push_back(kReplacement);
			break;
		}

		bool valid = true;
		for (int32_t k = 1; k < length; ++k) {
			const uint8_t continuation = static_cast<uint8_t>(text[i + k]);
			if ((continuation & 0xC0) != 0x80) {
				valid = false;
				break;
			}
			codePoint = (codePoint << 6) | (continuation & 0x3Fu);
		}

		if (!valid) {
			result.push_back(kReplacement);
			++i;
			continue;
		}

		result.push_back(codePoint);
		i += length;
	}

	return result;
}
