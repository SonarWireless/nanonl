#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nanonl/nl_80211.h>

int main(void)
{
	struct nl_80211_ctx *ctx = nl_80211_open();
	if (!ctx) {
		return 1;
	}

	int r = nl_80211_set_regdomain(ctx, "UA");
	printf("set reg domain: %d\n", r);
	nl_80211_close(ctx);
	return 0;
}
