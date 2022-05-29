#include "aarch64/vmem.h"
#include "bcm2835_mailbox.h"
#include "errno.h"
#include "kio.h"
#include <stdint.h>
#include <stddef.h>

static uint8_t *framebuffer = NULL;

int64_t framebuffer_pread(int minor, void *buf, size_t count, size_t offset) {
	uint8_t *c = buf;
	uint8_t *s = framebuffer + offset;
	size_t sz = count;
	while (sz--) {
		*c++ = *s++;
	}
	return count;
}

int64_t framebuffer_pwrite(int minor, const void *buf, size_t count, size_t offset) {
	const uint8_t *c = buf;
	uint8_t *s = framebuffer + offset;
	size_t sz = count;
	while (sz--) {
		*s++ = *c++;
	}
	return count;
}

struct fb_info {
	uint32_t width, height, pitch, order;
};

int framebuffer_ioctl(int minor, uint32_t request, void *data) {
	struct fb_info *info = data;
	DEFINE_MAILBOX_BUFFER(buf,
		DEFINE_MAILBOX_TAG(display_size, 8)
		DEFINE_MAILBOX_TAG(buffer_size, 8)
		DEFINE_MAILBOX_TAG(offset, 8)
		DEFINE_MAILBOX_TAG(depth, 4)
		DEFINE_MAILBOX_TAG(pixel_order, 4)
		DEFINE_MAILBOX_TAG(framebuffer, 8)
		DEFINE_MAILBOX_TAG(pitch, 8)
		);
	INIT_MAILBOX_BUFFER(buf);
	INIT_MAILBOX_TAG(buf.display_size);
	INIT_MAILBOX_TAG(buf.buffer_size);
	INIT_MAILBOX_TAG(buf.offset);
	INIT_MAILBOX_TAG(buf.depth);
	INIT_MAILBOX_TAG(buf.pixel_order);
	INIT_MAILBOX_TAG(buf.framebuffer);
	INIT_MAILBOX_TAG(buf.pitch);
	buf.display_size.id = 0x48003;
	buf.display_size.buffer_u32[0] = info->width;
	buf.display_size.buffer_u32[1] = info->height;
	buf.buffer_size.id = 0x48004;
	buf.buffer_size.buffer_u32[0] = info->width;
	buf.buffer_size.buffer_u32[1] = info->height;
	buf.offset.id = 0x48009;
	buf.offset.buffer_u32[0] = 0;
	buf.offset.buffer_u32[1] = 0;
	buf.depth.id = 0x48005;
	buf.depth.buffer_u32[0] = 32;
	buf.pixel_order.id = 0x48006;
	buf.pixel_order.buffer_u32[0] = info->order;
	buf.framebuffer.id = 0x40001;
	buf.framebuffer.buffer_u32[0] = 4096;
	buf.pitch.id = 0x40008;
	if (mbox_call(MAILBOX_CHANNEL_PROP, &buf) != ((((uint64_t)&buf - HIGH_MEM_OFFSET) &
				~MAILBOX_CHANNEL_MASK) | MAILBOX_CHANNEL_PROP)) {
		kputs("ERR: unexpected value on mailbox read\n");
		return -EINVAL;
	} else if (buf.code != 0x80000000) {
		kputs("WARN: unexpected buffer code on mailbox read\n");
	}
	info->width = buf.display_size.buffer_u32[0];
	info->height = buf.display_size.buffer_u32[1];
	info->order = buf.pixel_order.buffer_u32[0];
	info->pitch = buf.pitch.buffer_u32[0];
	framebuffer = (uint8_t *)((buf.framebuffer.buffer_u32[0] & 0x3fffffff) + HIGH_MEM_OFFSET);
	return 0;
}
