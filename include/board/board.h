#pragma once
#include <kernel/settings.h>
#define stringify(x) #x
#define GEN_INC_PATH(A) <board/A/board.h>
#include GEN_INC_PATH(BOARD_TYPE)

