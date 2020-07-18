#pragma once

#include "../storage/storage_io_interface.hpp"

template<typename Record, typename StorageImplementation>
[[nodiscard]] bool serializeRecord(Record&& r, StorageIO<StorageImplementation>& io)
{
	static_assert (Record::isRecord());
}

template<typename ReceivingFunctor, typename StorageImplementation>
[[nodiscard]] bool deserializeRecord(ReceivingFunctor&& receiver, StorageIO<StorageImplementation>& io)
{

}
