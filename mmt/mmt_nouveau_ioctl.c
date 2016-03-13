/*
   Copyright (C) 2012 Marcin Slusarz <marcin.slusarz@gmail.com>

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

#include "mmt_nouveau_ioctl.h"
#include "pub_tool_libcbase.h"
#include "pub_tool_libcprint.h"
#include "vki-linux-drm-nouveau.h"

#include <sys/select.h>

static fd_set nouveau_fds;
int mmt_trace_nouveau_ioctls = False;

void mmt_nouveau_ioctl_post_open(UWord *args, SysRes res)
{
	const char *path = (const char *)args[0];

	if (mmt_trace_nouveau_ioctls)
	{
		if (VG_(strncmp)(path, "/dev/dri/card", 13) == 0)
			FD_SET(res._val, &nouveau_fds);
	}
}

void mmt_nouveau_ioctl_post_close(UWord *args)
{
	int fd = (int)args[0];

	if (mmt_trace_nouveau_ioctls)
		FD_CLR(fd, &nouveau_fds);
}

void mmt_nouveau_ioctl_pre(UWord *args)
{
	int fd = args[0];
//	UInt id = args[1];
//	UInt *data = (UInt *) args[2];

	if (!FD_ISSET(fd, &nouveau_fds))
		return;
}

void mmt_nouveau_ioctl_post(UWord *args)
{
	int fd = args[0];
	UInt id = args[1];
	void *data = (void *) args[2];

	if (!FD_ISSET(fd, &nouveau_fds))
		return;

	if (id == VKI_DRM_IOCTL_NOUVEAU_GROBJ_ALLOC)
	{
		struct vki_drm_nouveau_grobj_alloc *arg = data;
		VG_(message) (Vg_DebugMsg,
				"create gpu object 0x%08x:0x%08x type 0x%04x (%s)\n",
				0, arg->handle, arg->class, "");
	}
	else if (id == VKI_DRM_IOCTL_NOUVEAU_GPUOBJ_FREE)
	{
		struct vki_drm_nouveau_gpuobj_free *arg = data;
		VG_(message) (Vg_DebugMsg, "destroy object 0x%08x:0x%08x\n", 0, arg->handle);
	}
}

void mmt_nouveau_ioctl_pre_clo_init(void)
{
	FD_ZERO(&nouveau_fds);
}
