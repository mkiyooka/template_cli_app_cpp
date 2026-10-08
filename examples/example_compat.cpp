#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <template_cli_app_cpp/compat/expected.hpp>
#include <template_cli_app_cpp/compat/span.hpp>

namespace {

compat::expected<double, std::string> SafeDivide(double numerator, double denominator) {
    if (denominator == 0.0) {
        return compat::unexpected<std::string>("division by zero");
    }
    return numerator / denominator;
}

double Sum(compat::span<const double> values) {
    return std::accumulate(values.begin(), values.end(), 0.0);
}

double Mean(compat::span<const double> values) {
    if (values.empty()) {
        return 0.0;
    }
    return Sum(values) / static_cast<double>(values.size());
}

} // namespace

int main() {
    // compat::expected の使用例
    std::cout << "=== compat::expected ===" << '\n';

    const auto ok = SafeDivide(10.0, 3.0);
    if (ok) {
        std::cout << "10 / 3 = " << *ok << '\n';
    }

    const auto err = SafeDivide(10.0, 0.0);
    if (!err) {
        std::cout << "10 / 0 => error: " << err.error() << '\n';
    }

    // compat::span の使用例
    std::cout << "\n=== compat::span ===" << '\n';

    const std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};

    std::cout << "sum  = " << Sum(data) << '\n';
    std::cout << "mean = " << Mean(data) << '\n';

    // span でサブレンジを渡す
    const compat::span<const double> sub(data.data() + 1, 3);
    std::cout << "sub[1..4) sum  = " << Sum(sub) << '\n';
    std::cout << "sub[1..4) mean = " << Mean(sub) << '\n';

    return 0;
}
