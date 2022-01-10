template <typename T, typename S = uint8_t>
struct SizedArray {
  T *data = nullptr;
  S size = 0;

  const T &at(size_t pos) const {
    return data[pos];
  }
};

class Stream {
public:
  virtual int write(const char *data, uint32_t length) = 0;
  virtual int read(char *data, uint32_t length) = 0;
  virtual char *alloc(size_t bytes) = 0;
};

class BufferStream : public Stream {
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


static int write(Stream *stream, uint8_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, uint16_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, uint32_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, uint64_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, int8_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, int16_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, int32_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, int64_t val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, float val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, double val) {
  return stream->write((const char *)&val, sizeof(val));
}

static int write(Stream *stream, bool val) {
  return stream->write((const char *)&val, sizeof(val));
}

template <class V, class F>
static int writeArray(Stream *stream, const V *val, int size, F writeCb) {
  for(int i = 0; i < size; i++) {
    int rcode = writeCb(stream, val[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class V, class F>
static int writeArray(Stream *stream, const SizedArray<V> &val, F writeCb) {
  write(stream, val.size);
  for(int i = 0; i < val.size; i++) {
    int rcode = writeCb(stream, val.at(i));
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

static int writeBytes(Stream *stream, const uint8_t *val, int size) {
  return stream->write((const char *)val, size);
}

static int writeBytes(Stream *stream, const SizedArray<uint8_t> &val) {
  int rcode = write(stream, val.size);
  return stream->write((const char *)val.data, val.size);
}

static int writeString(Stream *stream, const char *val, int size) {
  return stream->write(val, size);
}

static int writeString(Stream *stream, const char *val) {
  if(val == nullptr) {
    return write(stream, (uint8_t)0);
  }

  uint8_t len = strlen(val);
  int rcode = stream->write(val, len);
  if(rcode != 0)
    return rcode;
  return write(stream, (uint8_t)0);
}

static int read(Stream *stream, uint8_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, uint16_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, uint32_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, uint64_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, int8_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, int16_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, int32_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, int64_t &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, float &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, double &val) {
  return stream->read((char *)&val, sizeof(val));
}

static int read(Stream *stream, bool &val) {
  return stream->read((char *)&val, sizeof(val));
}

template <class V, class F>
static int readArray(Stream *stream, V val[], int size, F readCb) {
  for(int i = 0; i < size; i++) {
    int rcode = readCb(stream, val[i]);
    if(rcode != 0)
      return rcode;
  }
  return 0;
}

template <class V, class F, class S = uint8_t>
static int readArray(Stream *stream, SizedArray<V, S> &val, F readCb) {
  S size = 0;
  int rcode = read(stream, size);
  if(rcode != 0)
      return rcode;

  val.data = (V*)stream->alloc(sizeof(V) * size);
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

static int readBytes(Stream *stream, uint8_t *val, int size) {
  return stream->read((char *)val, size);
}

static int readBytes(Stream *stream, SizedArray<uint8_t, uint8_t> &val) {
  uint8_t size = 0;
  int rcode = read(stream, size);
  if(rcode != 0)
      return rcode;

  val.data = (uint8_t*)stream->alloc(size);
  val.size = size;

  if(val.data == nullptr) {
    return -5;
  }

  return stream->read((char *)val.data, val.size);
}

static int readString(Stream *stream, char *val, int size) {
  return stream->read(val, size);
}

static int readString(Stream *stream, char* &val) {
  char *newByte = stream->alloc(1);
  val = newByte;

  do {
    int rcode = stream->read(newByte, 1);
    if(rcode != 0)
      return rcode;

    if(*newByte == 0) {
      return 0;
    }
  } while((newByte = stream->alloc(1)) != nullptr);

  return -6;
}