#pragma once
#include "c18/abi.h"
#include <string>
#include <vector>

extern "C" std::string c18_bad_name();
extern "C" c18_status C18_CALL c18_bad_vector(std::vector<unsigned char>* bytes);
