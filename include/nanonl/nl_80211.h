#pragma once

#include <sys/types.h>
#include <linux/netlink.h>
#include <linux/genetlink.h>

#include "nl.h"

struct nl_80211_ctx;

/**
 * \brief Create a netlink connection to nl80211 subsystem.
 * \return nl_80211_ctx on success, NULL on error.*
 */
struct nl_80211_ctx* nl_80211_open(void);

/**
 * \brief Close a netlink connection to nl80211 subsystem.
 * \param[in] ctx       nl80211 context.
 */
void nl_80211_close(struct nl_80211_ctx *ctx);

/**
 * \brief Get current interface frequency (if any).
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
 * \param[out] freq     cirrent frequency.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_get_freq(struct nl_80211_ctx *ctx, const char *iface, __u16 *freq);

/**
 * \brief Set current regulator domain
 * \param[in] ctx       nl80211 context.
 * \param[in] domain    country code.
 * \return 0 on success, non-zero on error.
 */
int nl_80211_set_regdomain(struct nl_80211_ctx *ctx, const char *domain);
