#include "be_cross.h"

static unsigned char EGAHEAD_C3D[] = {
 BE_Cross_SwapGroup24LE(0x00, 0x00, 0x00) BE_Cross_SwapGroup24LE(0x7b, 0x01, 0x00) BE_Cross_SwapGroup24LE(0x89, 0x01, 0x00) BE_Cross_SwapGroup24LE(0xa8, 0x01, 0x00) BE_Cross_SwapGroup24LE(0x93, 0x06, 0x00)
 BE_Cross_SwapGroup24LE(0x48, 0x0a, 0x00) BE_Cross_SwapGroup24LE(0x09, 0x0b, 0x00) BE_Cross_SwapGroup24LE(0x17, 0x0c, 0x00) BE_Cross_SwapGroup24LE(0xd8, 0x0c, 0x00) BE_Cross_SwapGroup24LE(0x94, 0x0d, 0x00)
 BE_Cross_SwapGroup24LE(0xb9, 0x0e, 0x00) BE_Cross_SwapGroup24LE(0x18, 0x10, 0x00) BE_Cross_SwapGroup24LE(0xf1, 0x10, 0x00) BE_Cross_SwapGroup24LE(0x1d, 0x12, 0x00) BE_Cross_SwapGroup24LE(0x4c, 0x13, 0x00)
 BE_Cross_SwapGroup24LE(0x4a, 0x14, 0x00) BE_Cross_SwapGroup24LE(0x4e, 0x15, 0x00) BE_Cross_SwapGroup24LE(0x4d, 0x16, 0x00) BE_Cross_SwapGroup24LE(0xe6, 0x16, 0x00) BE_Cross_SwapGroup24LE(0x50, 0x17, 0x00)
 BE_Cross_SwapGroup24LE(0xab, 0x18, 0x00) BE_Cross_SwapGroup24LE(0xc8, 0x62, 0x00) BE_Cross_SwapGroup24LE(0x65, 0x9e, 0x00) BE_Cross_SwapGroup24LE(0x6a, 0xd6, 0x00) BE_Cross_SwapGroup24LE(0xd6, 0xfe, 0x00)
 BE_Cross_SwapGroup24LE(0xb9, 0x27, 0x01) BE_Cross_SwapGroup24LE(0x27, 0x33, 0x01) BE_Cross_SwapGroup24LE(0x57, 0x3a, 0x01) BE_Cross_SwapGroup24LE(0x2c, 0x40, 0x01) BE_Cross_SwapGroup24LE(0xcf, 0x54, 0x01)
 BE_Cross_SwapGroup24LE(0x26, 0x6c, 0x01) BE_Cross_SwapGroup24LE(0x6c, 0x80, 0x01) BE_Cross_SwapGroup24LE(0xb0, 0x91, 0x01) BE_Cross_SwapGroup24LE(0x8e, 0xa4, 0x01) BE_Cross_SwapGroup24LE(0x47, 0xb9, 0x01)
 BE_Cross_SwapGroup24LE(0x06, 0xcc, 0x01) BE_Cross_SwapGroup24LE(0x21, 0xe3, 0x01) BE_Cross_SwapGroup24LE(0x34, 0xe3, 0x01) BE_Cross_SwapGroup24LE(0xe8, 0xe3, 0x01) BE_Cross_SwapGroup24LE(0x62, 0xe6, 0x01)
 BE_Cross_SwapGroup24LE(0xa2, 0xe9, 0x01) BE_Cross_SwapGroup24LE(0x17, 0xed, 0x01) BE_Cross_SwapGroup24LE(0x63, 0xf1, 0x01) BE_Cross_SwapGroup24LE(0xc0, 0xf5, 0x01) BE_Cross_SwapGroup24LE(0x18, 0xfa, 0x01)
 BE_Cross_SwapGroup24LE(0x57, 0xfe, 0x01) BE_Cross_SwapGroup24LE(0x7d, 0x02, 0x02) BE_Cross_SwapGroup24LE(0xc5, 0x06, 0x02) BE_Cross_SwapGroup24LE(0x1c, 0x0b, 0x02) BE_Cross_SwapGroup24LE(0x84, 0x0f, 0x02)
 BE_Cross_SwapGroup24LE(0xce, 0x13, 0x02) BE_Cross_SwapGroup24LE(0x2b, 0x18, 0x02) BE_Cross_SwapGroup24LE(0x75, 0x1c, 0x02) BE_Cross_SwapGroup24LE(0xb0, 0x20, 0x02) BE_Cross_SwapGroup24LE(0xd9, 0x24, 0x02)
 BE_Cross_SwapGroup24LE(0x1c, 0x29, 0x02) BE_Cross_SwapGroup24LE(0x67, 0x2d, 0x02) BE_Cross_SwapGroup24LE(0xbd, 0x31, 0x02) BE_Cross_SwapGroup24LE(0x55, 0x4c, 0x02) BE_Cross_SwapGroup24LE(0x64, 0x4c, 0x02)
 BE_Cross_SwapGroup24LE(0x77, 0x4e, 0x02) BE_Cross_SwapGroup24LE(0xa6, 0x50, 0x02) BE_Cross_SwapGroup24LE(0xc3, 0x52, 0x02) BE_Cross_SwapGroup24LE(0xed, 0x54, 0x02) BE_Cross_SwapGroup24LE(0xbe, 0x56, 0x02)
 BE_Cross_SwapGroup24LE(0xb2, 0x58, 0x02) BE_Cross_SwapGroup24LE(0xc8, 0x5a, 0x02) BE_Cross_SwapGroup24LE(0x88, 0x5c, 0x02) BE_Cross_SwapGroup24LE(0xff, 0x5d, 0x02) BE_Cross_SwapGroup24LE(0x53, 0x5f, 0x02)
 BE_Cross_SwapGroup24LE(0x9c, 0x62, 0x02) BE_Cross_SwapGroup24LE(0xc0, 0x65, 0x02) BE_Cross_SwapGroup24LE(0xdf, 0x68, 0x02) BE_Cross_SwapGroup24LE(0x18, 0x6c, 0x02) BE_Cross_SwapGroup24LE(0x4a, 0x6f, 0x02)
 BE_Cross_SwapGroup24LE(0x3b, 0x73, 0x02) BE_Cross_SwapGroup24LE(0x54, 0x77, 0x02) BE_Cross_SwapGroup24LE(0x11, 0x7b, 0x02) BE_Cross_SwapGroup24LE(0x82, 0x7e, 0x02) BE_Cross_SwapGroup24LE(0xf6, 0x80, 0x02)
 BE_Cross_SwapGroup24LE(0x26, 0x83, 0x02) BE_Cross_SwapGroup24LE(0x81, 0x86, 0x02) BE_Cross_SwapGroup24LE(0xbb, 0x89, 0x02) BE_Cross_SwapGroup24LE(0xe9, 0x8c, 0x02) BE_Cross_SwapGroup24LE(0x2f, 0x90, 0x02)
 BE_Cross_SwapGroup24LE(0x08, 0x91, 0x02) BE_Cross_SwapGroup24LE(0xe0, 0x91, 0x02) BE_Cross_SwapGroup24LE(0xa7, 0x92, 0x02) BE_Cross_SwapGroup24LE(0x70, 0x93, 0x02) BE_Cross_SwapGroup24LE(0x5b, 0x94, 0x02)
 BE_Cross_SwapGroup24LE(0x37, 0x95, 0x02) BE_Cross_SwapGroup24LE(0x23, 0x96, 0x02) BE_Cross_SwapGroup24LE(0x0b, 0x97, 0x02) BE_Cross_SwapGroup24LE(0xe9, 0x97, 0x02) BE_Cross_SwapGroup24LE(0x27, 0x99, 0x02)
 BE_Cross_SwapGroup24LE(0x24, 0x9b, 0x02) BE_Cross_SwapGroup24LE(0xbf, 0x9b, 0x02) BE_Cross_SwapGroup24LE(0x5a, 0x9c, 0x02) BE_Cross_SwapGroup24LE(0x7e, 0x9d, 0x02) BE_Cross_SwapGroup24LE(0xb1, 0x9e, 0x02)
 BE_Cross_SwapGroup24LE(0x0d, 0xa2, 0x02) BE_Cross_SwapGroup24LE(0x54, 0xa5, 0x02) BE_Cross_SwapGroup24LE(0xa7, 0xa8, 0x02) BE_Cross_SwapGroup24LE(0xf4, 0xab, 0x02) BE_Cross_SwapGroup24LE(0x92, 0xaf, 0x02)
 BE_Cross_SwapGroup24LE(0xa4, 0xb3, 0x02) BE_Cross_SwapGroup24LE(0x12, 0xb7, 0x02) BE_Cross_SwapGroup24LE(0x9f, 0xba, 0x02) BE_Cross_SwapGroup24LE(0x8f, 0xbe, 0x02) BE_Cross_SwapGroup24LE(0x6b, 0xc1, 0x02)
 BE_Cross_SwapGroup24LE(0xc6, 0xc3, 0x02) BE_Cross_SwapGroup24LE(0xba, 0xc6, 0x02) BE_Cross_SwapGroup24LE(0xad, 0xc9, 0x02) BE_Cross_SwapGroup24LE(0xa4, 0xcb, 0x02) BE_Cross_SwapGroup24LE(0x87, 0xce, 0x02)
 BE_Cross_SwapGroup24LE(0x0d, 0xd0, 0x02) BE_Cross_SwapGroup24LE(0x6d, 0xd1, 0x02) BE_Cross_SwapGroup24LE(0xe1, 0xd2, 0x02) BE_Cross_SwapGroup24LE(0x65, 0xd4, 0x02) BE_Cross_SwapGroup24LE(0xd9, 0xd5, 0x02)
 BE_Cross_SwapGroup24LE(0x48, 0xd7, 0x02) BE_Cross_SwapGroup24LE(0xa1, 0xd8, 0x02) BE_Cross_SwapGroup24LE(0xb8, 0xd9, 0x02) BE_Cross_SwapGroup24LE(0xe7, 0xdd, 0x02) BE_Cross_SwapGroup24LE(0x17, 0xe2, 0x02)
 BE_Cross_SwapGroup24LE(0x3e, 0xe5, 0x02) BE_Cross_SwapGroup24LE(0x50, 0xe8, 0x02) BE_Cross_SwapGroup24LE(0xff, 0xea, 0x02) BE_Cross_SwapGroup24LE(0x03, 0xed, 0x02) BE_Cross_SwapGroup24LE(0x24, 0xef, 0x02)
 BE_Cross_SwapGroup24LE(0x67, 0xf1, 0x02) BE_Cross_SwapGroup24LE(0x86, 0xf3, 0x02) BE_Cross_SwapGroup24LE(0x36, 0xf5, 0x02) BE_Cross_SwapGroup24LE(0x5c, 0xf9, 0x02) BE_Cross_SwapGroup24LE(0x67, 0xf9, 0x02)
 BE_Cross_SwapGroup24LE(0x79, 0xfc, 0x02) BE_Cross_SwapGroup24LE(0xfc, 0xff, 0x02) BE_Cross_SwapGroup24LE(0x84, 0x03, 0x03) BE_Cross_SwapGroup24LE(0x7d, 0x09, 0x03) BE_Cross_SwapGroup24LE(0x84, 0x0d, 0x03)
 BE_Cross_SwapGroup24LE(0x69, 0x11, 0x03) BE_Cross_SwapGroup24LE(0x33, 0x17, 0x03) BE_Cross_SwapGroup24LE(0xca, 0x1a, 0x03) BE_Cross_SwapGroup24LE(0x2a, 0x1e, 0x03) BE_Cross_SwapGroup24LE(0xb8, 0x22, 0x03)
 BE_Cross_SwapGroup24LE(0x64, 0x29, 0x03) BE_Cross_SwapGroup24LE(0x79, 0x2c, 0x03) BE_Cross_SwapGroup24LE(0xaa, 0x31, 0x03) BE_Cross_SwapGroup24LE(0x75, 0x34, 0x03) BE_Cross_SwapGroup24LE(0x2e, 0x37, 0x03)
 BE_Cross_SwapGroup24LE(0x8b, 0x39, 0x03) BE_Cross_SwapGroup24LE(0x4a, 0x3c, 0x03) BE_Cross_SwapGroup24LE(0xa3, 0x3f, 0x03) BE_Cross_SwapGroup24LE(0x08, 0x43, 0x03) BE_Cross_SwapGroup24LE(0x00, 0x47, 0x03)
 BE_Cross_SwapGroup24LE(0xf9, 0x4a, 0x03) BE_Cross_SwapGroup24LE(0x52, 0x4e, 0x03) BE_Cross_SwapGroup24LE(0xb7, 0x51, 0x03) BE_Cross_SwapGroup24LE(0x18, 0x55, 0x03) BE_Cross_SwapGroup24LE(0x80, 0x58, 0x03)
 BE_Cross_SwapGroup24LE(0xc1, 0x5d, 0x03) BE_Cross_SwapGroup24LE(0x98, 0x69, 0x03) BE_Cross_SwapGroup24LE(0x5a, 0x6e, 0x03) BE_Cross_SwapGroup24LE(0x48, 0x74, 0x03) BE_Cross_SwapGroup24LE(0x65, 0x74, 0x03)
 BE_Cross_SwapGroup24LE(0x85, 0x74, 0x03) BE_Cross_SwapGroup24LE(0xad, 0x74, 0x03) BE_Cross_SwapGroup24LE(0xd7, 0x79, 0x03) BE_Cross_SwapGroup24LE(0xf4, 0x7a, 0x03) BE_Cross_SwapGroup24LE(0x05, 0x7b, 0x03)
 BE_Cross_SwapGroup24LE(0x5a, 0x7b, 0x03) BE_Cross_SwapGroup24LE(0xc5, 0x7b, 0x03) BE_Cross_SwapGroup24LE(0x0d, 0x7c, 0x03) BE_Cross_SwapGroup24LE(0x83, 0x7c, 0x03) BE_Cross_SwapGroup24LE(0xc8, 0x7c, 0x03)
 BE_Cross_SwapGroup24LE(0xf2, 0x7c, 0x03) BE_Cross_SwapGroup24LE(0x2d, 0x7d, 0x03) BE_Cross_SwapGroup24LE(0x99, 0x7d, 0x03) BE_Cross_SwapGroup24LE(0x1f, 0x7e, 0x03) BE_Cross_SwapGroup24LE(0x77, 0x7e, 0x03)
 BE_Cross_SwapGroup24LE(0xf9, 0x7e, 0x03) BE_Cross_SwapGroup24LE(0x55, 0x7f, 0x03) BE_Cross_SwapGroup24LE(0x9b, 0x7f, 0x03) BE_Cross_SwapGroup24LE(0xf2, 0x7f, 0x03) BE_Cross_SwapGroup24LE(0x52, 0x80, 0x03)
 BE_Cross_SwapGroup24LE(0xba, 0x80, 0x03) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0x13, 0x81, 0x03) BE_Cross_SwapGroup24LE(0x78, 0x81, 0x03)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xca, 0x81, 0x03) BE_Cross_SwapGroup24LE(0x42, 0x82, 0x03) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xa7, 0x82, 0x03) BE_Cross_SwapGroup24LE(0x0b, 0x83, 0x03) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0x5c, 0x83, 0x03) BE_Cross_SwapGroup24LE(0xc1, 0x83, 0x03) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0x13, 0x84, 0x03) BE_Cross_SwapGroup24LE(0x42, 0x84, 0x03)
 BE_Cross_SwapGroup24LE(0x95, 0x84, 0x03) BE_Cross_SwapGroup24LE(0xeb, 0x84, 0x03) BE_Cross_SwapGroup24LE(0x33, 0x85, 0x03) BE_Cross_SwapGroup24LE(0x8f, 0x85, 0x03) BE_Cross_SwapGroup24LE(0xd9, 0x85, 0x03)
 BE_Cross_SwapGroup24LE(0x20, 0x86, 0x03) BE_Cross_SwapGroup24LE(0x6f, 0x86, 0x03) BE_Cross_SwapGroup24LE(0xcc, 0x86, 0x03) BE_Cross_SwapGroup24LE(0x1f, 0x87, 0x03) BE_Cross_SwapGroup24LE(0x6a, 0x87, 0x03)
 BE_Cross_SwapGroup24LE(0xc5, 0x87, 0x03) BE_Cross_SwapGroup24LE(0x0a, 0x88, 0x03) BE_Cross_SwapGroup24LE(0x68, 0x88, 0x03) BE_Cross_SwapGroup24LE(0xc6, 0x88, 0x03) BE_Cross_SwapGroup24LE(0x22, 0x89, 0x03)
 BE_Cross_SwapGroup24LE(0x71, 0x89, 0x03) BE_Cross_SwapGroup24LE(0xca, 0x89, 0x03) BE_Cross_SwapGroup24LE(0x1d, 0x8a, 0x03) BE_Cross_SwapGroup24LE(0x67, 0x8a, 0x03) BE_Cross_SwapGroup24LE(0xa7, 0x8a, 0x03)
 BE_Cross_SwapGroup24LE(0x00, 0x8b, 0x03) BE_Cross_SwapGroup24LE(0x54, 0x8b, 0x03) BE_Cross_SwapGroup24LE(0xb1, 0x8b, 0x03) BE_Cross_SwapGroup24LE(0x0d, 0x8c, 0x03) BE_Cross_SwapGroup24LE(0x59, 0x8c, 0x03)
 BE_Cross_SwapGroup24LE(0xa1, 0x8c, 0x03) BE_Cross_SwapGroup24LE(0xd0, 0x8c, 0x03) BE_Cross_SwapGroup24LE(0xff, 0x8c, 0x03) BE_Cross_SwapGroup24LE(0x2e, 0x8d, 0x03) BE_Cross_SwapGroup24LE(0x5d, 0x8d, 0x03)
 BE_Cross_SwapGroup24LE(0x8c, 0x8d, 0x03) BE_Cross_SwapGroup24LE(0xbb, 0x8d, 0x03) BE_Cross_SwapGroup24LE(0xea, 0x8d, 0x03) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0x1d, 0x8e, 0x03) BE_Cross_SwapGroup24LE(0xb6, 0x8e, 0x03) BE_Cross_SwapGroup24LE(0x3c, 0x8f, 0x03) BE_Cross_SwapGroup24LE(0xd1, 0x8f, 0x03) BE_Cross_SwapGroup24LE(0x62, 0x90, 0x03)
 BE_Cross_SwapGroup24LE(0xd8, 0x90, 0x03) BE_Cross_SwapGroup24LE(0x1c, 0x91, 0x03) BE_Cross_SwapGroup24LE(0x7e, 0x91, 0x03) BE_Cross_SwapGroup24LE(0xd4, 0x91, 0x03) BE_Cross_SwapGroup24LE(0x41, 0x92, 0x03)
 BE_Cross_SwapGroup24LE(0x98, 0x92, 0x03) BE_Cross_SwapGroup24LE(0xef, 0x92, 0x03) BE_Cross_SwapGroup24LE(0x8a, 0x93, 0x03) BE_Cross_SwapGroup24LE(0x24, 0x94, 0x03) BE_Cross_SwapGroup24LE(0xc1, 0x94, 0x03)
 BE_Cross_SwapGroup24LE(0x60, 0x95, 0x03) BE_Cross_SwapGroup24LE(0xf6, 0x95, 0x03) BE_Cross_SwapGroup24LE(0x93, 0x96, 0x03) BE_Cross_SwapGroup24LE(0x29, 0x97, 0x03) BE_Cross_SwapGroup24LE(0xc9, 0x97, 0x03)
 BE_Cross_SwapGroup24LE(0x6a, 0x98, 0x03) BE_Cross_SwapGroup24LE(0xc9, 0x98, 0x03) BE_Cross_SwapGroup24LE(0x3a, 0x99, 0x03) BE_Cross_SwapGroup24LE(0xa5, 0x99, 0x03) BE_Cross_SwapGroup24LE(0x3f, 0x9a, 0x03)
 BE_Cross_SwapGroup24LE(0x6f, 0x9a, 0x03) BE_Cross_SwapGroup24LE(0xd2, 0x9a, 0x03) BE_Cross_SwapGroup24LE(0x1f, 0x9b, 0x03) BE_Cross_SwapGroup24LE(0xae, 0x9b, 0x03) BE_Cross_SwapGroup24LE(0x21, 0x9c, 0x03)
 BE_Cross_SwapGroup24LE(0x93, 0x9c, 0x03) BE_Cross_SwapGroup24LE(0x2c, 0x9d, 0x03) BE_Cross_SwapGroup24LE(0xc7, 0x9d, 0x03) BE_Cross_SwapGroup24LE(0x5d, 0x9e, 0x03) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xf9, 0x9e, 0x03) BE_Cross_SwapGroup24LE(0x78, 0x9f, 0x03) BE_Cross_SwapGroup24LE(0xef, 0x9f, 0x03) BE_Cross_SwapGroup24LE(0x33, 0xa0, 0x03) BE_Cross_SwapGroup24LE(0x97, 0xa0, 0x03)
 BE_Cross_SwapGroup24LE(0xf4, 0xa0, 0x03) BE_Cross_SwapGroup24LE(0x72, 0xa1, 0x03) BE_Cross_SwapGroup24LE(0xea, 0xa1, 0x03) BE_Cross_SwapGroup24LE(0x2e, 0xa2, 0x03) BE_Cross_SwapGroup24LE(0x94, 0xa2, 0x03)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff)
 BE_Cross_SwapGroup24LE(0xff, 0xff, 0xff) BE_Cross_SwapGroup24LE(0xf1, 0xa2, 0x03) BE_Cross_SwapGroup24LE(0x70, 0xa4, 0x03) BE_Cross_SwapGroup24LE(0xe1, 0xa5, 0x03) BE_Cross_SwapGroup24LE(0xde, 0xa7, 0x03)
 BE_Cross_SwapGroup24LE(0xbe, 0xa9, 0x03) BE_Cross_SwapGroup24LE(0xb9, 0xab, 0x03) BE_Cross_SwapGroup24LE(0x9b, 0xad, 0x03) BE_Cross_SwapGroup24LE(0x84, 0xaf, 0x03) BE_Cross_SwapGroup24LE(0x93, 0xb1, 0x03)
 BE_Cross_SwapGroup24LE(0x78, 0xb3, 0x03) BE_Cross_SwapGroup24LE(0x32, 0xb5, 0x03) BE_Cross_SwapGroup24LE(0x1e, 0xb7, 0x03) BE_Cross_SwapGroup24LE(0x1e, 0xb9, 0x03) BE_Cross_SwapGroup24LE(0x32, 0xbb, 0x03)
 BE_Cross_SwapGroup24LE(0x33, 0xbd, 0x03) BE_Cross_SwapGroup24LE(0x13, 0xbf, 0x03) BE_Cross_SwapGroup24LE(0x0f, 0xc1, 0x03) BE_Cross_SwapGroup24LE(0x05, 0xc3, 0x03) BE_Cross_SwapGroup24LE(0x09, 0xc5, 0x03)
 BE_Cross_SwapGroup24LE(0x11, 0xc7, 0x03) BE_Cross_SwapGroup24LE(0x66, 0xc9, 0x03) BE_Cross_SwapGroup24LE(0x56, 0xda, 0x03) BE_Cross_SwapGroup24LE(0x83, 0xeb, 0x03)
};

uint32_t *EGAhead = (uint32_t *)EGAHEAD_C3D;