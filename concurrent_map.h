#include <map>
#include <random>
#include <vector>
#include <mutex>

template <typename Key, typename Value>
class concurrent_map {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    struct Part {
        std::mutex part_m_;
        std::map<Key, Value> part_data_;
    };

    explicit concurrent_map(size_t bucket_count)
        :number_of_parts_(bucket_count), data_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        uint64_t index = key % number_of_parts_;
        return { std::lock_guard(data_[index].part_m_), data_[index].part_data_[key] };
    }

    Value at(const Key& key) const {
        uint64_t index = key % number_of_parts_;
        return data_[index].part_data_[key];
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;

        for (size_t i = 0; i < number_of_parts_; ++i) {
            std::lock_guard c(data_[i].part_m_);
            result.insert(data_[i].part_data_.begin(), data_[i].part_data_.end());
        }

        return result;
    }

private:
    size_t number_of_parts_ = 1u;
    std::vector<Part> data_;
};