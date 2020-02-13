#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/if.h>
#include <sys/time.h>

#include <linux/nl80211.h>
#include <linux/version.h>

#include <nanonl/nl_gen.h>
#include <nanonl/nl_attr.h>
#include <nanonl/nl_80211.h>

#if NL80211_DEBUG
#define dd(fmt, ...)                                                                                                   \
	fprintf(stdout, "[%04llu] nl: [%s:%d] " fmt "\n", nl_debug_timestamp(), __func__, __LINE__, ##__VA_ARGS__)
static void nl_hex_dump(const void *data, size_t size)
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char *)data)[i]);
		if (((unsigned char *)data)[i] >= ' ' && ((unsigned char *)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char *)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i + 1) % 8 == 0 || i + 1 == size) {
			printf(" ");
			if ((i + 1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i + 1 == size) {
				ascii[(i + 1) % 16] = '\0';
				if ((i + 1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i + 1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}

#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000ull
#endif /* USEC_PER_SEC */

static __u64 nl_debug_timestamp()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (USEC_PER_SEC * (__u64)tv.tv_sec + (__u64)tv.tv_usec) & 0xFFFFFFFFull;
}
#else
#define dd(...)
#define nl_hex_dump(...)
#endif

// clang-format off
struct nl_80211_ctx {
	__u32 seq;               // message number
	__u32 pid;               // process ID
	__u16 family;            // family number
	__u8 buf_rx[8192];       // see NLMSG_GOODSIZE
	__u8 buf_tx[8192];       // see NLMSG_GOODSIZE
	struct nlmsghdr *msg_rx; // ptr to buf_rx
	struct nlmsghdr *msg_tx; // ptr to buf_tx
	int fd;
};
// clang-format on

int nl_80211_set_buffer_size(struct nl_80211_ctx *ctx)
{
	int cap_rx = sizeof(ctx->buf_rx);
	int cap_tx = sizeof(ctx->buf_tx);

	if (setsockopt(ctx->fd, SOL_SOCKET, SO_SNDBUF, &cap_tx, sizeof(cap_tx)) < 0)
		return -1;

	if (setsockopt(ctx->fd, SOL_SOCKET, SO_RCVBUF, &cap_rx, sizeof(cap_rx)) < 0)
		return -2;

	dd("ctx %p: set buffer size: rx/tx: %d/%d (bytes)", ctx, cap_rx, cap_tx);
	return 0;
}

static int nl_80211_exec(struct nl_80211_ctx *ctx)
{
	__u32 dest_pid = 0; /* Should be always ZERO in direction to the kernel */
	ssize_t n;

	n = nl_send(ctx->fd, dest_pid, ctx->msg_tx);
	dd("ctx %p: msg with seq [%d] sent: [%zd] (bytes)", ctx, ctx->msg_tx->nlmsg_seq, n);
	if (ctx->msg_tx->nlmsg_len != n) {
		return -1;
	}

	while ((n = nl_recv(ctx->fd, ctx->msg_rx, sizeof(ctx->buf_rx), NULL)) > 0) {
		dd("ctx: %p: got message with seq [%d], len: [%zd] (bytes)", ctx, ctx->msg_rx->nlmsg_seq, n);
		if (ctx->msg_rx->nlmsg_seq == ctx->msg_tx->nlmsg_seq) {
			break;
		}
		dd("ctx: %p: invalid seq: [%d]", ctx, ctx->msg_rx->nlmsg_seq);
	}

	if (n < 0) {
		if (errno < 0) {
			dd("ctx: %p: netlink error: [%d] (%s)", ctx, errno, strerror(abs(errno)));
			return -2;
		} else {
			dd("ctx: %p: failed to do the request: %s", ctx, strerror(errno));
			return -3;
		}
	}

	dd("ctx: %p: command seq [%d]: OK", ctx, ctx->msg_tx->nlmsg_seq);
	return 0;
}

static void nl_80211_command(struct nl_80211_ctx *ctx, __u8 cmd)
{
	// Reset the buffer
	memset(ctx->buf_tx, 0, sizeof(ctx->buf_tx));

	// Create a new request
	dd("ctx: %p: set command: [%u] (%#x)", ctx, cmd, cmd);
	nl_gen_request(ctx->msg_tx, ctx->pid, ctx->family, cmd, 1);

	// Update seq
	ctx->msg_tx->nlmsg_seq = ++ctx->seq;
}

static int nl_80211_add_iface(struct nl_80211_ctx *ctx, const char *iface)
{
	__u32 idx;

	if (!iface)
		return -1;

	idx = if_nametoindex(iface);
	if (!idx) {
		dd("interface error: %s: %s", iface, strerror(errno));
		return -2;
	}

	dd("ctx %p: interface: [%s] --> idx [%u]", ctx, iface, idx);
	nla_put_u32(ctx->msg_tx, NL80211_ATTR_IFINDEX, idx);
	return 0;
}

struct nl_80211_ctx *nl_80211_open(void)
{
	struct nl_80211_ctx *ctx = calloc(1, sizeof(struct nl_80211_ctx));
	struct nlattr *nla;

	assert(ctx);

	ctx->pid = (__u32)getpid();
	ctx->msg_rx = (struct nlmsghdr *)ctx->buf_rx;
	ctx->msg_tx = (struct nlmsghdr *)ctx->buf_tx;
	ctx->fd = nl_open(NETLINK_GENERIC, ctx->pid);
	ctx->seq = (__u32)time(0);
	if (ctx->fd < 0) {
		goto fail;
	}

	dd("ctx: %p: created", ctx);

	nl_80211_set_buffer_size(ctx);

	/* We are going to use non blocking IO */
	fcntl(ctx->fd, F_SETFL, O_NONBLOCK);

	/* Create lookup request */
	nl_gen_find_family(ctx->msg_tx, NL80211_GENL_NAME);

	if (nl_80211_exec(ctx))
		goto fail;

	if ((nla = nl_gen_get_attr(ctx->msg_rx, CTRL_ATTR_FAMILY_ID))) {
		ctx->family = nla_get_u16(nla);
		dd("ctx: %p, family [%d], pid: [%d]", ctx, ctx->family, getpid());
		return ctx;
	} else {
		dd("ctx: %p: family id for [%s] was not found", ctx, NL80211_GENL_NAME);
	}

fail:
	if (ctx->fd >= 0) {
		close(ctx->fd);
	}
	free(ctx);
	return NULL;
}

void nl_80211_close(struct nl_80211_ctx *ctx)
{
	assert(ctx);

	close(ctx->fd);
	free(ctx);
}

int nl_80211_get_freq(struct nl_80211_ctx *ctx, const char *iface, __u16 *freq)
{
	struct nlattr *attrs[NL80211_ATTR_MAX + 1];

	nl_80211_command(ctx, NL80211_CMD_GET_INTERFACE);

	if (nl_80211_add_iface(ctx, iface))
		return -1;

	if (nl_80211_exec(ctx))
		return -2;

	__u16 count = nl_gen_get_attrv(ctx->msg_rx, attrs);
	if ((count > 0) && (attrs[NL80211_ATTR_WIPHY_FREQ])) {
		*freq = nla_get_u32(attrs[NL80211_ATTR_WIPHY_FREQ]);
		dd("ctx: %p: freq for [%s] is [%u]", ctx, iface, *freq);
		return 0;
	}

	return -3;
}

int nl_80211_set_freq(struct nl_80211_ctx *ctx, const char *iface, __u16 freq)
{
	nl_80211_command(ctx, NL80211_CMD_SET_CHANNEL);
	nla_put_u32(ctx->msg_tx, NL80211_ATTR_WIPHY_FREQ, freq);

	if (nl_80211_add_iface(ctx, iface))
		return -1;

	dd("ctx: %p: set freq for [%s]: [%u]", ctx, iface, freq);
	return nl_80211_exec(ctx);
}

int nl_80211_set_regdomain(struct nl_80211_ctx *ctx, const char *domain)
{
	char alpha2[3];

	nl_80211_command(ctx, NL80211_CMD_REQ_SET_REG);

	alpha2[0] = toupper(domain[0]);
	alpha2[1] = toupper(domain[1]);
	alpha2[2] = '\0';
	nl_add_attr(ctx->msg_tx, NL80211_ATTR_REG_ALPHA2, alpha2, sizeof(alpha2));

	dd("ctx: %p: set reg domain to [%s]", ctx, alpha2);
	return nl_80211_exec(ctx);
}

int nl_80211_regdomain_reload(struct nl_80211_ctx *ctx)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 15, 0)
	nl_80211_command(ctx, NL80211_CMD_RELOAD_REGDB);

	dd("ctx: %p: regdb reload", ctx);
	return nl_80211_exec(ctx);
#else
	errno = ENOSYS;
	return -1;
#endif
}

int nl_80211_set_mode(struct nl_80211_ctx *ctx, const char *iface, enum nl80211_iftype mode)
{
	nl_80211_command(ctx, NL80211_CMD_SET_INTERFACE);

	if (nl_80211_add_iface(ctx, iface))
		return -1;

	nla_put_u32(ctx->msg_tx, NL80211_ATTR_IFTYPE, mode);

	dd("ctx: %p: set interface [%s] mode to [%u]", ctx, iface, mode);
	return nl_80211_exec(ctx);
}

int nl_80211_set_power(struct nl_80211_ctx *ctx, const char *iface, enum nl80211_tx_power_setting mode, __u32 value)
{
	switch (mode) {
	case NL80211_TX_POWER_AUTOMATIC:
	case NL80211_TX_POWER_FIXED:
	case NL80211_TX_POWER_LIMITED:
	default:
		return -1;
	}

	nl_80211_command(ctx, NL80211_CMD_SET_WIPHY);

	if (nl_80211_add_iface(ctx, iface))
		return -2;

	nla_put_u32(ctx->msg_tx, NL80211_ATTR_WIPHY_TX_POWER_SETTING, mode);
	if (mode != NL80211_TX_POWER_AUTOMATIC) {
		nla_put_u32(ctx->msg_tx, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, value);
	}

	dd("ctx: %p: set interface [%s] power mode to [%u] with value [%u]", ctx, iface, mode, value);
	return nl_80211_exec(ctx);
}

static void nl_80211_set_rate_attr(struct nl_80211_ctx *ctx, const __u8 *rates, __u8 len, enum nl80211_band band)
{
	struct nlattr *nla;

	if (!rates)
		return;

	nla = nla_start(ctx->msg_tx, band);
	nla_add_attr(nla, NL80211_TXRATE_LEGACY, rates, len);
	nla_end(ctx->msg_tx, nla);

	dd("ctx: %p: set rates for band [%d] len [%u]", ctx, band, len);
	return;
}

int nl_80211_set_rates(struct nl_80211_ctx *ctx, const char *iface, const __u8 *rates2, __u8 rates2_len, __u8 *rates5,
		       __u8 rates5_len)
{
	struct nlattr *nla;

	if ((rates2_len > 32) || (rates5_len > 32)) {
		return -1;
	}

	if (rates2_len && !rates2) {
		return -2;
	}

	if (rates5_len && !rates5) {
		return -3;
	}

	nl_80211_command(ctx, NL80211_CMD_SET_TX_BITRATE_MASK);
	if (nl_80211_add_iface(ctx, iface))
		return -4;

	nla = nla_start(ctx->msg_tx, NL80211_ATTR_TX_RATES);
	nl_80211_set_rate_attr(ctx, rates2, rates2_len, NL80211_BAND_2GHZ);
	nl_80211_set_rate_attr(ctx, rates5, rates5_len, NL80211_BAND_5GHZ);
	nla_end(ctx->msg_tx, nla);

	dd("ctx: %p: set rates for interface [%s] len2 [%u], len5 [%u]", ctx, iface, rates2_len, rates5_len);
	return nl_80211_exec(ctx);
}

int nl_80211_get_supported_channels(struct nl_80211_ctx *ctx, const char *iface, __u8 *channels, __u8 *channels_len,
				    __u8 only_for_2gz)
{
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1] = { 0 };
	struct nlattr *tb_band[NL80211_BAND_ATTR_MAX + 1] = { 0 };
	struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1] = { 0 };
	struct nlattr *nl_band;
	struct nlattr *nl_freq;
	__u8 list_idx = 0;

	if (!(channels && channels_len && *channels_len))
		return -1;

	nl_80211_command(ctx, NL80211_CMD_GET_WIPHY);
	if (nl_80211_add_iface(ctx, iface))
		return -2;

	if (nl_80211_exec(ctx))
		return -3;

	dd("ctx: %p: parsing to [%p] with len [%u]", ctx, channels, *channels_len);

	// Now parse our bands:
	// NL80211_ATTR_WIPHY_BANDS --> NL80211_BAND_ATTR_FREQS --> NL80211_FREQUENCY_ATTR_FREQ
	__u16 iface_attrs_count = nl_gen_get_attrv(ctx->msg_rx, tb_msg);
	if (!(iface_attrs_count && tb_msg[NL80211_ATTR_WIPHY_BANDS])) {
		dd("ctx: %p: interface [%s]: attr count: [%u]: cannot find [%u]", ctx, iface, iface_attrs_count,
		   NL80211_ATTR_WIPHY_BANDS);
	}

	// Ok, we have NL80211_ATTR_WIPHY_BANDS
	nla_each(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS])
	{
		__u16 band_attrs_count = nla_get_attrv(nl_band, tb_band, NL80211_BAND_ATTR_MAX);
		dd("ctx: %p: processing band [%p] with attr count [%u]", ctx, nl_band, band_attrs_count);
		// Make GCC happy
		band_attrs_count = band_attrs_count;

		if (!tb_band[NL80211_BAND_ATTR_FREQS]) {
			dd("ctx: %p: band freq attr [%u] doesn't exist", ctx, NL80211_BAND_ATTR_FREQS);
			continue;
		}

		// Almost done, now process NL80211_BAND_ATTR_FREQS
		nla_each(nl_freq, tb_band[NL80211_BAND_ATTR_FREQS])
		{
			__u16 freq_attrs_count = nla_get_attrv(nl_freq, tb_freq, NL80211_FREQUENCY_ATTR_MAX);

			if (!(freq_attrs_count && tb_freq[NL80211_FREQUENCY_ATTR_FREQ])) {
				dd("ctx: %p: band [%p]: freq attr [%u] doesn't exist", ctx, nl_band,
				   NL80211_FREQUENCY_ATTR_FREQ);
				continue;
			}

			// Our goal
			__u16 freq = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);
			__u8 chan = nl_80211_freq_to_channel(freq);

			if (tb_freq[NL80211_FREQUENCY_ATTR_DISABLED]) {
				dd("ctx: %p: chan [%u] at [%u] is disabled", ctx, chan, freq);
				continue;
			}

			if ((!only_for_2gz || chan < 15) && (list_idx < *channels_len)) {
				channels[list_idx++] = chan;
			}
		}
	}

	if (list_idx) {
		*channels_len = list_idx;
		return 0;
	}

	return -4;
}
