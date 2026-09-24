#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <random>
#include <mutex>
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;

// Seeded once, shared safely across request threads via mutex.
static std::mt19937 g_rng(std::random_device{}());
static std::mutex g_rng_mutex;

static bool coin_flip()
{
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    std::mt19937 rng(static_cast<unsigned int>(ns));
    std::uniform_int_distribution<int> dist(0, 1);

    return dist(rng) == 1;
}

int main(int argc, char* argv[])
{
    // Get parameters
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port;
    try {
        size_t pos;
        port = std::stoi(argv[1], &pos);
        if (pos != std::string(argv[1]).size())
            throw std::invalid_argument("trailing characters");
        if (port < 1 || port > 65535)
            throw std::out_of_range("port out of range");
    } catch (const std::exception& e) {
        std::cerr << "Invalid port '" << argv[1] << "': " << e.what() << std::endl;
        return 1;
    }

    httplib::Server server;

    // Open server to all origins
    server.set_default_headers({
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type"}
            });

    // Raw string regex, matches any path. So this catches OPTIONS request to any route.
    server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    server.Get("/api/choose", [](const httplib::Request& req, httplib::Response& res){
        json out;

        try {
            // Get parameters from request
            std::string finp_str;
            if (req.has_param("finp"))
                finp_str = req.get_param_value("finp");

            std::string sinp_str;
            if (req.has_param("sinp"))
                sinp_str = req.get_param_value("sinp");

            // Validate none of them are empty
            if (sinp_str.length() == 0 || finp_str.length() == 0)
                throw std::invalid_argument("Both input fields should have some content");

            // Select randomly from two choices
            bool option = coin_flip();

            // Build response
            out["choice"] = (option) ? finp_str : sinp_str;

            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            // Catch exceptions
            res.status = 400;
            json err = {{"error", e.what()}};
            res.set_content(err.dump(), "application/json");
        }
    });

    printf("API hosted at http://0.0.0.0:%d\n", port);
    if (!server.listen("0.0.0.0", port)) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return 1;
    }

    return 0;
}
