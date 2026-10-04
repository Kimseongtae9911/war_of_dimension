#include "pch.h"

namespace wod_server {
    std::unique_ptr<CMatchMgr> CMatchMgr::m_instance;

    bool CMatchMgr::Initialize()
    {
        return true;
    }

    bool CMatchMgr::Release()
    {
        return true;
    }

}