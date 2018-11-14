//#include "Encoder.h"
#include <iostream>
#include <string>
#include <zlib/zlib.h>
//using namespace GLB_PROC;

int main() {
	//Encoder ec;
	//ec.LoadBinary("../../testGLB.glb");
	std::string orgData = "hello world";
	std::string cmpData;
	std::string uncmpData;
	z_stream cmpStream;
	cmpStream.next_in = reinterpret_cast<Bytef*>(&orgData[0]);
	cmpStream.avail_in = orgData.size();
	cmpStream.total_in = 0;

	size_t cmpSize = orgData.size() + 1024;
	cmpData.reserve(cmpSize);
	cmpStream.next_out = reinterpret_cast<Bytef*>(&cmpData[0]);
	cmpStream.avail_out = cmpSize;
	cmpStream.total_out = 0;

	cmpStream.zalloc = Z_NULL;
	cmpStream.zfree = Z_NULL;
	if (deflateInit(&cmpStream, Z_DEFAULT_COMPRESSION) != Z_OK) {
		std::cout << "deflate init failed" << std::endl;
	}

	deflate(&cmpStream, Z_FINISH);
	deflateEnd(&cmpStream);
	
	system("pause");
	return 0;
}