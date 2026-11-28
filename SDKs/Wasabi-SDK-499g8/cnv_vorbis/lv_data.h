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
/*
main.obj : error LNK2001: unresolved external symbol _ogg_stream_flush
main.obj : error LNK2001: unresolved external symbol _ogg_stream_packetin
main.obj : error LNK2001: unresolved external symbol _vorbis_analysis_headerout
main.obj : error LNK2001: unresolved external symbol _ogg_stream_init
main.obj : error LNK2001: unresolved external symbol _vorbis_block_init
main.obj : error LNK2001: unresolved external symbol _vorbis_analysis_init
main.obj : error LNK2001: unresolved external symbol _vorbis_comment_init
main.obj : error LNK2001: unresolved external symbol _vorbis_encode_init_vbr
main.obj : error LNK2001: unresolved external symbol _vorbis_info_init
main.obj : error LNK2001: unresolved external symbol _ogg_stream_pageout
main.obj : error LNK2001: unresolved external symbol _vorbis_analysis
main.obj : error LNK2001: unresolved external symbol _vorbis_analysis_blockout
main.obj : error LNK2001: unresolved external symbol _vorbis_analysis_wrote
main.obj : error LNK2001: unresolved external symbol _vorbis_analysis_buffer
main.obj : error LNK2001: unresolved external symbol _vorbis_info_clear
main.obj : error LNK2001: unresolved external symbol _vorbis_comment_clear
main.obj : error LNK2001: unresolved external symbol _vorbis_dsp_clear
main.obj : error LNK2001: unresolved external symbol _vorbis_block_clear
main.obj : error LNK2001: unresolved external symbol _ogg_stream_clear
*/

declare_funcc(ov_time_total)
declare_funcc(ov_read_float)
declare_funcc(ov_info)
declare_funcc(ov_time_seek)
declare_funcc(ov_open_callbacks)
declare_funcc(ov_clear)
