#include "utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <io.h>
#include <vector>
#include <array>
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <unistd.h>
#endif
using namespace std;


uint32_t adler32(uint32_t adler, const uint8_t *data, size_t len) {
	uint32_t s1 =(adler)?adler & 0xFFFF:1&0xFFFF;
	uint32_t s2 = (adler)?adler >> 16:1>>16;
	for (size_t i = 0; i < len; i++) {
		s1 = (s1 + data[i]) % 65521;
		s2 = (s2 + s1) % 65521;
	}
	return s2 << 16 | s1;
}

uint32_t crc32(uint32_t initial,const void *data, size_t length)
{
	static const uint32_t crc32tab[16] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190,
		0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344,
		0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278,
		0xbdbdf21c};

	const uint8_t *buf = (const uint8_t *)data;
	uint32_t crc =(initial)? initial ^ 0xffffffff:0xffffffff;

	if (length == 0)
		return 0;

	for (uint32_t i = 0; i < length; ++i)
	{
		crc ^= buf[i];
		crc = crc32tab[crc & 0x0f] ^ (crc >> 4);
		crc = crc32tab[crc & 0x0f] ^ (crc >> 4);
	}

	return crc ^ 0xffffffff;
}




void MD5(uint8_t *hash_data,uint8_t *data, size_t len ) {
  HCRYPTPROV hProv = 0;
  HCRYPTPROV hHash = 0;
  CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0);
  CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
  CryptHashData(hHash, data, len, 0);
  DWORD cbHash = 16;
  CryptGetHashParam(hHash, HP_HASHVAL, hash_data, &cbHash, 0);
  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);
}

void SHA1(uint8_t *hash_data,uint8_t *data, size_t len) {
  HCRYPTPROV hProv = 0;
  HCRYPTPROV hHash = 0;
  CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 0);
  CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
  CryptHashData(hHash, data, len, 0);
  DWORD cbHash = 20;
  CryptGetHashParam(hHash, HP_HASHVAL, hash_data, &cbHash, 0);
  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);
}

void SHA256(uint8_t *hash_data,uint8_t *data, size_t len) {
  HCRYPTPROV hProv = 0;
  HCRYPTPROV hHash = 0;
  CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES,CRYPT_VERIFYCONTEXT);
  CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
  CryptHashData(hHash, data, len, 0);
  DWORD cbHash = 32;
  CryptGetHashParam(hHash, HP_HASHVAL, hash_data, &cbHash, 0);
  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);
}

