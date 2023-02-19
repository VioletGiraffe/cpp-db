#pragma once

namespace WAL {

using OpID = uint32_t;

enum class OpStatus : uint8_t {
	Successful = 0xEE,
	Failed = 0x11
};

struct OperationCompletedMarker {
	static constexpr uint8_t markerID = 0xDD;
	OpStatus status;
};

}
