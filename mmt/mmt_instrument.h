#ifndef MMT_INSTRUMENT_H_
#define MMT_INSTRUMENT_H_

#include "pub_tool_basics.h"
#include "pub_tool_tooliface.h"

IRSB *mmt_instrument(VgCallbackClosure *closure,
				IRSB *bbIn,
				const VexGuestLayout *layout,
				const VexGuestExtents *vge,
				const VexArchInfo* archinfo_host,
				IRType gWordTy, IRType hWordTy);

#endif /* MMT_INSTRUMENT_H_ */
