#pragma once

enum class BlendCategory {
	Opaque, // None
	Transparent,   // Normal α
	Additive,      // Add / Sub
	ColorFilter,   // Multiply / Screen
	Count
};

constexpr BlendCategory kForwardCategories[] =
{
	BlendCategory::Transparent,
	BlendCategory::Additive,
	BlendCategory::ColorFilter,
};