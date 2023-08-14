#pragma once

#include "interfaces.hpp"
#include <memory>
#include <tuple>

class PortSwitcher : public ControllerPortInterface {
  private:
    PortSwitcher() { printf("PortSwitcher + %p\n", this); }

    std::weak_ptr<PortSwitcher> swap_sibling_;
    std::shared_ptr<ControllerPortInterface> target_;

  public:
    virtual ~PortSwitcher() { printf("PortSwitcher -\n"); }

    static std::pair<std::shared_ptr<PortSwitcher>,
                     std::shared_ptr<PortSwitcher>>
    construct_pair(std::shared_ptr<ControllerPortInterface> ta,
                   std::shared_ptr<ControllerPortInterface> tb) {
        std::shared_ptr<PortSwitcher> a =
            std::shared_ptr<PortSwitcher>(new PortSwitcher());
        std::shared_ptr<PortSwitcher> b =
            std::shared_ptr<PortSwitcher>(new PortSwitcher());

        a->target_ = ta;
        b->target_ = tb;

        a->swap_sibling_ = b;
        b->swap_sibling_ = a;

        return std::make_pair(a, b);
    }

    void swap() {
        printf("Port Swap %p!\n", this);

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
    void configure_gpios() override { target_->configure_gpios(); };

    const char *get_name() override { return target_->get_name(); }
};
