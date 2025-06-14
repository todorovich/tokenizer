#pragma once

struct PoolHandle {
	size_t index;
	bool operator==(const PoolHandle&) const = default;
};

template<typename T>
class CircularPool {
public:
	explicit CircularPool(size_t capacity)
		: _items(capacity), _free(capacity), _in_use(capacity, false) {
		for (size_t i = 0; i < capacity; ++i)
			_free[i] = i;
	}

	std::optional<PoolHandle> try_acquire() {
		if (_free.empty()) return std::nullopt;
		size_t index = _free.back();
		_free.pop_back();
		_in_use[index] = true;
		return PoolHandle{index};
	}

	T& get(const PoolHandle& h) {
		if (!_in_use[h.index]) throw std::runtime_error("access to released slot");
		return _items[h.index];
	}

	void release(const PoolHandle& h) {
		if (!_in_use[h.index]) throw std::runtime_error("double free");
		_in_use[h.index] = false;
		_free.push_back(h.index);
	}

private:
	std::vector<T> _items;
	std::vector<size_t> _free;
	std::vector<bool> _in_use;
};