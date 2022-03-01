#ifndef BCM2835_MAILBOX_H
#define BCM2835_MAILBOX_H

#define MAILBOX_CHANNEL_MASK	0xf
#define MAILBOX_CHANNEL_PROP	0x8
#define MAILBOX_REQUEST	0x0
#define TAG_REQUEST	0x0

//TODO: weird multiple tags usage without variadic macros
// (but variadic macros produce commas)
#define DEFINE_MAILBOX_BUFFER(ident, tags) struct { \
	uint32_t size, code; \
	tags \
	uint32_t end __attribute__((aligned(4))); \
} ident __attribute__((aligned(16)))

#define DEFINE_MAILBOX_TAG(ident, bufsz) struct { \
	uint32_t id, size, code; \
	union { \
		uint8_t buffer[bufsz]; \
		uint32_t buffer_u32[bufsz / 4]; \
	}; \
} ident __attribute__((aligned(4)));

#define INIT_MAILBOX_BUFFER(ident) { \
	ident.size = sizeof(ident); \
	ident.code = MAILBOX_REQUEST; \
	ident.end = 0; \
}

#define INIT_MAILBOX_TAG(ident) { \
	ident.size = sizeof(ident.buffer); \
	ident.code = TAG_REQUEST; \
}

#define TAG_GET_BOARD_MODEL	0x00010001
#define TAG_GET_BOARD_REVISION	0x00010002
#define TAG_GET_CPU_MEMINFO	0x00010005
#define TAG_GET_VC_MEMINFO	0x00010006

void bcm2835_mbox_print_info();

#endif
