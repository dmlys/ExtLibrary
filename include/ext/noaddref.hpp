#pragma once

namespace ext
{
	// special tag type for pointing that reference counter should not be incremented, used by various smart pointers
	class noaddref_type {};
	const noaddref_type noaddref;
}
