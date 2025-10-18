/**
 * @brief Parameter context for button callbacks
 * Provides access to parameter values and modification during callbacks
 */

#pragma once

namespace mp {

// Forward declarations
template<class Bag, class... Ps>
class ParamSet;

/**
 * @brief Parameter context passed to button callbacks
 *
 * Allows reading and writing parameter values during button click handling.
 *
 * Usage:
 *   mp::button(5, "Randomize", "Generate random values")
 *       .onClick([](auto& ctx) {
 *           ctx.setValue(&MyParams::seed, rand() % 1000);
 *           ctx.setValue(&MyParams::mix, 75.0f);
 *       })
 */
template<typename Bag>
class ParamContext {
    Bag bag_;
    const void* source_;  // Pointer to AESource or OfxSource
    const void* paramSet_;  // Pointer to ParamSet
    bool is_ae_;  // true = AESource, false = OfxSource

public:
    ParamContext(const Bag& initial_bag, const void* source, const void* paramSet, bool is_ae)
        : bag_(initial_bag), source_(source), paramSet_(paramSet), is_ae_(is_ae) {}

    /**
     * @brief Get current parameter value
     * @param member Pointer to member in Bag
     * @return Current value
     */
    template<typename T>
    T getValue(T Bag::*member) const {
        return bag_.*member;
    }

    /**
     * @brief Set parameter value
     * @param member Pointer to member in Bag
     * @param value New value to set
     *
     * This will update both the local bag and the actual parameter in the host.
     */
    template<typename T>
    void setValue(T Bag::*member, const T& value);

    /**
     * @brief Access to parameter bag
     */
    Bag& getBag() { return bag_; }
    const Bag& getBag() const { return bag_; }

    /**
     * @brief Get source pointer (for internal use by setValue)
     */
    const void* getSource() const { return source_; }
    const void* getParamSet() const { return paramSet_; }
    bool isAE() const { return is_ae_; }
};

// Implementation of setValue is in param_set.hpp after ParamSet is defined

} // namespace mp
