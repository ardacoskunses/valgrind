/*--------------------------------------------------------------------*/
/*--- nvtrace: mmaptracer tool that tracks NVidia ioctls           ---*/
/*--------------------------------------------------------------------*/

/*
   Copyright (C) 2006 Dave Airlie
   Copyright (C) 2007 Wladimir J. van der Laan
   Copyright (C) 2009 Marcin Slusarz <marcin.slusarz@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/

/*
	Vg_UserMsg for important messages
	Vg_DebugMsg for memory load/store messages
	Vg_DebugExtraMsg for other messages 
*/

#include "pub_tool_basics.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_libcassert.h"

#include "pub_tool_tooliface.h"
#include "pub_tool_debuginfo.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_options.h"
#include "pub_tool_machine.h"
#include "pub_tool_threadstate.h"
#include "pub_tool_vki.h"

#include "pub_tool_vkiscnums.h"
#include "pub_tool_libcfile.h"
#include "pub_tool_mallocfree.h"

#include "coregrind/pub_core_basics.h"
#include "coregrind/pub_core_libcassert.h"
#include "coregrind/m_syswrap/priv_types_n_macros.h"

#include <fcntl.h>
#include <string.h>

#include "mmt_nv_ioctl.h"
#include "mmt_main.h"

#define MAX_TRACE_FILES 10

#ifdef __LP64__
#define MMT_64BIT
#endif

struct mmt_mmap_data mmt_mmaps[MMT_MAX_REGIONS];
int mmt_last_region = -1;

UInt current_item = 1;

/* Command line options */
//UInt mmt_clo_offset = (UInt) -1;
int dump_load = True, dump_store = True;
static int trace_opens = False;

static struct trace_file {
	const char *path;
	fd_set fds;
} trace_files[MAX_TRACE_FILES];
static int trace_all_files = False;

static struct mmt_mmap_data *find_mmap(Addr addr)
{
	struct mmt_mmap_data *region = NULL;
	int i;

	for (i = 0; i <= mmt_last_region; i++)
	{
		region = &mmt_mmaps[i];
		if (addr >= region->start && addr < region->end)
			return region;
	}

	return NULL;
}

static void mydescribe(Addr inst_addr, char *namestr, int len)
{
#if 0
	const SegInfo *si;
	/* Search for it in segments */
	VG_(snprintf) (namestr, len, "@%08x", inst_addr);
	for (si = VG_(next_seginfo) (NULL);
		 si != NULL; si = VG_(next_seginfo) (si))
	{
		Addr base = VG_(seginfo_start) (si);
		SizeT size = VG_(seginfo_size) (si);

		if (inst_addr >= base && inst_addr < base + size)
		{
			const UChar *filename = VG_(seginfo_filename) (si);
			VG_(snprintf) (namestr, len, "@%08x (%s:%08x)", inst_addr,
					filename, inst_addr - base);

			break;
		}
	}
#else
	VG_(strcpy) (namestr, "");
#endif

}

static VG_REGPARM(2)
void trace_store(Addr addr, SizeT size, Addr inst_addr, UWord value)
{
	struct mmt_mmap_data *region;
	char valstr[64];
	char namestr[256];

	region = find_mmap(addr);
	if (!region)
		return;

	switch (size)
	{
		case 1:
			VG_(sprintf) (valstr, "0x%02lx", value);
			break;
		case 2:
			VG_(sprintf) (valstr, "0x%04lx", value);
			break;
		case 4:
			VG_(sprintf) (valstr, "0x%08lx", value);
			break;
#ifdef MMT_64BIT
		case 8:
			VG_(sprintf) (valstr, "0x%08lx,0x%08lx", value >> 32, value & 0xffffffff);
			break;
#endif
		default:
			return;
	}
	mydescribe(inst_addr, namestr, 256);

	VG_(message) (Vg_DebugMsg, "w %d:0x%04x, %s %s\n", region->id, (unsigned int)(addr - region->start), valstr, namestr);
}

static VG_REGPARM(2)
void trace_store2(Addr addr, SizeT size, Addr inst_addr, UWord value1, UWord value2)
{
	struct mmt_mmap_data *region;
	char valstr[64];
	char namestr[256];

	region = find_mmap(addr);
	if (!region)
		return;

	switch (size)
	{
		case 4:
			VG_(sprintf) (valstr, "0x%08lx,0x%08lx", value1, value2);
			break;
#ifdef MMT_64BIT
		case 8:
			VG_(sprintf) (valstr, "0x%08lx,0x%08lx,0x%08lx,0x%08lx",
					value1 >> 32, value1 & 0xffffffff,
					value2 >> 32, value2 & 0xffffffff);
			break;
#endif
		default:
			return;
	}

	mydescribe(inst_addr, namestr, 256);

	VG_(message) (Vg_DebugMsg, "w %d:0x%04x, %s %s\n", region->id, (unsigned int)(addr - region->start), valstr, namestr);
}

#ifndef MMT_64BIT
static VG_REGPARM(2)
void trace_store4(Addr addr, Addr inst_addr, UWord value1, UWord value2, UWord value3, UWord value4)
{
	struct mmt_mmap_data *region;
	char valstr[64];
	char namestr[256];

	region = find_mmap(addr);
	if (!region)
		return;

	VG_(sprintf) (valstr, "0x%08lx,0x%08lx,0x%08lx,0x%08lx", value1, value2, value3, value4);
	mydescribe(inst_addr, namestr, 256);

	VG_(message) (Vg_DebugMsg, "w %d:0x%04x, %s %s\n", region->id, (unsigned int)(addr - region->start), valstr, namestr);
}
#endif

static VG_REGPARM(2)
void trace_load(Addr addr, SizeT size, UInt inst_addr, UWord value)
{
	struct mmt_mmap_data *region;
	char valstr[64];
	char namestr[256];

	region = find_mmap(addr);
	if (!region)
		return;

	switch (size)
	{
		case 1:
			VG_(sprintf) (valstr, "0x%02lx", value);
			break;
		case 2:
			VG_(sprintf) (valstr, "0x%04lx", value);
			break;
		case 4:
			VG_(sprintf) (valstr, "0x%08lx", value);
			break;
#ifdef MMT_64BIT
		case 8:
			VG_(sprintf) (valstr, "0x%08lx,0x%08lx", value >> 32, value & 0xffffffff);
			break;
#endif
		default:
			return;
	}
	mydescribe(inst_addr, namestr, 256);

	VG_(message) (Vg_DebugMsg, "r %d:0x%04x, %s %s\n", region->id, (unsigned int)(addr - region->start), valstr, namestr);
}

static VG_REGPARM(2)
void trace_load2(Addr addr, SizeT size, UInt inst_addr, UWord value1, UWord value2)
{
	struct mmt_mmap_data *region;
	char valstr[64];
	char namestr[256];

	region = find_mmap(addr);
	if (!region)
		return;

	switch (size)
	{
		case 4:
			VG_(sprintf) (valstr, "0x%08lx,0x%08lx", value1, value2);
			break;
#ifdef MMT_64BIT
		case 8:
			VG_(sprintf) (valstr, "0x%08lx,0x%08lx,0x%08lx,0x%08lx",
					value1 >> 32, value1 & 0xffffffff,
					value2 >> 32, value2 & 0xffffffff);
			break;
#endif
		default:
			return;
	}
	mydescribe(inst_addr, namestr, 256);

	VG_(message) (Vg_DebugMsg, "r %d:0x%04x, %s %s\n", region->id, (unsigned int)(addr - region->start), valstr, namestr);
}

#ifndef MMT_64BIT
static VG_REGPARM(2)
void trace_load4(Addr addr, SizeT size, UInt inst_addr, UWord value1, UWord value2, UWord value3, UWord value4)
{
	struct mmt_mmap_data *region;
	char valstr[64];
	char namestr[256];

	region = find_mmap(addr);
	if (!region)
		return;

	VG_(sprintf) (valstr, "0x%08lx,0x%08lx,0x%08lx,0x%08lx", value1, value2, value3, value4);
	mydescribe(inst_addr, namestr, 256);

	VG_(message) (Vg_DebugMsg, "r %d:0x%04x, %s %s\n", region->id, (unsigned int)(addr - region->start), valstr, namestr);
}
#endif

static void add_trace_load1(IRSB *bb, IRExpr *addr, Int size, Addr inst_addr, IRExpr *val1)
{
	IRExpr **argv = mkIRExprVec_4(addr, mkIRExpr_HWord(size),
					  mkIRExpr_HWord(inst_addr), val1);
	IRDirty *di = unsafeIRDirty_0_N(2,
					"trace_load",
					VG_(fnptr_to_fnentry) (trace_load),
					argv);
	addStmtToIRSB(bb, IRStmt_Dirty(di));
}

static void add_trace_load2(IRSB *bb, IRExpr *addr, Int size, Addr inst_addr, IRExpr *val1, IRExpr *val2)
{
	IRExpr **argv = mkIRExprVec_5(addr, mkIRExpr_HWord(size),
					  mkIRExpr_HWord(inst_addr), val1, val2);
	IRDirty *di = unsafeIRDirty_0_N(2,
					"trace_load2",
					VG_(fnptr_to_fnentry) (trace_load2),
					argv);
	addStmtToIRSB(bb, IRStmt_Dirty(di));
}

#ifndef MMT_64BIT
static void add_trace_load4(IRSB *bb, IRExpr *addr, Int size, Addr inst_addr, IRExpr *val1, IRExpr *val2, IRExpr *val3, IRExpr *val4)
{
	IRExpr **argv = mkIRExprVec_7(addr, mkIRExpr_HWord(size),
					  mkIRExpr_HWord(inst_addr), val1, val2, val3, val4);
	IRDirty *di = unsafeIRDirty_0_N(2,
					"trace_load4",
					VG_(fnptr_to_fnentry) (trace_load4),
					argv);
	addStmtToIRSB(bb, IRStmt_Dirty(di));
}
#endif

#ifdef MMT_64BIT
static void add_trace_load(IRSB *bb, IRExpr *addr, Int size, Addr inst_addr, IRExpr *data, IRType arg_ty)
{
	IRTemp t;
	IRStmt *cast;
	IRExpr *data1, *data2;

	switch (arg_ty)
	{
		case Ity_I8:
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_8Uto64, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);

			add_trace_load1(bb, addr, size, inst_addr, data);
			break;

		case Ity_I16:
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_16Uto64, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);
			add_trace_load1(bb, addr, size, inst_addr, data);
			break;

		case Ity_F32:
			// reinterpret as I32
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF32asI32, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);

			// no break;
		case Ity_I32:
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_32Uto64, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);

			add_trace_load1(bb, addr, size, inst_addr, data);
			break;

		case Ity_F64:
			// reinterpret as I64
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF64asI64, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);
			// no break;

		case Ity_I64:
			add_trace_load1(bb, addr, size, inst_addr, data);
			break;

		case Ity_V128:
			// upper 64
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128HIto64, data));
			addStmtToIRSB(bb, cast);
			data1 = IRExpr_RdTmp(t);

			// lower 64
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128to64, data));
			addStmtToIRSB(bb, cast);
			data2 = IRExpr_RdTmp(t);

			add_trace_load2(bb, addr, sizeofIRType(Ity_I64), inst_addr, data1, data2);
			break;
		default:
			VG_(message) (Vg_UserMsg, "Warning! we missed a read of 0x%08x\n", (UInt) arg_ty);
			break;
	}
}
#else
static void add_trace_load(IRSB *bb, IRExpr *addr, Int size, Addr inst_addr, IRExpr *data, IRType arg_ty)
{
	IRTemp t;
	IRStmt *cast;
	IRExpr *data0;
	IRExpr *data1, *data2;
	IRExpr *data3, *data4;

	switch (arg_ty)
	{
		case Ity_I8:
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_8Uto32, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);

			add_trace_load1(bb, addr, size, inst_addr, data);
			break;

		case Ity_I16:
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_16Uto32, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);

			add_trace_load1(bb, addr, size, inst_addr, data);
			break;

		case Ity_F32:
			// reinterpret as I32
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF32asI32, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);

			// no break;
		case Ity_I32:
			add_trace_load1(bb, addr, size, inst_addr, data);
			break;

		case Ity_F64:
			// reinterpret as I64
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF64asI64, data));

			addStmtToIRSB(bb, cast);
			data = IRExpr_RdTmp(t);
			// no break;
		case Ity_I64:
			// we cannot pass whole 64-bit value in one parameter, so we split it

			// upper 32
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64HIto32, data));
			addStmtToIRSB(bb, cast);
			data1 = IRExpr_RdTmp(t);

			// lower 32
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64to32, data));
			addStmtToIRSB(bb, cast);
			data2 = IRExpr_RdTmp(t);

			add_trace_load2(bb, addr, sizeofIRType(Ity_I32), inst_addr, data1, data2);
			break;
		case Ity_V128:
			// upper 64
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128HIto64, data));
			addStmtToIRSB(bb, cast);
			data0 = IRExpr_RdTmp(t);

			// upper 32 of upper 64
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64HIto32, data0));
			addStmtToIRSB(bb, cast);
			data1 = IRExpr_RdTmp(t);

			// lower 32 of upper 64
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64to32, data0));
			addStmtToIRSB(bb, cast);
			data2 = IRExpr_RdTmp(t);

			// lower 64
			t = newIRTemp(bb->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128to64, data));
			addStmtToIRSB(bb, cast);
			data0 = IRExpr_RdTmp(t);

			// upper 32 of lower 64
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64HIto32, data0));
			addStmtToIRSB(bb, cast);
			data3 = IRExpr_RdTmp(t);

			// lower 32 of lower 64
			t = newIRTemp(bb->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64to32, data0));
			addStmtToIRSB(bb, cast);
			data4 = IRExpr_RdTmp(t);

			add_trace_load4(bb, addr, sizeofIRType(Ity_I32), inst_addr, data1, data2, data3, data4);
			break;
		default:
			VG_(message) (Vg_UserMsg, "Warning! we missed a read of 0x%08x\n", (UInt) arg_ty);
			break;
	}
}
#endif

static void
add_trace_store1(IRSB *bb, IRExpr *addr, Int size, Addr inst_addr,
		IRExpr *data)
{
	IRExpr **argv = mkIRExprVec_4(addr, mkIRExpr_HWord(size),
					mkIRExpr_HWord(inst_addr), data);
	IRDirty *di = unsafeIRDirty_0_N(2,
					"trace_store",
					VG_(fnptr_to_fnentry) (trace_store),
					argv);
	addStmtToIRSB(bb, IRStmt_Dirty(di));
}

static void
add_trace_store2(IRSB *bb, IRExpr *addr, Int size, Addr inst_addr,
		IRExpr *data1, IRExpr *data2)
{
	IRExpr **argv = mkIRExprVec_5(addr, mkIRExpr_HWord(size),
					mkIRExpr_HWord(inst_addr),
					data1, data2);
	IRDirty *di = unsafeIRDirty_0_N(2,
					"trace_store2",
					VG_(fnptr_to_fnentry) (trace_store2),
					argv);
	addStmtToIRSB(bb, IRStmt_Dirty(di));
}

#ifndef MMT_64BIT
static void
add_trace_store4(IRSB *bb, IRExpr *addr, Addr inst_addr,
		IRExpr *data1, IRExpr *data2, IRExpr *data3, IRExpr *data4)
{
	IRExpr **argv = mkIRExprVec_6(addr, mkIRExpr_HWord(inst_addr),
					data1, data2, data3, data4);
	IRDirty *di = unsafeIRDirty_0_N(2,
					"trace_store4",
					VG_(fnptr_to_fnentry) (trace_store4),
					argv);
	addStmtToIRSB(bb, IRStmt_Dirty(di));
}
#endif

#ifdef MMT_64BIT
static void add_trace_store(IRSB *bbOut, IRExpr *destAddr, Addr inst_addr,
				IRType arg_ty, IRExpr *data_expr)
{
	IRTemp t = IRTemp_INVALID;
	IRStmt *cast = NULL;
	IRExpr *data_expr1, *data_expr2;
	
	Int size = sizeofIRType(arg_ty);

	switch (arg_ty)
	{
		case Ity_I8:
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_8Uto64, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);

			add_trace_store1(bbOut, destAddr, size, inst_addr, data_expr);
			break;
		case Ity_I16:
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_16Uto64, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);

			add_trace_store1(bbOut, destAddr, size, inst_addr, data_expr);
			break;
		case Ity_F32:
			// reinterpret as I32
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF32asI32, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);

			// no break;
		case Ity_I32:
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_32Uto64, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);

			add_trace_store1(bbOut, destAddr, size, inst_addr, data_expr);
			break;
		case Ity_F64:
			// reinterpret as I64
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF64asI64, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);
			// no break;
		case Ity_I64:
			add_trace_store1(bbOut, destAddr, size, inst_addr, data_expr);
			break;
		case Ity_V128:
			// we cannot pass whole 128-bit value in one parameter, so we split it
			
			// upper 64
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128HIto64, data_expr));
			addStmtToIRSB(bbOut, cast);
			data_expr1 = IRExpr_RdTmp(t);

			// lower 64
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128to64, data_expr));
			addStmtToIRSB(bbOut, cast);
			data_expr2 = IRExpr_RdTmp(t);

			add_trace_store2(bbOut, destAddr, sizeofIRType(Ity_I64), inst_addr,
					data_expr1, data_expr2);

			break;
		default:
			VG_(message) (Vg_UserMsg, "Warning! we missed a write of 0x%08x\n", (UInt) arg_ty);
			break;
	}
}
#else
static void add_trace_store(IRSB *bbOut, IRExpr *destAddr, Addr inst_addr,
				IRType arg_ty, IRExpr *data_expr)
{
	IRTemp t = IRTemp_INVALID;
	IRStmt *cast = NULL;
	IRExpr *data_expr0;
	IRExpr *data_expr1, *data_expr2;
	IRExpr *data_expr3, *data_expr4;
	
	Int size = sizeofIRType(arg_ty);

	switch (arg_ty)
	{
		case Ity_I8:
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_8Uto32, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);

			add_trace_store1(bbOut, destAddr, size, inst_addr, data_expr);
			break;
		case Ity_I16:
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_16Uto32, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);

			add_trace_store1(bbOut, destAddr, size, inst_addr, data_expr);
			break;
		case Ity_F32:
			// reinterpret as I32
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF32asI32, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);

			// no break;
		case Ity_I32:
			add_trace_store1(bbOut, destAddr, size, inst_addr, data_expr);
			break;
		case Ity_F64:
			// reinterpret as I64
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_ReinterpF64asI64, data_expr));

			addStmtToIRSB(bbOut, cast);
			data_expr = IRExpr_RdTmp(t);
			// no break;
		case Ity_I64:
			// we cannot pass whole 64-bit value in one parameter, so we split it
			
			// upper 32
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64HIto32, data_expr));
			addStmtToIRSB(bbOut, cast);
			data_expr1 = IRExpr_RdTmp(t);

			// lower 32
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64to32, data_expr));
			addStmtToIRSB(bbOut, cast);
			data_expr2 = IRExpr_RdTmp(t);

			add_trace_store2(bbOut, destAddr, sizeofIRType(Ity_I32), inst_addr, data_expr1, data_expr2);
			break;
		case Ity_V128:
			// upper 64
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128HIto64, data_expr));
			addStmtToIRSB(bbOut, cast);
			data_expr0 = IRExpr_RdTmp(t);

			// upper 32 of upper 64
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64HIto32, data_expr0));
			addStmtToIRSB(bbOut, cast);
			data_expr1 = IRExpr_RdTmp(t);

			// lower 32 of upper 64
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64to32, data_expr0));
			addStmtToIRSB(bbOut, cast);
			data_expr2 = IRExpr_RdTmp(t);

			// lower 64
			t = newIRTemp(bbOut->tyenv, Ity_I64);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_V128to64, data_expr));
			addStmtToIRSB(bbOut, cast);
			data_expr0 = IRExpr_RdTmp(t);

			// upper 32 of lower 64
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64HIto32, data_expr0));
			addStmtToIRSB(bbOut, cast);
			data_expr3 = IRExpr_RdTmp(t);

			// lower 32 of lower 64
			t = newIRTemp(bbOut->tyenv, Ity_I32);
			cast = IRStmt_WrTmp(t, IRExpr_Unop(Iop_64to32, data_expr0));
			addStmtToIRSB(bbOut, cast);
			data_expr4 = IRExpr_RdTmp(t);

			add_trace_store4(bbOut, destAddr, inst_addr,
					data_expr1, data_expr2, data_expr3, data_expr4);

			break;
		default:
			VG_(message) (Vg_UserMsg, "Warning! we missed a write of 0x%08x\n", (UInt) arg_ty);
			break;
	}
}
#endif

static IRSB *mmt_instrument(VgCallbackClosure *closure,
				IRSB *bbIn,
				VexGuestLayout *layout,
				VexGuestExtents *vge,
				IRType gWordTy, IRType hWordTy)
{
	IRSB *bbOut;
	int i = 0;
	Addr inst_addr;

	if (gWordTy != hWordTy)
	{
		/* We don't currently support this case. */
		VG_(tool_panic) ("host/guest word size mismatch");
	}

	/* Set up BB */
	bbOut = deepCopyIRSBExceptStmts(bbIn);

	/* Copy verbatim any IR preamble preceding the first IMark */
	while (i < bbIn->stmts_used && bbIn->stmts[i]->tag != Ist_IMark)
	{
		addStmtToIRSB(bbOut, bbIn->stmts[i]);
		i++;
	}

	inst_addr = 0;

	for (; i < bbIn->stmts_used; i++)
	{
		IRStmt *st = bbIn->stmts[i];
		IRExpr *data_expr;
		IRType arg_ty;

		if (!st)
			continue;

		if (st->tag == Ist_IMark)
		{
			inst_addr = st->Ist.IMark.addr;
			addStmtToIRSB(bbOut, st);
		}
		else if (st->tag == Ist_Store && dump_store)
		{
			data_expr = st->Ist.Store.data;

			arg_ty = typeOfIRExpr(bbIn->tyenv, data_expr);

			add_trace_store(bbOut, st->Ist.Store.addr, inst_addr,
					arg_ty, data_expr);
			addStmtToIRSB(bbOut, st);
		}
		else if (st->tag == Ist_WrTmp && dump_load)
		{
			data_expr = st->Ist.WrTmp.data;

			if (data_expr->tag == Iex_Load)
			{
				IRTemp dest = st->Ist.WrTmp.tmp;
				IRExpr *value;

				addStmtToIRSB(bbOut, st);

				value = IRExpr_RdTmp(dest);

				arg_ty = typeOfIRExpr(bbIn->tyenv, value);

				add_trace_load(bbOut, data_expr->Iex.Load.addr,
						sizeofIRType(data_expr->Iex.Load.ty),
						inst_addr, value, arg_ty);
			}
			else
				addStmtToIRSB(bbOut, st);
		}
		else
			addStmtToIRSB(bbOut, st);

	}
	return bbOut;
}

#define TF_OPT "--mmt-trace-file="
#define TN_OPT "--mmt-trace-nvidia-ioctls"
#define TO_OPT "--mmt-trace-all-opens"
#define TA_OPT "--mmt-trace-all-files"
#define TM_OPT "--mmt-trace-marks"

static Bool mmt_process_cmd_line_option(Char * arg)
{
//	VG_(printf)("arg: %s\n", arg);
	if (VG_(strncmp)(arg, TF_OPT, strlen(TF_OPT)) == 0)
	{
		int i;
		for (i = 0; i < MAX_TRACE_FILES; ++i)
			if (trace_files[i].path == NULL)
				break;
		if (i == MAX_TRACE_FILES)
		{
			VG_(printf)("too many files to trace\n");
			return False;
		}
		trace_files[i].path = VG_(strdup)("mmt.options-parsing", arg + strlen(TF_OPT));
		return True;
	}
	else if (VG_(strcmp)(arg, TN_OPT) == 0)
	{
		mmt_trace_nvidia_ioctls = True;
		return True;
	}
	else if (VG_(strcmp)(arg, TO_OPT) == 0)
	{
		trace_opens = True;
		return True;
	}
	else if (VG_(strcmp)(arg, TA_OPT) == 0)
	{
		trace_all_files = True;
		return True;
	}
	else if (VG_(strcmp)(arg, TM_OPT) == 0)
	{
		mmt_trace_marks = True;
		return True;
	}

	return False;
}

static void mmt_print_usage(void)
{
	VG_(printf)("    " TF_OPT "path     trace loads and stores to memory mapped for\n"
		"                              this file (e.g. /dev/nvidia0) (you can pass \n"
		"                              this option multiple times)\n");
	VG_(printf)("    " TA_OPT "     trace loads and store to memory mapped for all files\n");
	VG_(printf)("    " TN_OPT "     trace ioctls on /dev/nvidiactl\n");
	VG_(printf)("    " TO_OPT "     trace all 'open' syscalls\n");
	VG_(printf)("    " TM_OPT "     send mmiotrace marks before and after ioctls\n");
}

static void mmt_print_debug_usage(void)
{
}

static void mmt_fini(Int exitcode)
{
	mmt_nv_ioctl_fini();
}

static void mmt_post_clo_init(void)
{
	mmt_nv_ioctl_post_clo_init();
}

static void pre_syscall(ThreadId tid, UInt syscallno, UWord *args, UInt nArgs)
{
	if (syscallno == __NR_ioctl)
		mmt_nv_ioctl_pre(args);
}

void mmt_free_region(int idx)
{
	if (mmt_last_region != idx)
		VG_(memmove)(mmt_mmaps + idx, mmt_mmaps + idx + 1,
				(mmt_last_region - idx) * sizeof(struct mmt_mmap_data));
	VG_(memset)(&mmt_mmaps[mmt_last_region--], 0, sizeof(struct mmt_mmap_data));
}

static void post_open(ThreadId tid, UWord *args, UInt nArgs, SysRes res)
{
	const char *path = (const char *)args[0];
	int i;

	if (trace_opens)
	{
		int flags = (int)args[1];
		int mode = (int)args[2];
		VG_(message)(Vg_DebugMsg, "sys_open: %s, flags: 0x%x, mode: 0x%x, ret: %ld\n", path, flags, mode, res._val);
	}
	if (res._isError)
		return;

	if (!trace_all_files)
	{
		for (i = 0; i < MAX_TRACE_FILES; ++i)
		{
			const char *path2 = trace_files[i].path;
			if (path2 != NULL && VG_(strcmp)(path, path2) == 0)
			{
				FD_SET(res._val, &trace_files[i].fds);
//				VG_(message)(Vg_DebugMsg, "fd %ld connected to %s\n", res._val, path);
				break;
			}
		}
	}

	mmt_nv_ioctl_post_open(args, res);
}

static void post_close(ThreadId tid, UWord *args, UInt nArgs, SysRes res)
{
	int fd = (int)args[0];
	int i;

	if (!trace_all_files)
		for(i = 0; i < MAX_TRACE_FILES; ++i)
		{
			if (trace_files[i].path != NULL && FD_ISSET(fd, &trace_files[i].fds))
			{
				FD_CLR(fd, &trace_files[i].fds);
				break;
			}
		}

	mmt_nv_ioctl_post_close(args);
}

static void post_mmap(ThreadId tid, UWord *args, UInt nArgs, SysRes res, int offset_unit)
{
	void *start = (void *)args[0];
	unsigned long len = args[1];
//	unsigned long prot = args[2];
//	unsigned long flags = args[3];
	unsigned long fd = args[4];
	unsigned long offset = args[5];
	int i;
	struct mmt_mmap_data *region;

	if (res._isError || (int)fd == -1)
		return;

	start = (void *)res._val;
	
	if (!trace_all_files)
	{
		for(i = 0; i < MAX_TRACE_FILES; ++i)
		{
			if (FD_ISSET(fd, &trace_files[i].fds))
				break;
		}
		if (i == MAX_TRACE_FILES)
		{
//			VG_(message)(Vg_DebugMsg, "fd %ld not found\n", fd);
			return;
		}
	}

	mmt_nv_ioctl_post_mmap(args, res, offset_unit);

	if (mmt_last_region + 1 >= MMT_MAX_REGIONS)
	{
		VG_(message)(Vg_UserMsg, "not enough space for new mmap!\n");
		return;
	}

	region = &mmt_mmaps[++mmt_last_region];

	region->fd = fd;
	region->id = current_item++;
	region->start = (Addr)start;
	region->end = (Addr)(((char *)start) + len);
	region->offset = offset * offset_unit;

	VG_(message) (Vg_DebugMsg,
			"got new mmap at %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
			(void *)region->start, len, region->offset, region->id);
}

static void post_munmap(ThreadId tid, UWord *args, UInt nArgs, SysRes res)
{
	void *start = (void *)args[0];
//	unsigned long len = args[1];
	int i;
	struct mmt_mmap_data *region;

	if (res._isError)
		return;

	for (i = 0; i <= mmt_last_region; ++i)
	{
		region = &mmt_mmaps[i];
		if (region->start == (Addr)start)
		{
			VG_(message) (Vg_DebugMsg,
					"removed mmap 0x%lx:0x%lx for: %p, len: 0x%08lx, offset: 0x%llx, serial: %d\n",
					region->data1, region->data2, (void *)region->start,
					region->end - region->start, region->offset, region->id);
			mmt_free_region(i);
			return;
		}
	}
}

static void post_mremap(ThreadId tid, UWord *args, UInt nArgs, SysRes res)
{
	void *start = (void *)args[0];
	unsigned long old_len = args[1];
	unsigned long new_len = args[2];
//	unsigned long flags = args[3];
	int i;
	struct mmt_mmap_data *region;

	if (res._isError)
		return;

	for (i = 0; i <= mmt_last_region; ++i)
	{
		region = &mmt_mmaps[i];
		if (region->start == (Addr)start)
		{
			region->start = (Addr) res._val;
			region->end = region->start + new_len;
			VG_(message) (Vg_DebugMsg,
					"changed mmap 0x%lx:0x%lx from: (address: %p, len: 0x%08lx), to: (address: %p, len: 0x%08lx), offset 0x%llx, serial %d\n",
					region->data1, region->data2,
					start, old_len,
					(void *)region->start, region->end - region->start,
					region->offset, region->id);
			return;
		}
	}
}

static void post_syscall(ThreadId tid, UInt syscallno, UWord *args,
			UInt nArgs, SysRes res)
{
	if (syscallno == __NR_ioctl)
		mmt_nv_ioctl_post(args);
	else if (syscallno == __NR_open)
		post_open(tid, args, nArgs, res);
	else if (syscallno == __NR_close)
		post_close(tid, args, nArgs, res);
	else if (syscallno == __NR_mmap)
		post_mmap(tid, args, nArgs, res, 1);
#ifndef MMT_64BIT
	else if (syscallno == __NR_mmap2)
		post_mmap(tid, args, nArgs, res, 4096);
#endif
	else if (syscallno == __NR_munmap)
		post_munmap(tid, args, nArgs, res);
	else if (syscallno == __NR_mremap)
		post_mremap(tid, args, nArgs, res);
}

static void mmt_pre_clo_init(void)
{
	int i;
	VG_(details_name) ("mmaptrace");
	VG_(details_version) (NULL);
	VG_(details_description) ("an MMAP tracer");
	VG_(details_copyright_author)
		("Copyright (C) 2007,2009, and GNU GPL'd, by Dave Airlie, W.J. van der Laan, Marcin Slusarz.");
	VG_(details_bug_reports_to) (VG_BUGS_TO);

	VG_(basic_tool_funcs) (mmt_post_clo_init, mmt_instrument, mmt_fini);

	VG_(needs_command_line_options) (mmt_process_cmd_line_option,
					 mmt_print_usage,
					 mmt_print_debug_usage);

	VG_(needs_syscall_wrapper) (pre_syscall, post_syscall);

	for (i = 0; i < MAX_TRACE_FILES; ++i)
		FD_ZERO(&trace_files[i].fds);

	mmt_nv_ioctl_pre_clo_init();
}

VG_DETERMINE_INTERFACE_VERSION(mmt_pre_clo_init)
