/*

  Nullsoft WASABI Source File License

  Copyright 1999-2003 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

*/
#define ADPCM_BUF 0x400
class CAdpcmPlayer : public CWavPlayerRd
{
public:
	BYTE buf[ADPCM_BUF];
	UINT bufpos,data;
	UINT bits;
	bool eof;
	void checkbuf(bool* ks);
	UINT read_bits(UINT bits,bool * ks);
	UINT read_nibble(bool* ks) {return read_bits(4,ks);}
	UINT read_byte(bool* ks) {return read_bits(8,ks);}
	UINT read_buf(void*,UINT,bool*);
	virtual UINT GetLengthSamp();
	void GetInfoInternal(WAVEINFO* wi,char* tag);
	void flush(UINT p,bool* ks);
};