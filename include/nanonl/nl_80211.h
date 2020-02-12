#pragma once

#include <sys/types.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>

#include <linux/nl80211.h>

#include "nl.h"

struct nl_80211_ctx;

/**
 * \brief Convert frequency to channel number
 * \param[in] freq frequency.
 * \return channel number.
 */
static inline __u8 nl_80211_freq_to_channel(__u16 freq)
{
	if (freq < 200)
		return (__u8)freq;
	if (freq == 2484)
		return 14;
	if (freq < 2484)
		return (freq - 2407) / 5;
	return (freq - 5000) / 5;
}

/**
 * \brief Convert channel number to frequency
 * \param[in] chan channel number
 * \return frequency.
 */
static inline __u16 nl_80211_channel_to_freq(__u8 chan)
{
	if (chan == 0)
		return 0;
	if (chan == 14)
		return 2484;
	if (chan < 14)
		return (chan * 5) + 2407;
	return (chan + 1000) * 5;
}

/**
 * \brief Create a netlink connection to nl80211 subsystem.
 * \return nl_80211_ctx on success, NULL on error.*
 */
struct nl_80211_ctx *nl_80211_open(void);

/**
 * \brief Close a netlink connection to nl80211 subsystem.
 * \param[in] ctx       nl80211 context.
 */
void nl_80211_close(struct nl_80211_ctx *ctx);

/**
 * \brief Set frequency for current interface.
 * \param[in] ctx       nl80211 context.
 * \param[in] iface     network interface.
 * \param[in] freq      frequency to set.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_set_freq(struct nl_80211_ctx *ctx, const char *iface, __u16 freq);

/**
 * \brief Get current interface frequency (if any).
 * \param[in] ctx       nl80211 context.
 * \param[in] iface     network interface.
 * \param[out] freq     current frequency.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_get_freq(struct nl_80211_ctx *ctx, const char *iface, __u16 *freq);

/**
 * \brief Set channel for current interface.
 * \param[in] ctx       nl80211 context.
 * \param[in] iface     network interface.
 * \param[in] ch        channel to set.
 * \return 0 on success, non-zero on error.
 */
static inline int nl_80211_set_channel(struct nl_80211_ctx *ctx, const char *iface, __u16 ch)
{
	return nl_80211_set_freq(ctx, iface, nl_80211_channel_to_freq(ch));
}

/**
 * \brief Get current interface channel (if any).
 * \param[in] ctx       nl80211 context.
 * \param[in] iface     network interface.
 * \param[out] ch       current channel.
 * \return 0 on success, non-zero on error.
 */
static inline int nl_80211_get_channel(struct nl_80211_ctx *ctx, const char *iface, __u16 *ch)
{
	__u16 freq = 0;
	int rc;

	rc = nl_80211_get_freq(ctx, iface, &freq);
	if (rc == 0) {
		*ch = nl_80211_freq_to_channel(freq);
		return 0;
	}

	return rc;
}

/**
 * \brief Set current regulator domain
 * \param[in] ctx       nl80211 context.
 * \param[in] domain    country code.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_set_regdomain(struct nl_80211_ctx *ctx, const char *domain);

/**
 * \brief Reload regdb from disk
 * \param[in] ctx       nl80211 context.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_regdomain_reload(struct nl_80211_ctx *ctx);

/**
 * \brief Set interface mode
 * \param[in] ctx       nl80211 context.
 * \param[in] iface     network interface.
 * \param[in] mode      interface mode.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_set_mode(struct nl_80211_ctx *ctx, const char *iface, enum nl80211_iftype mode);

/**
 * \brief Set interface power mode
 * \param[in] ctx       nl80211 context.
 * \param[in] iface     network interface.
 * \param[in] mode      power mode.
 * \param[in] value     power value.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_set_power(struct nl_80211_ctx *ctx, const char *iface, enum nl80211_tx_power_setting mode, __u32 value);

/**
 * \brief Set interface rates
 * \param[in] ctx        nl80211 context.
 * \param[in] iface      network interface.
 * \param[in] rates2     array of rates for 2.4Ghz.
 * \param[in] rates2_len length of rates2.
 * \param[in] rates5     array of rates for 5Ghz.
 * \param[in] rates5_len length of rates5.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_set_rates(struct nl_80211_ctx *ctx, const char *iface, const __u8 *rates2, __u8 rates2_len, __u8 *rates5,
		       __u8 rates5_len);

/**
 * \brief Get supported channel list for the interface
 * \param[in]  ctx          nl80211 context.
 * \param[in]  iface        network interface.
 * \param[out] channels     preallocated array where to save.
 * \param[out] channels_len length of channels list.
 * \param[in]  only_for_2gz connect information only for 2.4Ghz band.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_get_supported_channels(struct nl_80211_ctx *ctx, const char *iface, __u8 *channels, __u8 *channels_len,
				    __u8 only_for_2gz);
