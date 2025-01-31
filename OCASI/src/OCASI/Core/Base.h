#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>

using Path = std::filesystem::path;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T, typename ...Args>
constexpr SharedPtr<T> MakeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T, typename ...Args>
constexpr UniquePtr<T> MakeUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

#include "OCASI/Core/Logger.h"
#include "OCASI/Core/Debug.h"