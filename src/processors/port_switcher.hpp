/**
 * @file port_switcher.hpp
 * @author André Zeps
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "interfaces.hpp"
#include <memory>
#include <tuple>

/**
 * @brief Proxy for \ref ControllerPortInterface to swap data flow targets
 * Instances come always in pairs.
 */
class PortSwitcher : public ControllerPortInterface {
  private:
    PortSwitcher() {
        PRINTF("PortSwitcher + %p\n", this);
    }

    /// @brief Other instance to swap controller ports with
    std::weak_ptr<PortSwitcher> swap_sibling_;

    /// @brief Own data sink for controller port data
    std::shared_ptr<ControllerPortInterface> target_;

  public:
    virtual ~PortSwitcher() {
        PRINTF("PortSwitcher -\n");
    }

    /**
     * @brief Constructs two instances of this class which are interconnected
     *
     * @param ta    first controller port target
     * @param tb    second controller port target
     * @return std::pair<std::shared_ptr<PortSwitcher>,
     * std::shared_ptr<PortSwitcher>> Pair of instances
     */
    static std::pair<std::shared_ptr<PortSwitcher>, std::shared_ptr<PortSwitcher>>
    construct_pair(std::shared_ptr<ControllerPortInterface> ta, std::shared_ptr<ControllerPortInterface> tb) {
        std::shared_ptr<PortSwitcher> a = std::shared_ptr<PortSwitcher>(new PortSwitcher());
        std::shared_ptr<PortSwitcher> b = std::shared_ptr<PortSwitcher>(new PortSwitcher());

        a->target_ = ta;
        b->target_ = tb;

        a->swap_sibling_ = b;
        b->swap_sibling_ = a;

        return std::make_pair(a, b);
    }

    /**
     * @brief Performs the swap
     * The data sink of both instances are swapped with each other.
     * It is recommended to repeat the pin muxing process after doing so!
     */
    void swap() {
        PRINTF("Port Swap %p!\n", this);

        if (swap_sibling_.use_count() > 0 && swap_sibling_.expired() == false) {
            std::swap(target_, swap_sibling_.lock()->target_);
        }
    }
    void set_port_state(ControllerPortState &state) override {
        target_->set_port_state(state);
    }
    uint get_pot_x_drain_gpio() override {
        return target_->get_pot_x_drain_gpio();
    };
    uint get_pot_y_drain_gpio() override {
        return target_->get_pot_y_drain_gpio();
    };
    uint get_pot_y_sense_gpio() override {
        return target_->get_pot_y_sense_gpio();
    };
    void configure_gpios() override {
        target_->configure_gpios();
    };

    const char *get_name() override {
        return target_->get_name();
    }

    size_t get_index() override {
        return target_->get_index();
    }
};
