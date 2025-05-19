// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

#ifndef DUMP_H
#define DUMP_H

#include "common.h"
#include "SmartPtr.h"

void dumpBlocks(smartPtr<uint8_t>& fcBuf, size_t startOffset, ssize_t length, int blockLen);
void dumpLines(smartPtr<uint8_t>& fcBuf, size_t startOffset, ssize_t length, int blockLen);

#endif  // DUMP_H
