#pragma once

#include <cstdbool>
#include <cstdint>
#include <cstddef>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <cstring>

#ifdef __PSP__
#include <sys/endian.h>
#else
#include <endian.h>
#endif

#include <unistd.h>
#include <getopt.h>

#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <array>

using namespace std;

#include "Dump.h"
#include "SmartPtr.h"

