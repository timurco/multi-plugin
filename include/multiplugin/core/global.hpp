#pragma once

#include <mutex>
#include <memory>

namespace mp {

/**
 * @brief Thread-safe global singleton for plugin-wide data
 *
 * Usage:
 * @code
 * struct MyGlobalData {
 *     bool gpuAvailable = false;
 *     int licenseLevel = 0;
 * };
 *
 * // Access from anywhere
 * auto& global = GlobalSingleton<MyGlobalData>::get();
 * global.gpuAvailable = true;
 * @endcode
 *
 * @tparam T Data type to store globally
 */
template<typename T>
class GlobalSingleton {
private:
    static T* instance_;
    static std::once_flag init_flag_;

    GlobalSingleton() = delete;
    ~GlobalSingleton() = delete;
    GlobalSingleton(const GlobalSingleton&) = delete;
    GlobalSingleton& operator=(const GlobalSingleton&) = delete;

public:
    /**
     * @brief Get the singleton instance
     *
     * Thread-safe lazy initialization on first call
     *
     * @return Reference to the global instance
     */
    static T& get() {
        std::call_once(init_flag_, []() {
            instance_ = new T();
        });
        return *instance_;
    }

    /**
     * @brief Initialize the singleton explicitly
     *
     * Call this from plugin initialization (GlobalSetup in AE, constructor in OFX)
     * to ensure the singleton is created at a predictable time.
     *
     * @return Reference to the global instance
     */
    static T& initialize() {
        return get(); // Force initialization
    }

    /**
     * @brief Reset the singleton (mainly for testing)
     *
     * WARNING: This is not thread-safe and should only be used
     * when you're certain no other threads are accessing the singleton
     */
    static void reset() {
        if (instance_) {
            delete instance_;
            instance_ = nullptr;
            new (&init_flag_) std::once_flag(); // Reset the once_flag
        }
    }

    /**
     * @brief Check if singleton is initialized
     * @return true if instance exists
     */
    static bool isInitialized() {
        return instance_ != nullptr;
    }
};

// Static member definitions
template<typename T>
T* GlobalSingleton<T>::instance_ = nullptr;

template<typename T>
std::once_flag GlobalSingleton<T>::init_flag_;

/**
 * @brief Convenience alias for cleaner syntax
 */
template<typename T>
using Global = GlobalSingleton<T>;

} // namespace mp