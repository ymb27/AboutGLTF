#ifndef ZLIB_WRAPPER_HPP
#define ZLIB_WRAPPER_HPP
#define ZLIB_CONST
#include <zlib/zlib.h>
#include <limits>
#include <vector>
#ifdef _DEBUG
#include <iostream>
#endif
namespace zlib_wrapper {
	enum COMPRESS_FORMAT {
		/* setting window size to control compress format */
		/* 9~15 means using zlib, and has zlib header */
		/* (9~ 15) + 16 means using gzip, of course has gzip header */
		/* (-9~-15) means using raw flate, there won't be any header or check sum */
		ZLIB = 15,
		GZIP = 31,
		RAW = -15
	};
	const COMPRESS_FORMAT format = RAW;
	inline int compress(std::vector<uint8_t>& dest,
		const std::vector<uint8_t>& src) {
		z_stream strm;
		int ret = Z_NULL;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.data_type = Z_TEXT;
		/* init zlib, use raw flate */
		ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			format, 8, Z_DEFAULT_STRATEGY);
		/* TODO: better exception handler */
		if (ret != Z_OK) {
			return ret;
		}
		dest.resize(deflateBound(&strm, src.size()));

		uInt max = std::numeric_limits<uInt>::max();

		uInt destAvailOut = dest.size();
		uInt srcAvailIn = src.size();
		strm.next_out = dest.data();
		strm.next_in = src.data();
		strm.avail_out = 0;
		strm.avail_in = 0;

		do {
			if (strm.avail_out == 0) {
				strm.avail_out = destAvailOut > max ? max : destAvailOut;
				destAvailOut -= strm.avail_out;
			}
			if (strm.avail_in == 0) {
				strm.avail_in = srcAvailIn > max ? max : srcAvailIn;
				srcAvailIn -= strm.avail_in;
			}
			ret = deflate(&strm, srcAvailIn ? Z_NO_FLUSH : Z_FINISH);
		} while (ret == Z_OK);
		/* TODO: better exception handler */
		if (ret != Z_STREAM_END) {
			deflateEnd(&strm);
			return ret;
		}
#ifdef _DEBUG
		std::cout << "src size: " << strm.total_in << '\n'
			<< "cmp size: " << strm.total_out << '\n';
#endif
		/* deflateBound returns more space than actual compressed data size */
		if (strm.total_out < dest.size())
			dest.resize(strm.total_out);
		deflateEnd(&strm);
		return ret;
	}
	/* caller should guarantee dest has enough space */
	inline int uncompress(std::vector<uint8_t>& dest,
		const std::vector<uint8_t>& src) {
		z_stream strm;
		int ret = Z_NULL;
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		ret = inflateInit2(&strm, format);
		/* TODO: better exception handler */
		if (ret != Z_OK)
			return ret;

		uInt max = std::numeric_limits<uInt>::max();
		uInt destAvailOut = dest.size();
		uInt srcAvailIn = src.size();
		strm.avail_out = 0;
		strm.avail_in = 0;
		strm.next_in = src.data();
		strm.next_out = dest.data();

		do {
			if (strm.avail_out == 0) {
				strm.avail_out = destAvailOut > max ? max : destAvailOut;
				destAvailOut -= strm.avail_out;
			}
			if (strm.avail_in == 0) {
				strm.avail_in = srcAvailIn > max ? max : srcAvailIn;
				srcAvailIn -= strm.avail_in;
			}
			ret = inflate(&strm, srcAvailIn ? Z_NO_FLUSH : Z_FINISH);
		} while (ret == Z_OK);
		/* TODO: better exception handler */
		if (ret != Z_STREAM_END) {
			inflateEnd(&strm);
			return ret;
		}
		inflateEnd(&strm);
		return ret;
	}
};
#endif