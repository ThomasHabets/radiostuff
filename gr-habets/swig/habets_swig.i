/* -*- c++ -*- */

#define HABETS_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "habets_swig_doc.i"

%{
#include "habets/ook_decode_b.h"
#include "habets/packet_size_filter.h"
#include "habets/pn_decode_f.h"
#include "habets/bitpacker.h"
#include "habets/pn_source_f.h"
#include "habets/file_pdu_generator.h"
#include "habets/bitunpacker.h"
#include "habets/packetize_burst.h"
#include "habets/stairclocker.h"
#include "habets/float_streamer.h"
#include "habets/jt65_encode.h"
#include "habets/jt65_add_sync.h"
#include "habets/jt65_decode.h"
#include "habets/tagged_stream_to_large_pdu_f.h"
%}


%include "habets/ook_decode_b.h"
GR_SWIG_BLOCK_MAGIC2(habets, ook_decode_b);
%include "habets/packet_size_filter.h"
GR_SWIG_BLOCK_MAGIC2(habets, packet_size_filter);
%include "habets/pn_decode_f.h"
GR_SWIG_BLOCK_MAGIC2(habets, pn_decode_f);
%include "habets/bitpacker.h"
GR_SWIG_BLOCK_MAGIC2(habets, bitpacker);
%include "habets/pn_source_f.h"
GR_SWIG_BLOCK_MAGIC2(habets, pn_source_f);
%include "habets/file_pdu_generator.h"
GR_SWIG_BLOCK_MAGIC2(habets, file_pdu_generator);
%include "habets/bitunpacker.h"
GR_SWIG_BLOCK_MAGIC2(habets, bitunpacker);
%include "habets/packetize_burst.h"
GR_SWIG_BLOCK_MAGIC2(habets, packetize_burst);
%include "habets/stairclocker.h"
GR_SWIG_BLOCK_MAGIC2(habets, stairclocker);
%include "habets/float_streamer.h"
GR_SWIG_BLOCK_MAGIC2(habets, float_streamer);
%include "habets/jt65_encode.h"
GR_SWIG_BLOCK_MAGIC2(habets, jt65_encode);
%include "habets/jt65_add_sync.h"
GR_SWIG_BLOCK_MAGIC2(habets, jt65_add_sync);
%include "habets/jt65_decode.h"
GR_SWIG_BLOCK_MAGIC2(habets, jt65_decode);
%include "habets/tagged_stream_to_large_pdu_f.h"
GR_SWIG_BLOCK_MAGIC2(habets, tagged_stream_to_large_pdu_f);
