#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include <pathwatch.h>

#include <queue>
#include <thread>
#include <random>

namespace local {
    template<typename RNG>
    std::string randstring(int len, RNG& rng) {
        const static std::string chars = "abcdefghijklmnopqrstuvwxyz";
        std::string res(len, ' ');
        std::sample(chars.begin(), chars.end(), res.begin(), res.length(), rng);
        return res;
    }


    template<typename T>
    void writeTo(std::filesystem::path path, T t) {
        std::ofstream off(path);
        off << t;
    }

    std::string nowStr() {

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");
        return oss.str();
    }

    struct TmpFile {
        template<typename RNG>
        TmpFile(RNG& rng)
        {
            auto now = nowStr();
            path = std::filesystem::temp_directory_path() / "pathwatch-testing" / now / (local::randstring(8, rng) + ".tmp");
            REQUIRE_FALSE(std::filesystem::exists(path));
        }
        ~TmpFile() {
            if (std::filesystem::exists(path)) {
                std::cout << "Deleting" << std::endl;
                std::filesystem::remove(path);
            }
        }

        std::filesystem::path path;
    };
}

TEST_CASE("FileCreationTest", "[create]") {
    std::mt19937 mt;
    mt.seed(time(NULL));

    using namespace pathwatch::actions;
    std::queue<pathwatch::PathWatcher::Action> queue;
    {
        pathwatch::PathWatcher watcher;
        {

            local::TmpFile tmpFile(mt);

            std::filesystem::create_directories(tmpFile.path.parent_path());

            bool ready = false;
            watcher.watch(tmpFile.path.parent_path(), [&](auto action) {
                REQUIRE(queue.size() > 0);
                std::visit([&](auto expected) {
                    using Expected = std::decay_t<decltype(expected)>;
                    using Got = std::decay_t<decltype(action)>;
                    std::cout << "Expected: " << Expected::type << std::endl;
                    std::cout << "Got: " << Got::type << std::endl;
                    CHECK(std::is_same_v< Expected, Got>);
                    queue.pop();

                    ready = true;
                    }, queue.front());

                });

            queue.emplace(FileAdded{});
            local::writeTo(tmpFile.path, "Line Added");


            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            queue.emplace(FileRemoved{});


            //  REQUIRE(ready);
            //  REQUIRE(queue.empty());
        }
    }

}
