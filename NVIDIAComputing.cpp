#include "PALComputing.hpp"

#ifdef SAI_USE_CUBLAS
// TODO
#else
DeviceRef PALInitializeComputingForNVIDIA() {
	return nullptr;
}
void PALFinalizeComputingForNVIDIA(DeviceRef&) {}
#endif