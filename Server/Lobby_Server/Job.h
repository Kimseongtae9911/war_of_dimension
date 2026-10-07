#pragma once
#include <ServerCore/Concurrency.h>
using IJob = wod::core::IJob;
template<class Func> using Job = wod::core::Job<Func>;
