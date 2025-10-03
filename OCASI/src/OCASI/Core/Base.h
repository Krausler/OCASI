#pragma once

#include <cstdint>

#include "OCBase/OCBase.h"
#include "OCBase/STDTypedefs.h"

#include "OCASI/Core/Error.h"

#define OCASI_LOG_INFO(...) OCB_LOG_INFO_TEMPLATE("OCASI", __VA_ARGS__)
#define OCASI_LOG_WARN(...) OCB_LOG_WARNING_TEMPLATE("OCASI", __VA_ARGS__)
#define OCASI_LOG_ERROR(...) OCB_LOG_ERROR_TEMPLATE("OCASI", __VA_ARGS__)

#define OCASI_ASSERT(...) OCB_ASSERT_TEMPLATE("OCASI", __VA_ARGS__)


