#pragma once

#include <stdexcept>

namespace urara {

template<typename T>
struct increment_traits {
  void operator()(T& x) const {
    ++x;
  }
};

template<typename T>
struct increment_by_traits {
  T delta;

  increment_by_traits(const T& _delta): delta(_delta) {}

  void operator()(T& x) const {
    x += delta;
  }
};

template<typename T, typename Increment = increment_traits<T>>
class numeric_range {
public:
  class iterator;

  enum class direction {
    increasing,
    decreasing
  };

  using value_type = T;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using difference_type = void;
  using size_type = void;
  using iterator_category = std::input_iterator_tag;

  numeric_range(const T& from, const T& to): 
    current(from), final(to), dir(direction::increasing) {
  }

  numeric_range(const T& from, const T& to, const Increment& inc, const direction& dir):
    current(from), final(to), inc(inc), dir(dir) {
  }

  iterator begin() {
    return iterator(this);
  }

  iterator end() {
    return iterator(nullptr);
  }

  class iterator {
  private:
    class postinc_return;

  public:
    iterator(numeric_range* _range): range(_range) {
      check_done();
    }

    iterator& operator++() {
      if (range == nullptr) {
        throw std::runtime_error("increment a past-the-end iterator.");
      }
      range->inc(range->current);
      check_done();
      return *this;
    }

    postinc_return operator++(int) {
      postinc_return temp(**this);
      ++*this;
      return temp;
    }

    value_type operator*() const {
      return range->current;
    }

    pointer operator->() const {
      return &range->current;
    }

    bool operator==(const iterator& rhs) const {
      return range == rhs.range;
    }

    bool operator!=(const iterator& rhs) const {
      return !(*this == rhs);
    }

  private:
    numeric_range* range;

    void check_done() {
      if (range != nullptr && range->is_end()) {
        range = nullptr;
      }
    }

    class postinc_return {
    public:
      postinc_return(const value_type& _value): value(_value) {}

      value_type operator*() {
        return std::move(value);
      }
    private:
      value_type value;
    };
  };

private:
  bool is_end() const {
    if (dir == direction::increasing) {
      return current >= final;
    } else {
      return current <= final;
    }
  }

private:
  value_type current;
  value_type final;
  Increment inc;
  direction dir;
};

template<typename T>
numeric_range<T> range(const T& from, const T& to) {
  return numeric_range<T>(from, to);
}

template<typename T>
numeric_range<T> range(const T& to) {
  return numeric_range<T>(T(), to);
}

template<typename T>
numeric_range<T, increment_by_traits<T>> range(const T& from, const T& to, const T& delta) {
  if (!delta) {
    throw std::runtime_error("step must be non-zero.");
  }
  using direction = typename numeric_range<T, increment_by_traits<T>>::direction;
  const direction dir = (delta > T()) ? direction::increasing : direction::decreasing;
  return numeric_range<T, increment_by_traits<T>>(from, to, increment_by_traits<T>(delta), dir);
}

}
