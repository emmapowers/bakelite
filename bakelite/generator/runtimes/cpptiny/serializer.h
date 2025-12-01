/// Fixed-capacity array with runtime length, similar to std::vector but stack-allocated.
/// @tparam T Element type
/// @tparam N Maximum capacity
/// @tparam SizeT Type for length field (uint8_t for N <= 255)
template<typename T, size_t N, typename SizeT = uint8_t>
struct SizedArray {
    T data[N];
    SizeT len = 0;

    // Default constructor - empty array
    SizedArray() = default;

    // Construct from raw array and length
    SizedArray(const T* src, size_t count) : len(static_cast<SizeT>(count < N ? count : N)) {
        memcpy(data, src, len * sizeof(T));
    }

    // Construct from initializer list
    SizedArray(std::initializer_list<T> init) : len(0) {
        for (const auto& item : init) {
            if (len >= N) break;
            data[len++] = item;
        }
    }

    // Copy constructor
    SizedArray(const SizedArray& other) : len(other.len) {
        memcpy(data, other.data, len * sizeof(T));
    }

    // Move constructor
    SizedArray(SizedArray&& other) noexcept : len(other.len) {
        memcpy(data, other.data, len * sizeof(T));
        other.len = 0;
    }

    // Copy assignment
    SizedArray& operator=(const SizedArray& other) {
        if (this != &other) {
            len = other.len;
            memcpy(data, other.data, len * sizeof(T));
        }
        return *this;
    }

    // Move assignment
    SizedArray& operator=(SizedArray&& other) noexcept {
        if (this != &other) {
            len = other.len;
            memcpy(data, other.data, len * sizeof(T));
            other.len = 0;
        }
        return *this;
    }

    /// Copy data from a raw array
    void assign(const T* src, size_t count) {
        len = static_cast<SizeT>(count < N ? count : N);
        memcpy(data, src, len * sizeof(T));
    }

    // Capacity and size
    size_t size() const { return len; }
    size_t capacity() const { return N; }
    bool empty() const { return len == 0; }

    // Element access
    T& operator[](size_t i) { return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }

    // Iterators (enables range-for loops)
    T* begin() { return data; }
    T* end() { return data + len; }
    const T* begin() const { return data; }
    const T* end() const { return data + len; }

    // Modifiers
    void push_back(const T& val) { if (len < N) data[len++] = val; }
    void clear() { len = 0; }
};

class BufferStream {
public:
    BufferStream(char *buff, size_t size) :
        m_buff(buff),
        m_size(size),
        m_pos(0)
    {}

    int write(const char *data, size_t length) {
        size_t endPos = m_pos + length;
        if (endPos > m_size) {
            return -1;
        }

        memcpy(m_buff + m_pos, data, length);
        m_pos += length;

        return 0;
    }

    int read(char *data, size_t length) {
        size_t endPos = m_pos + length;
        if (endPos > m_size) {
            return -2;
        }

        memcpy(data, m_buff + m_pos, length);
        m_pos += length;

        return 0;
    }

    int seek(size_t pos) {
        if (pos >= m_size) {
            return -3;
        }
        m_pos = pos;
        return 0;
    }

    size_t size() const { return m_size; }
    size_t pos() const { return m_pos; }

private:
    char *m_buff;
    size_t m_size;
    size_t m_pos;
};

// Write primitives
template <class T, class V>
int write(T& stream, V val) {
    return stream.write((const char *)&val, sizeof(val));
}

// Write fixed-size array
template <class T, class V, class F>
int writeArray(T& stream, const V *val, size_t size, F writeCb) {
    for (size_t i = 0; i < size; i++) {
        int rcode = writeCb(stream, val[i]);
        if (rcode != 0)
            return rcode;
    }
    return 0;
}

// Write variable-length array (SizedArray)
template <class T, class V, size_t N, typename SizeT, class F>
int writeArray(T& stream, const SizedArray<V, N, SizeT> &val, F writeCb) {
    int rcode = write(stream, static_cast<SizeT>(val.len));
    if (rcode != 0)
        return rcode;
    for (size_t i = 0; i < val.len; i++) {
        rcode = writeCb(stream, val.data[i]);
        if (rcode != 0)
            return rcode;
    }
    return 0;
}

// Write fixed-size bytes
template <class T>
int writeBytes(T& stream, const char *val, size_t size) {
    return stream.write(val, size);
}

// Write variable-length bytes (SizedArray)
template <class T, size_t N, typename SizeT>
int writeBytes(T& stream, const SizedArray<uint8_t, N, SizeT> &val) {
    int rcode = write(stream, static_cast<SizeT>(val.len));
    if (rcode != 0)
        return rcode;
    return stream.write((const char *)val.data, val.len);
}

// Write null-terminated string
template <class T>
int writeString(T& stream, const char *val) {
    size_t len = strlen(val);
    int rcode = stream.write(val, len);
    if (rcode != 0)
        return rcode;
    return write(stream, (uint8_t)0);  // null terminator
}

// Read primitives (pointer to support packed struct members)
template <class T, class V>
int read(T& stream, V *val) {
    return stream.read((char *)val, sizeof(*val));
}

// Read fixed-size array
template <class T, class V, class F>
int readArray(T& stream, V val[], size_t size, F readCb) {
    for (size_t i = 0; i < size; i++) {
        int rcode = readCb(stream, &val[i]);
        if (rcode != 0)
            return rcode;
    }
    return 0;
}

// Read variable-length array into SizedArray (inline storage)
template <class T, class V, size_t N, typename SizeT, class F>
int readArray(T& stream, SizedArray<V, N, SizeT> &val, F readCb) {
    SizeT size = 0;
    int rcode = read(stream, &size);
    if (rcode != 0)
        return rcode;

    if (size > N) {
        return -4;  // Exceeds capacity
    }

    val.len = size;
    for (size_t i = 0; i < size; i++) {
        rcode = readCb(stream, &val.data[i]);
        if (rcode != 0)
            return rcode;
    }
    return 0;
}

// Read fixed-size bytes
template <class T>
int readBytes(T& stream, char *val, size_t size) {
    return stream.read(val, size);
}

// Read variable-length bytes into SizedArray (inline storage)
template <class T, size_t N, typename SizeT>
int readBytes(T& stream, SizedArray<uint8_t, N, SizeT> &val) {
    SizeT size = 0;
    int rcode = read(stream, &size);
    if (rcode != 0)
        return rcode;

    if (size > N) {
        return -5;  // Exceeds capacity
    }

    val.len = size;
    return stream.read((char *)val.data, size);
}

// Read null-terminated string into char array
template <class T, size_t N>
int readString(T& stream, char (&val)[N]) {
    size_t i = 0;
    while (i < N - 1) {
        int rcode = stream.read(&val[i], 1);
        if (rcode != 0)
            return rcode;
        if (val[i] == '\0') {
            return 0;
        }
        i++;
    }
    // Read and discard until null terminator or error
    char c;
    do {
        int rcode = stream.read(&c, 1);
        if (rcode != 0)
            return rcode;
    } while (c != '\0');
    val[N - 1] = '\0';
    return 0;
}
