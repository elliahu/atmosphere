#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <typeinfo>

namespace hammock {
    class ArgParser {
    public:
        template<typename T>
        void addArgument(const std::string &name, const std::string &help = "", bool required = false) {
            arguments_[name] = {help, required, "", typeid(T).name()};
        }

        void parse(int argc, char *argv[]) {
            std::vector<std::string> args(argv + 1, argv + argc);

            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i].rfind("--", 0) == 0) {
                    // starts with "--"
                    std::string key = args[i].substr(2);
                    if (arguments_.find(key) == arguments_.end()) {
                        throw std::invalid_argument("Unknown argument: " + key);
                    }

                    if (i + 1 < args.size() && args[i + 1].rfind("--", 0) != 0) {
                        arguments_[key].value = args[++i];
                    } else {
                        arguments_[key].value = "true"; // Boolean flag
                    }

                    arguments_[key].required = false;
                }
            }

            // Check for missing required arguments
            for (const auto &[key, arg]: arguments_) {
                if (arg.required && arg.value.empty()) {
                    throw std::invalid_argument("Missing required argument: --" + key);
                }
            }
        }

        template<typename T>
        T get(const std::string &name) const {
            if (arguments_.find(name) == arguments_.end()) {
                throw std::invalid_argument("Argument not found: " + name);
            }

            const auto &arg = arguments_.at(name);
            if (arg.value.empty()) {
                throw std::invalid_argument("Argument value is missing: " + name);
            }

            T value;
            std::istringstream iss(arg.value);

            if (!(iss >> value)) {
                throw std::invalid_argument("Failed to convert argument to type: " + name);
            }

            return value;
        }

        void printHelp() const {
            std::cout << "Arguments:\n";
            for (const auto &[key, arg]: arguments_) {
                std::cout << "  --" << key << " : " << arg.help;
                if (arg.required) {
                    std::cout << " (required)";
                }
                std::cout << '\n';
            }
        }

    private:
        struct Argument {
            std::string help;
            bool required;
            std::string value;
            std::string type;
        };

        std::unordered_map<std::string, Argument> arguments_;
    };
}
