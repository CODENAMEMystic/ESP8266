#ifndef SSIDList_h
#define SSIDList_h

#include <EEPROM.h>
#include "Mac.h"
#include "MacList.h"

#define listAdr 2048
#define lenAdr 2047
#define SSIDListLength 48
#define SSIDLength 32

extern const bool debug;

class SSIDList
{
  public:
    SSIDList();

    void load();
    void clear();
    void add(String name);
    void addClone(String name);
    void edit(int num, String name);
    String get(int num);
    void remove(int num);
    void _random();
    void save();
    int len = 0;
  private:

    char letters[67] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x20, 0x2c, 0x2e, 0x2d, 0x5f};
    char names[SSIDListLength][SSIDLength];
};

#endif

