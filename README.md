nanonl
======

This library implements a small set of helper functions for constructing
netlink messages, and looking up netlink attributes (NLAs) within nmessages.

The helper functions for each subsystem mau be independently enabled.

cmake Options (`-D<opt name>=<ON|OFF>`)
-----------------
```
  ENABLE_ALL            enable support for everything
  ENABLE_GENERIC        enable netlink generic support
  ENABLE_80211          enable simple nl80211 API
  ENABLE_NETFILTER      enable nfnetlink support
  ENABLE_NFQUEUE        enable nfqueue support (implies netfilter)
  ENABLE_IFINFO         enable interface info support
  ENABLE_IFADDR         enable interface address support
  ENABLE_SAMPLES        enable sample code
```

What this library doesn't do
----------------------------

- Manage memory
- Chew bubble gum (it's all out...)

