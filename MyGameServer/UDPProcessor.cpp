#include "UDPProcessor.h"

bool CUDPProcessor::Run() {
	if (IsRun()) return false;
	_isRun = true;
	return true;
}

CUDPProcessor::CUDPProcessor() : _isRun(false)
{
}


CUDPProcessor::~CUDPProcessor()
{
}
