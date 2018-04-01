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
