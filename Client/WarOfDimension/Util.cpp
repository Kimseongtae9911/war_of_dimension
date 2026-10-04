#include "stdafx.h"
#include "Util.h"


random_device Util::m_randomDevice;
mt19937 Util::m_randomEngine(m_randomDevice());