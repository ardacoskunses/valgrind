#ifndef MMT_TRACE_BIN_H
#define MMT_TRACE_BIN_H

#include "pub_tool_basics.h"
#include "pub_tool_libcfile.h"
#include "coregrind/pub_core_libcprint.h"
#include "pub_tool_libcbase.h"

#include "mmt_trace.h"

VG_REGPARM(2)
void mmt_trace_store_bin_1(Addr addr, UWord value);
VG_REGPARM(2)
void mmt_trace_store_bin_1_ia(Addr addr, UWord value, Addr inst_addr);
VG_REGPARM(2)
void mmt_trace_store_bin_2(Addr addr, UWord value);
VG_REGPARM(2)
void mmt_trace_store_bin_2_ia(Addr addr, UWord value, Addr inst_addr);
VG_REGPARM(2)
void mmt_trace_store_bin_4(Addr addr, UWord value);
VG_REGPARM(2)
void mmt_trace_store_bin_4_ia(Addr addr, UWord value, Addr inst_addr);

#ifdef MMT_64BIT
	VG_REGPARM(2)
	void mmt_trace_store_bin_8(Addr addr, UWord value);
	VG_REGPARM(2)
	void mmt_trace_store_bin_8_ia(Addr addr, UWord value, Addr inst_addr);
#endif

VG_REGPARM(2)
void mmt_trace_store_bin_4_4(Addr addr, UWord value1, UWord value2);
VG_REGPARM(2)
void mmt_trace_store_bin_4_4_ia(Addr addr, UWord value1, UWord value2, Addr inst_addr);

#ifdef MMT_64BIT
	VG_REGPARM(2)
	void mmt_trace_store_bin_8_8(Addr addr, UWord value1, UWord value2);
	VG_REGPARM(2)
	void mmt_trace_store_bin_8_8_ia(Addr addr, UWord value1, UWord value2, Addr inst_addr);
	VG_REGPARM(2)
	void mmt_trace_store_bin_8_8_8_8(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4);
	VG_REGPARM(2)
	void mmt_trace_store_bin_8_8_8_8_ia(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4, Addr inst_addr);
#endif

#ifndef MMT_64BIT
	VG_REGPARM(2)
	void mmt_trace_store_bin_4_4_4_4(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4);
	VG_REGPARM(2)
	void mmt_trace_store_bin_4_4_4_4_ia(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4, Addr inst_addr);
#endif

VG_REGPARM(2)
void mmt_trace_load_bin_1(Addr addr, UWord value);
VG_REGPARM(2)
void mmt_trace_load_bin_1_ia(Addr addr, UWord value, Addr inst_addr);
VG_REGPARM(2)
void mmt_trace_load_bin_2(Addr addr, UWord value);
VG_REGPARM(2)
void mmt_trace_load_bin_2_ia(Addr addr, UWord value, Addr inst_addr);
VG_REGPARM(2)
void mmt_trace_load_bin_4(Addr addr, UWord value);
VG_REGPARM(2)
void mmt_trace_load_bin_4_ia(Addr addr, UWord value, Addr inst_addr);

#ifdef MMT_64BIT
	VG_REGPARM(2)
	void mmt_trace_load_bin_8(Addr addr, UWord value);
	VG_REGPARM(2)
	void mmt_trace_load_bin_8_ia(Addr addr, UWord value, Addr inst_addr);
#endif

VG_REGPARM(2)
void mmt_trace_load_bin_4_4(Addr addr, UWord value1, UWord value2);
VG_REGPARM(2)
void mmt_trace_load_bin_4_4_ia(Addr addr, UWord value1, UWord value2, Addr inst_addr);

#ifdef MMT_64BIT
	VG_REGPARM(2)
	void mmt_trace_load_bin_8_8(Addr addr, UWord value1, UWord value2);
	VG_REGPARM(2)
	void mmt_trace_load_bin_8_8_ia(Addr addr, UWord value1, UWord value2, Addr inst_addr);
	VG_REGPARM(2)
	void mmt_trace_load_bin_8_8_8_8(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4);
	VG_REGPARM(2)
	void mmt_trace_load_bin_8_8_8_8_ia(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4, Addr inst_addr);
#endif

#ifndef MMT_64BIT
	VG_REGPARM(2)
	void mmt_trace_load_bin_4_4_4_4(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4);
	VG_REGPARM(2)
	void mmt_trace_load_bin_4_4_4_4_ia(Addr addr, UWord value1, UWord value2, UWord value3, UWord value4, Addr inst_addr);
#endif

void mmt_bin_write_1(UChar u8);
void mmt_bin_write_2(UShort u16);
void mmt_bin_write_4(UInt u32);
void mmt_bin_write_8(ULong u64);
void mmt_bin_write_str(const char *str);
void mmt_bin_write_buffer(UChar *buffer, int len);

void mmt_bin_flush(void);

#define mmt_bin_end() \
		mmt_bin_write_1(10)

#endif
