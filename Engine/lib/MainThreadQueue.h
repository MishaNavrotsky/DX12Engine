#include "stdafx.h"

#pragma once

inline tbb::concurrent_queue<std::function<void(void)>> MainThreadQueue;

