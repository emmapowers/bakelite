template <typename T, typename S = uint8_t>
struct SizedArray {
  T *data = nullptr;
  S size = 0;

  const T &at(size_t pos) const {
    return data[pos];
  }
};

class BufferStream {
public:
  BufferStream(char *buff, uint32_t size,
                char *heap = nullptr, uint32_t heapSize = 0):
    m_buff(buff),
    m_size(size),
    m_pos(0),
    m_heap(heap),
    m_heapPos(0),
    m_heapSize(heapSize)
  {}

  int write(const char *data, uint32_t length) {
    uint32_t endPos = m_pos + length;
    if(endPos > m_size) {
      return -1;
    }

    memcpy(m_buff+m_pos, data, length);
    m_pos += length;

    return 0;
  }

  int read(char *data, uint32_t length) {
    uint32_t endPos = m_pos + length;
    if(endPos > m_size) {
      return -2;
    }

    memcpy(data, m_buff+m_pos, length);
    m_pos += length;

    return 0;
  }

  int seek(uint32_t pos) {
    if(pos >= m_size) {
      return -3;
    }
    else {
      m_pos = pos;
    }
    return 0;
  }

  uint32_t size() const {
    return m_size;
  }

  uint32_t pos() const {
    return m_pos;
  }

  char *alloc(size_t bytes) {
    size_t newPos = m_heapPos + bytes;
    if(newPos >= m_heapSize)
      return nullptr;

    char *data = &m_heap[m_heapPos];
    m_heapPos = newPos;
    return data;
  }

private:
  char *m_buff;
  size_t m_size;
  size_t m_pos;
  
  char *m_heap;
  size_t m_heapPos;
  size_t m_heapSize;
};

template <class T, class V>
int write(T& stream, V val) {
  return stream.write((const char *)&val, sizeof(val));
}

template <class T, class V, class F>
int writeArray(T& stream, const V *val, int size, F writeCb) {
  for(int i = 0; i < size; i++) {
    int rcode = writeCb(stream, val[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T, class V, class F>
int writeArray(T& stream, const SizedArray<V> &val, F writeCb) {
  write(stream, val.size);
  for(int i = 0; i < val.size; i++) {
    int rcode = writeCb(stream, val.at(i));
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T>
int writeBytes(T& stream, const char *val, int size) {
  return stream.write((const char *)val, size);
}

template <class T>
int writeBytes(T& stream, const SizedArray<char> &val) {
  int rcode = write(stream, val.size);
  return stream.write((const char *)val.data, val.size);
}

template <class T>
int writeString(T& stream, const char *val, int size) {
  return stream.write(val, size);
}

template <class T>
int writeString(T& stream, const char *val) {
  if(val == nullptr) {
    return write(stream, (uint8_t)0);
  }

  uint8_t len = strlen(val);
  int rcode = stream.write(val, len);
  if(rcode != 0)
    return rcode;
  return write(stream, (uint8_t)0);
}

template <class T, class V>
int read(T& stream, V &val) {
  return stream.read((char *)&val, sizeof(val));
}

template <class T, class V, class F>
int readArray(T& stream, V val[], int size, F readCb) {
  for(int i = 0; i < size; i++) {
    int rcode = readCb(stream, val[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T, class V, class F, class S = uint8_t>
int readArray(T& stream, SizedArray<V, S> &val, F readCb) {
  S size = 0;
  int rcode = read(stream, size);
  if(rcode != 0)
      return rcode;

  val.data = (V*)stream.alloc(sizeof(V) * size);
  val.size = size;

  if(val.data == nullptr) {
    return -4;
  }

  for(int i = 0; i < size; i++) {
    int rcode = readCb(stream, val.data[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class T>
int readBytes(T& stream, char *val, int size) {
  return stream.read(val, size);
}

template <class T, typename S = uint8_t>
int readBytes(T& stream, SizedArray<char, S> &val) {
  S size = 0;
  int rcode = read(stream, size);
  if(rcode != 0)
      return rcode;

  val.data = stream.alloc(size);
  val.size = size;

  if(val.data == nullptr) {
    return -5;
  }

  return stream.read(val.data, val.size);
}

template <class T>
int readString(T& stream, char *val, int size) {
  return stream.read(val, size);
}

template <class T>
int readString(T& stream, char* &val) {
  char *newByte = stream.alloc(1);
  val = newByte;

  do {
    int rcode = stream.read(newByte, 1);
    if(rcode != 0)
      return rcode;

    if(*newByte == 0) {
      return 0;
    }
  } while((newByte = stream.alloc(1)) != nullptr);

  return -6;
}