#ifndef PUNCH_UCI_OPTION_H_
#define PUNCH_UCI_OPTION_H_

#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace punch {

class UciOption {
 public:
  virtual ~UciOption() = default;
  virtual std::string ToUciString() const = 0;
  virtual void Set(std::string_view value) = 0;
  virtual std::string GetName() const = 0;
};

class SpinOption : public UciOption {
 public:
  using OnChange = std::function<void(int)>;

  SpinOption(std::string_view name, int def, int min, int max,
             OnChange callback = nullptr)
      : name_(name),
        default_(def),
        value_(def),
        min_(min),
        max_(max),
        callback_(std::move(callback)) {
    if (callback_) {
      callback_(default_);
    }
  }

  std::string GetName() const override { return name_; }

  std::string ToUciString() const override {
    return std::format("option name {} type spin default {} min {} max {}",
                       name_, default_, min_, max_);
  }

  void Set(std::string_view value) override {
    value_ = std::clamp(std::stoi(std::string(value)), min_, max_);
    if (callback_) {
      callback_(value_);
    }
  }

 private:
  std::string name_;
  int default_, value_, min_, max_;
  OnChange callback_;
};

class ButtonOption : public UciOption {
 public:
  using OnClick = std::function<void()>;

  ButtonOption(std::string_view name, OnClick callback = nullptr)
      : name_(name), callback_(std::move(callback)) {}

  std::string GetName() const override { return name_; }

  std::string ToUciString() const override {
    return std::format("option name {} type button", name_);
  }

  void Set(std::string_view) override {
    if (callback_) {
      callback_();
    }
  }

 private:
  std::string name_;
  OnClick callback_;
};

class OptionManager {
 public:
  void Add(std::unique_ptr<UciOption> option) {
    options_[option->GetName()] = std::move(option);
  }

  bool Set(std::string_view name, std::string_view value) {
    std::string name_str(name);
    if (options_.contains(name_str)) {
      options_[name_str]->Set(value);
      return true;
    }
    return false;
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const OptionManager& option_manager) {
    bool first = true;
    for (const auto& [_, option] : option_manager.options_) {
      if (!first) {
        os << "\n";
      }
      os << option->ToUciString();
      first = false;
    }
    return os;
  }

 private:
  std::unordered_map<std::string, std::unique_ptr<UciOption>> options_;
};

}  // namespace punch

#endif  // PUNCH_UCI_OPTION_H_
