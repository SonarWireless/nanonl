#pragma once

#include <nanonl/nl.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void *nla_data(const struct nlattr *nla)
{
	return (__u8 *)nla + NLA_HDRLEN;
}

static inline __u32 nla_len(const struct nlattr *nla)
{
	return nla->nla_len - NLA_HDRLEN;
}

static inline __u32 nla_get_u32(struct nlattr *nla)
{
	return *(__u32 *)nla_data(nla);
}

static inline __u16 nla_get_u16(struct nlattr *nla)
{
	return *(__u16 *)nla_data(nla);
}

static inline __u8 nla_get_u8(struct nlattr *nla)
{
	return *(__u8 *)nla_data(nla);
}

static inline void nla_put_u8(struct nlmsghdr *m, __u16 type, __u8 val)
{
	nl_add_attr(m, type, &val, sizeof(val));
}

static inline void nla_put_u16(struct nlmsghdr *m, __u16 type, __u16 val)
{
	nl_add_attr(m, type, &val, sizeof(val));
}

static inline void nla_put_u32(struct nlmsghdr *m, __u16 type, __u32 val)
{
	nl_add_attr(m, type, &val, sizeof(val));
}

#ifdef __cplusplus
}
#endif
