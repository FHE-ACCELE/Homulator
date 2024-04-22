#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "Addr.h"
#include "Basic.h"
#include "mem.h"

typedef uint64_t DataType;

class Polynominal {
private:
  AddrType addressStart, addressEnd;

  std::vector<DataType> data;
  uint32_t length;

public:
  Polynominal(uint32_t size) {
    length = size;
    data.reserve(length);
  };

  void setData(std::vector<DataType> source) { data = std::move(source); };

  void setAddr(uint32_t start, uint32_t batchCount) {
    addressStart = start;
    addressEnd = start + batchCount;
  };

  AddrType getAddressStart() { return addressStart; };
  AddrType getAddressEnd() { return addressEnd; };

  ~Polynominal();
};

class Ciphertext {
private:
  uint32_t CurrentLevel;

  std::vector<Polynominal *> c0, c1;

public:
  Ciphertext(std::vector<Polynominal *> cc0, std::vector<Polynominal *> cc1) {
    c0 = cc0;
    c1 = cc1;
  };

  Ciphertext(uint32_t level, uint32_t N) {
    CurrentLevel = level;
    for (uint32_t l = 0; l < CurrentLevel; l++) {
      c0.push_back(new Polynominal(N));
      c1.push_back(new Polynominal(N));
    }
  };

  Ciphertext(uint32_t level, uint32_t N, std::vector<AddrType> &Datapool,
             uint32_t BS) {
    CurrentLevel = level;
    AddrType AddressStart = Datapool[Datapool.size() - 1] + 1;
    for (uint32_t l = 0; l < CurrentLevel; l++) {
      c0.push_back(new Polynominal(N));
      c0[l]->setAddr(AddressStart, BS);
      Datapool.push_back(AddressStart + BS);

      // std::cout<<"ciphertext addr is "<<AddressStart<<" "<<BS<<"\n";

      c1.push_back(new Polynominal(N));
      c1[l]->setAddr(AddressStart + BS, BS);
      Datapool.push_back(AddressStart + BS);

      AddressStart = AddressStart + 2 * BS;
    }
  };

  std::vector<Polynominal *> *getC0() { return &c0; };
  std::vector<Polynominal *> *getC1() { return &c1; };

  Polynominal *getC0Level(uint32_t level) { return c0[level]; };
  Polynominal *getC1Level(uint32_t level) { return c1[level]; };

  std::vector<AddrType> getC0Addr() {
    std::vector<AddrType> temp;
    for (auto &poly : c0) {
      temp.push_back(poly->getAddressStart());
    }
    return temp;
  };

  std::vector<AddrType> getC1Addr() {
    std::vector<AddrType> temp;
    for (auto &poly : c1) {
      temp.push_back(poly->getAddressStart());
    }
    return temp;
  };

  void setC0Addr(std::vector<AddrType> &addrlist, uint32_t BS) {
    uint32_t count = 0;
    for (auto &poly : c0) {
      poly->setAddr(addrlist[count], BS);
      count += 1;
    }
  };

  void setC1Addr(std::vector<AddrType> &addrlist, uint32_t BS) {
    uint32_t count = 0;
    for (auto &poly : c1) {
      poly->setAddr(addrlist[count], BS);
      count += 1;
    }
  };

  ~Ciphertext();
};

class Plaintext {
private:
  uint32_t CurrentLevel;

  std::vector<Polynominal *> c0;

public:
  Plaintext(std::vector<Polynominal *> cc0) { c0 = cc0; };

  Plaintext(uint32_t level, uint32_t N) {
    CurrentLevel = level;
    for (uint32_t l = 0; l < CurrentLevel; l++) {
      c0.push_back(new Polynominal(N));
    }
  };

  Plaintext(uint32_t level, uint32_t N, std::vector<AddrType> &Datapool,
            uint32_t BS) {
    CurrentLevel = level;
    AddrType AddressStart = Datapool[Datapool.size() - 1] + 1;
    for (uint32_t l = 0; l < CurrentLevel; l++) {
      c0.push_back(new Polynominal(N));
      c0[l]->setAddr(AddressStart, BS);
      Datapool.push_back(AddressStart + BS);

      AddressStart = AddressStart + 1 * BS;
    }
  };

  std::vector<Polynominal *> *getC0() { return &c0; };

  Polynominal *getC0Level(uint32_t level) { return c0[level]; };

  std::vector<AddrType> getC0Addr() {
    std::vector<AddrType> temp;
    for (auto &poly : c0) {
      temp.push_back(poly->getAddressStart());
    }
    return temp;
  };

  void setC0Addr(std::vector<AddrType> &addrlist, uint32_t BS) {
    uint32_t count = 0;
    for (auto &poly : c0) {
      poly->setAddr(addrlist[count], BS);
      count += 1;
    }
  };

  ~Plaintext();
};

#endif