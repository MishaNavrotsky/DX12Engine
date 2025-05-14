#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif
#ifndef NOMINMAX
# define NOMINMAX
#endif
#include <windows.h>
#include <initguid.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "include/d3dx12/d3dx12.h"
#include <dxcapi.h>
#include <objbase.h>
#include <vector>
#include "external/BS_thread_pool.hpp"
#include <functional>
#include <filesystem>
#include <thread>
#include <mutex>
#include <atomic>
#include <future>
#include <iostream>
#include <set>
#include <span>

#include <string>
#include <wrl.h>
#include <shellapi.h>
