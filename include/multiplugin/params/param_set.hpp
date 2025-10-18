/**
 * @file param_set.hpp
 * @brief Parameter set template for managing collections of parameters
 */

#pragma once

#include "param.hpp"
#include "helpers.hpp"
#include <tuple>
#include <type_traits>

namespace mp {

/**
 * @brief Parameter set holding a tuple of parameters for a given Bag type
 * @tparam Bag The parameter struct type containing all parameter values
 * @tparam Ps Parameter types
 */
template<class Bag, class... Ps>
class ParamSet {
    std::tuple<Ps...> params_;

public:
    using bag_type = Bag;
    static constexpr int count = sizeof...(Ps);

    /**
     * @brief Construct a parameter set from a list of parameters
     */
    explicit ParamSet(Ps... ps) : params_{ps...} {}

    /**
     * @brief Build all parameters using the provided builder
     * @tparam Builder Builder type (AEParamBuilder or OFXParamBuilder)
     */
    template<class Builder>
    void build(Builder& builder) {
        std::apply([&](auto&... p) {
            ((buildOne(builder, p)), ...);
        }, params_);
    }

    /**
     * @brief Fetch all parameter values from the source into the Bag
     * @tparam Source Source type (AESource or OfxSource)
     * @return Error code (0 if successful)
     */
    template<class Source>
    int fetch(const Source& source, Bag& bag) const {
        int err = 0;
        std::apply([&](const auto&... p) {
            ((err = fetchOne(source, bag, p)), ...);
        }, params_);
        return err;
    }

    /**
     * @brief Set a single parameter value by member pointer
     * @tparam T Value type
     * @tparam Source Source type
     * @return Error code (0 if successful)
     */
    template<class T, class Source>
    int setValue(const Source& source, T Bag::*member_ptr, const T& value) const {
        int err = 1; // Not found by default
        std::apply([&](const auto&... p) {
            ((err = setValueOne(source, p, member_ptr, value)), ...);
        }, params_);
        return err;
    }

    /**
     * @brief Set multiple parameter values from a Bag instance
     * @tparam Source Source type
     * @return Error code (0 if successful)
     */
    template<class Source>
    int setValues(const Source& source, const Bag& bag) const {
        int err = 0;
        std::apply([&](const auto&... p) {
            ((err = setValueOne(source, p, bag)), ...);
        }, params_);
        return err;
    }

    /**
     * @brief Get unique name by disk ID
     * @return Unique name string, or empty string if not found
     */
    std::string getUniqueName(unsigned int disk_id) const {
        std::string result;
        std::apply([&](const auto&... p) {
            ((disk_id == p.spec.disk_id ? (result = p.spec.unique_name, true) : false) || ...);
        }, params_);
        return result;
    }

    /**
     * @brief Get disk ID by unique name
     * @return Disk ID, or -1 if not found
     */
    int getDiskIdByUniqueName(const std::string& unique_name) const {
        int result = -1;
        std::apply([&](const auto&... p) {
            ((unique_name == p.spec.unique_name ? (result = static_cast<int>(p.spec.disk_id), true) : false) || ...);
        }, params_);
        return result;
    }

    /**
     * @brief Get disk ID by parameter index (1-based, AE style)
     * @return Disk ID, or -1 if not found
     */
    int getDiskIdByParamIndex(int param_index) const {
        int result = -1;
        int current_index = 0;
        std::apply([&](const auto&... p) {
            (void)(((current_index++ == param_index - 1 ? (result = p.spec.disk_id, true) : false) || ...));
        }, params_);
        return result;
    }

    /**
     * @brief Get parameter index by disk ID (1-based, AE style)
     * @return Parameter index, or -1 if not found
     */
    int getParamIndexByDiskId(unsigned int disk_id) const {
        int result = -1;
        int current_index = 0;
        std::apply([&](const auto&... p) {
            (void)(((disk_id == p.spec.disk_id ? (result = current_index, true) : (current_index++, false)) || ...));
        }, params_);
        return result + 1; // AE uses 1-based indexing
    }

    /**
     * @brief Set parameter visibility by disk ID
     * @tparam Source Source type
     * @return Error code (0 if successful)
     */
    template<class Source>
    int setVisible(const Source& source, unsigned int disk_id, bool visible) const {
        int err = 0;
        std::apply([&](const auto&... p) {
            (void)((disk_id == p.spec.disk_id ? (err = setVisibleOne(source, p, visible), true) : false) || ...);
        }, params_);
        return err;
    }

    /**
     * @brief Set parameter enabled state by disk ID
     * @tparam Source Source type
     * @return Error code (0 if successful)
     */
    template<class Source>
    int setEnabled(const Source& source, unsigned int disk_id, bool enabled) const {
        int err = 0;
        std::apply([&](const auto&... p) {
            (void)((disk_id == p.spec.disk_id ? (err = setEnabledOne(source, p, enabled), true) : false) || ...);
        }, params_);
        return err;
    }

private:
    // Forward declarations for implementation in sources
    template<class Builder, class P>
    void buildOne(Builder& builder, P& param);

    template<class Source, class P>
    int fetchOne(const Source& source, Bag& bag, const P& param) const;

    template<class Source, class P, class T>
    int setValueOne(const Source& source, const P& param, T Bag::*member_ptr, const T& value) const;

    template<class Source, class P>
    int setValueOne(const Source& source, const P& param, const Bag& bag) const;

    template<class Source, class P>
    int setVisibleOne(const Source& source, const P& param, bool visible) const;

    template<class Source, class P>
    int setEnabledOne(const Source& source, const P& param, bool enabled) const;
};

/**
 * @brief Helper function to create a parameter set
 * Accepts builder objects and stores them directly
 * @tparam Bag Parameter struct type
 * @tparam Builders Builder types (FloatSliderBuilder, CheckboxBuilder, etc.)
 */
template<class Bag, class... Builders>
auto makeSet(Builders&&... builders) {
    // Simply forward builders - they will be stored in the tuple as-is
    // buildOne() uses overload resolution with detail::addParam to handle each type
    return ParamSet<Bag, std::decay_t<Builders>...>(std::forward<Builders>(builders)...);
}

} // namespace mp
