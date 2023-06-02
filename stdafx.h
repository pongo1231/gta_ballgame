#pragma once

#include "cam.h"
#include "entity_iterator.h"
#include "hash.h"
#include "logging.h"
#include "memory.h"
#include "natives.h"

#include <minhook/include/MinHook.h>
#include <scripthookv/inc/main.h>
#include <scripthookv/inc/natives.h>

#include <wingdi.h>

#include <algorithm>
#include <array>
#include <exception>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <type_traits>
#include <unordered_map>
