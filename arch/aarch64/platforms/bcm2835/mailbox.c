#include "aarch64/vmem.h"
#include "bcm2835_mmio.h"
#include "bcm2835_mailbox.h"
#include "kio.h"

static void kpu32x(uint32_t val) {
	char hexbuf[9];
	hexbuf[8] = '\0';
	for (int i = 0; i < 8; i++) {
		int dig = val & 0xf;
		hexbuf[7 - i] = (dig > 9 ? 'a' - 10 : '0') + dig;
		val >>= 4;
	}
	kputs(hexbuf);
}

// this is awkward to use, but homework syscall spec seems to require it
uint32_t mbox_call(int channel, void *mbox) {
	uint64_t addr = (uint64_t) mbox;
	addr = addr > HIGH_MEM_OFFSET ? addr - HIGH_MEM_OFFSET : addr;
	uint32_t msg = (((uint64_t) addr) & ~MAILBOX_CHANNEL_MASK) | channel;
	while (*MAILBOX_STATUS & MAILBOX_FULL);
	*MAILBOX_1_WRITE = msg;
	while (*MAILBOX_STATUS & MAILBOX_EMPTY);
	return *MAILBOX_0_READ;
}

void bcm2835_mbox_print_info() {
	DEFINE_MAILBOX_BUFFER(buf,
		DEFINE_MAILBOX_TAG(model, 4)
		DEFINE_MAILBOX_TAG(revision, 4)
		DEFINE_MAILBOX_TAG(meminfo, 8)
		DEFINE_MAILBOX_TAG(vmeminfo, 8));
	INIT_MAILBOX_BUFFER(buf);
	INIT_MAILBOX_TAG(buf.model);
	buf.model.id = TAG_GET_BOARD_MODEL;
	INIT_MAILBOX_TAG(buf.revision);
	buf.revision.id = TAG_GET_BOARD_REVISION;
	INIT_MAILBOX_TAG(buf.meminfo);
	buf.meminfo.id = TAG_GET_CPU_MEMINFO;
	INIT_MAILBOX_TAG(buf.vmeminfo);
	buf.vmeminfo.id = TAG_GET_VC_MEMINFO;

	if (mbox_call(MAILBOX_CHANNEL_PROP, &buf) != ((((uint64_t)&buf - HIGH_MEM_OFFSET) &
				~MAILBOX_CHANNEL_MASK) | MAILBOX_CHANNEL_PROP)) {
		kputs("ERR: unexpected value on mailbox read\n");
		return;
	} else if (buf.code != 0x80000000) {
		kputs("WARN: unexpected buffer code on mailbox read\n");
	}

	kputs("Board 0x");
	kpu32x(buf.model.buffer_u32[0]);
	kputs(", rev: 0x");
	kpu32x(buf.revision.buffer_u32[0]);
	kputs("\nMemory base addr: 0x");
	kpu32x(buf.meminfo.buffer_u32[0]);
	kputs(", sz: 0x");
	kpu32x(buf.meminfo.buffer_u32[1]);
	kputs("\nVC mem base addr: 0x");
	kpu32x(buf.vmeminfo.buffer_u32[0]);
	kputs(", sz: 0x");
	kpu32x(buf.vmeminfo.buffer_u32[1]);
	kputs("\n");
};
