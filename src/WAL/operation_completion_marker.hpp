#pragma once

namespace WAL {

enum class OpStatus : uint8_t {
	Successful = 0xEE,
	Failed = 0x11
};

using OperationIdType = uint32_t;

struct OperationCompletedMarker {
	static constexpr uint8_t markerID = 0xDD;
	OpStatus status;
};

}
