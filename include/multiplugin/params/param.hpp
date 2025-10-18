/**
 * @file param.hpp
 * @brief Parameter wrapper template for the MultiPlugin parameter system
 */

#pragma once

#include "specs.hpp"
#include <type_traits>
#include <cstdint>  // for uint32_t

namespace mp {

// Host-specific handle types
#ifdef BUILD_FOR_AE
struct HFloat { int id; };
struct HInt { int id; };
struct HColor { int id; };
struct HBool { int id; };
struct HFlag { int param_index; uint32_t mask; };  // For bitwise flag operations
struct HButton { int id; };
struct HPopup { int id; };
struct HAngle { int id; };
struct HPoint2D { int id; };
struct HGroup { int id; };
struct HCustom { int id; };
#endif

#ifdef BUILD_FOR_OFX
// Use OFX C API handle types (defined in ofxCore.h)
// OfxParamHandle is already defined by OFX SDK
struct HFloat { void* param; };
struct HInt { void* param; };
struct HColor { void* param; };
struct HBool { void* param; };
struct HButton { void* param; };
struct HPopup { void* param; };
struct HAngle { void* param; };
struct HPoint2D { void* param; };
struct HGroup { void* param; };
#endif

/**
 * @brief Parameter wrapper connecting a parameter specification to a member variable
 * @tparam Bag The parameter struct type containing all parameter values
 * @tparam Spec The parameter specification type (SpecFloat, SpecInt, etc.)
 * @tparam Handle The host-specific handle type
 * @tparam Val The value type stored in the Bag member
 */
template<class Bag, class Spec, class Handle, class Val>
struct Param {
    using bag_type = Bag;
    using spec_type = Spec;
    using handle_type = Handle;
    using val_type = Val;

    Val Bag::* member;  // Member pointer
    Spec spec;          // Parameter specification
    Handle handle;      // Host-specific handle (filled during build)
};

/**
 * @brief Specialization for void Bag (e.g., groupBegin/groupEnd)
 */
template<class Spec, class Handle, class Val>
struct Param<void, Spec, Handle, Val> {
    using bag_type = void;
    using spec_type = Spec;
    using handle_type = Handle;
    using val_type = Val;

    std::nullptr_t member;  // No member pointer for void Bag
    Spec spec;              // Parameter specification
    Handle handle;          // Host-specific handle (filled during build)
};

} // namespace mp
